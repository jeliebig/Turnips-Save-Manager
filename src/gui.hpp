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

#pragma once

#include <cstdint>
#include <array>
#include <string>
#include <imgui.h>
#include <switch.h>

#include "parser.hpp"

namespace im {
    using namespace ImGui;
} // namespace im

namespace gui {

bool init();
bool loop();
void render();
void exit();

bool createBackground(const std::string &path);
bool createImageFromPersonalSave(ImTextureID &out_txid, const std::uint32_t pp_image_id, unsigned char* photo_data, const std::uint32_t photo_size);

void drawTurnipTab(const tp::TurnipParser &parser, const TimeCalendarTime &cal_time, const TimeCalendarAdditionalInfo &cal_info);
void drawVisitorTab(const tp::VisitorParser &parser, const TimeCalendarTime &cal_time, const TimeCalendarAdditionalInfo &cal_info);
// void draw_weather_tab(const tp::WeatherSeedParser &parser);
void drawLanguageTab();
void drawIslandTab(const tp::IslandInfoParser &island_parser, const std::pair<tp::PersonalInfoParser, ImTextureID>* personal_data, const size_t player_num , const tp::WeatherSeedParser &weather_parser, const tp::Date &save_date);
//void draw_photo_test_tab(const tp::PersonalPhotoParser& photo_parser);

template <typename F>
void do_with_color(std::uint32_t col, F f) {
    im::PushStyleColor(ImGuiCol_Text, col);
    f();
    im::PopStyleColor();
}

} // namespace gui
