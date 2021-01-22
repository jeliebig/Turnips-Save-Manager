// Copyright (C) 2020 averne
//
// This file is part of Turnips.
//
// Turnips is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Turnips is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Turnips.  If not, see <http://www.gnu.org/licenses/>.

#include <cstdio>
#include <cstdint>
#include <utility>
#include <dirent.h>
#include <switch.h>
#include <math.h>
#include <imgui.h>

#include "fs.hpp"
#include "gui.hpp"
#include "lang.hpp"
#include "save.hpp"
#include "theme.hpp"
#include "parser.hpp"

using namespace lang::literals;

constexpr static auto acnh_programid = 0x01006f8002326000ul;
constexpr static auto save_main_path = "/main.dat";
constexpr static auto save_main_hdr_path  = "/mainHeader.dat";
constexpr static auto save_personal_path = "/Villager%d/personal.dat";
constexpr static auto save_personal_hdr_path = "/Villager%d/personalHeader.dat";

constexpr static auto sd_app_dir_path = "/switch/ACNH_Save_Manager";
constexpr static auto sd_app_island_dir_path = "/switch/ACNH_Save_Manager/islands";
constexpr static auto sd_app_shared_island_dir_path = "/switch/ACNH_Save_Manager/islands/shared";

extern "C" void userAppInit() {
    setsysInitialize();
    plInitialize(PlServiceType_User);
    romfsInit();
    socketInitializeDefault();
    // nxlinkStdio();
    consoleDebugInit(debugDevice_SVC);
}

extern "C" void userAppExit() {
    setsysExit();
    plExit();
    romfsExit();
    socketExit();
}

int getPlayerNumber(fs::Filesystem &fs){
    int player_num = 0;
    char player_path[10];

    for (int i = 0; i < 8; i++)
    {
        sprintf(player_path, "/Villager%d", i);

        if (fs.isDirectory(player_path))
        {
            player_num +=1;
        }
    }

    return player_num;
}

// rc is just in here because I have no clue how the syntax around it works and how I need to use it
bool loadPersonalSave(std::vector<uint8_t> &out_save, tp::VersionParser &out_version, int villager_num, fs::Filesystem &fs, Result &rc) {
    char personal_path[24]; char personal_hdr_path[30];
    sprintf(personal_path, save_personal_path, villager_num);
    sprintf(personal_hdr_path, save_personal_hdr_path, villager_num);

    printf("[LPS] Checking for personal save files at: %s and %s\n", personal_path, personal_hdr_path);

    if (fs.is_file(personal_path) && fs.is_file(personal_hdr_path))
    {   printf("[LPS] Opening personal save files: Villager%d\n", villager_num);
        fs::File personal_file, personal_hdr_file;
        if (rc = fs.openFile(personal_file, personal_path) | fs.openFile(personal_hdr_file, personal_hdr_path); R_FAILED(rc)) {
            fprintf(stderr, "[LPS] Failed to open personal save files: %#x\n", rc);
            return false;
        }

        printf("[LPS] Deriving keys...\n");
        auto [personal_key, personal_ctr] = sv::getKeys(personal_hdr_file);
        printf("[LPS] Decrypting save file...\n");
        out_save = sv::decrypt(personal_file, 0xc00000, personal_key, personal_ctr);
        printf("[LPS] Parsing version...\n");
        out_version = tp::VersionParser(personal_hdr_file);

        printf("[LPS] Closing save files...\n");
        personal_file.close();
        personal_hdr_file.close();
        
        return true;
    }
    else {
        return false;
    }
}

bool createPlayerData(std::pair<tp::PersonalInfoParser, ImTextureID> &out, int player_num, const std::vector<uint8_t> &personal_save, const tp::VersionParser &personal_version) {
    tp::PersonalInfoParser personal_parser; tp::PersonalPhotoParser photo_parser;

    printf("[CPD] Parsing save data: Villager%d\n", player_num);
        
    personal_parser = tp::PersonalInfoParser  (static_cast<tp::Version>(personal_version), personal_save);
    photo_parser    = tp::PersonalPhotoParser (static_cast<tp::Version>(personal_version), personal_save);

    ImTextureID photo_txid;
    if (!gui::createImageFromPersonalSave(photo_txid, player_num + 2, photo_parser.personal_photo.photo_data, photo_parser.personal_photo.photo_size))
    {
        fprintf(stderr, "[CPD] Failed to create image from personal save.\n");
        return false;
    }

    printf("[CPD] Creating PlayerData...\n");
    out = std::make_pair(personal_parser, photo_txid);
    printf("[CPD] Villager%d done!\n", player_num);
    return true;
}

bool checkAppDir() {
    printf("[CAD] Opening sd filesystem...\n");
    FsFileSystem sd_handle;
    auto sd_fs = fs::Filesystem(sd_handle);
    auto rc = sd_fs.openSDMC();

    printf("[CAD] Checking sd app dir...\n");
    if (!sd_fs.isDirectory(sd_app_dir_path)) {
        rc = sd_fs.createDirectory(sd_app_dir_path);
        if (R_FAILED(rc))
        {
            fprintf(stderr, "[CAD] Failed to create app dir: %#x\n", rc);
            return false;
        }
        
        sd_fs.flush();
    }

    printf("[CAD] Checking if islands dir exists...\n");
    if (!sd_fs.isDirectory(sd_app_dir_path)) {
        printf("[CAD] Islands dir does not exist. Creating it now...\n");
        rc = sd_fs.createDirectory(sd_app_island_dir_path);
        if (R_FAILED(rc))
        {
            fprintf(stderr, "[CAD] Failed to create islands dir: %#x\n", rc);
            return false;
        }
        sd_fs.flush();

        rc = sd_fs.createDirectory(sd_app_shared_island_dir_path);
        if (R_FAILED(rc))
        {
            fprintf(stderr, "[CAD] Failed to create shared island dir: %#x\n", rc);
            return false;
        }
        sd_fs.flush();
    }
    else {
        printf("[CAD] Checking if shared island dir exists...\n");
        if (!sd_fs.isDirectory(sd_app_shared_island_dir_path)) {
            rc = sd_fs.createDirectory(sd_app_shared_island_dir_path);
            if (R_FAILED(rc))
            {
                fprintf(stderr, "[CAD] Failed to create shared island dir: %#x\n", rc);
                return false;
            }
            sd_fs.flush();
        }
    }

    return true;
}

int main(int argc, char **argv) {    
    tp::TurnipParser turnip_parser; tp::VisitorParser visitor_parser; tp::DateParser date_parser; tp::WeatherSeedParser seed_parser; tp::IslandInfoParser island_parser;
    std::pair<tp::PersonalInfoParser, ImTextureID> player_data[8]; int player_num = 0; std::vector<uint8_t> player_save[8]; tp::VersionParser player_version[8];
    {
        if (!checkAppDir())
        {
            return 1;
        }        

        printf("Opening save...\n");
        FsFileSystem handle;
        auto rc = fsOpen_DeviceSaveData(&handle, acnh_programid);
        auto fs = fs::Filesystem(handle);
        if (R_FAILED(rc)) {
            fprintf(stderr, "Failed to open save: %#x\n", rc);
            return 1;
        }
        fs::File main_header, main;
        if (rc = fs.openFile(main_header, save_main_hdr_path) | fs.openFile(main, save_main_path); R_FAILED(rc)) {
            fprintf(stderr, "Failed to open main save files: %#x\n", rc);
            return 1;
        }

        printf("Now working on save file: main\n");
        printf("Deriving keys...\n");
        auto [main_key, main_ctr] = sv::getKeys(main_header);
        printf("Decrypting save file...\n");
        auto main_decrypted  = sv::decrypt(main, 0xc00000, main_key, main_ctr);

        printf("Parsing save file...\n");
        auto version_parser = tp::VersionParser(main_header);
        turnip_parser   = tp::TurnipParser      (static_cast<tp::Version>(version_parser), main_decrypted);
        visitor_parser  = tp::VisitorParser     (static_cast<tp::Version>(version_parser), main_decrypted);
        date_parser     = tp::DateParser        (static_cast<tp::Version>(version_parser), main_decrypted);
        seed_parser     = tp::WeatherSeedParser (static_cast<tp::Version>(version_parser), main_decrypted);
        island_parser   = tp::IslandInfoParser  (static_cast<tp::Version>(version_parser), main_decrypted);

        printf("Closing save files...\n");
        main.close();
        main_header.close();

        player_num = getPlayerNumber(fs);

        for (int i = 0; i < player_num; i++)
        {
            if (!loadPersonalSave(player_save[i], player_version[i], i, fs, rc)) {
                fprintf(stderr, "Failed to load personal save: Villager%d\n", i);
                break;
            }
        }
        printf("Loaded %d villagers/(players) from save data!\n", player_num);
    }
    
    if (auto rc = lang::initializeToSystemLanguage(); R_FAILED(rc))
        fprintf(stderr, "Failed to init language: %#x, will fall back to key names\n", rc);

    printf("Starting gui\n");
    if (!gui::init())
        fprintf(stderr, "Failed to init\n");

    auto color_theme = ColorSetId_Dark;
    auto rc = setsysGetColorSetId(&color_theme);
    if (R_FAILED(rc))
        fprintf(stderr, "Failed to get theme id\n");
    if (color_theme == ColorSetId_Light)
        th::applyTheme(th::Theme::Light);
    else
        th::applyTheme(th::Theme::Dark);

    for (int i = 0; i < player_num; i++)
    {
        createPlayerData(player_data[i], i, player_save[i], player_version[i]);
    }  

    auto save_date = date_parser.date;
    // auto save_ts   = date_parser.to_posix();

    while (gui::loop()) {
        u64 ts = 0;
        auto rc = timeGetCurrentTime(TimeType_UserSystemClock, &ts);
        if (R_FAILED(rc))
            fprintf(stderr, "Failed to get timestamp\n");

        TimeCalendarTime           cal_time = {};
        TimeCalendarAdditionalInfo cal_info = {};
        rc = timeToCalendarTimeWithMyRule(ts, &cal_time, &cal_info);
        if (R_FAILED(rc))
            fprintf(stderr, "Failed to convert timestamp\n");

        /* bool is_outdated = (floor(ts / (24 * 60 * 60)) > floor(save_ts / (24 * 60 * 60)) + cal_info.wday)
            && ((cal_info.wday != 0) || (cal_time.hour >= 5)); */

        auto &[width, height] = im::GetIO().DisplaySize;

        im::SetNextWindowFocus();
        im::Begin(std::string("app_name"_lang + ", " + "version"_lang + " " + VERSION + "-" + COMMIT + "###main").c_str(), nullptr,
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove);
        im::SetWindowPos({0.24f * width, 0.16f * height});
        im::SetWindowSize({0.55f * width, 0.73f * height});

        im::BeginTabBar("##tab_bar", ImGuiTabBarFlags_NoTooltip);
        
        gui::drawIslandTab(island_parser, player_data, player_num, seed_parser, save_date);
        
        gui::drawTurnipTab(turnip_parser, cal_time, cal_info);
        gui::drawVisitorTab(visitor_parser, cal_time, cal_info);
        gui::drawLanguageTab();

        im::EndTabBar();

        im::End();

        gui::render();
    }

    gui::exit();

    return 0;
}
