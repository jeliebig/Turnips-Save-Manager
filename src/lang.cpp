// Copyright (C) 2021 jeliebig
// Copyright (C) 2020 averne
//
// This file is part of Turnips-Save-Manager (This is a modified version of Turnips).
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

#include <switch.h>
#include <json.hpp>

#include "fs.hpp"
#include "lang.hpp"

using json = nlohmann::json;

namespace lang {

namespace {

static json lang_json = nullptr;
static Language current_language = Language::Default;

} // namespace

const json &getJson() {
    return lang_json;
}

Language getCurrentLanguage() {
    return current_language;
}

Result setLanguage(Language lang) {
    const char *path;
    current_language = lang;
    switch (lang) {
        case Language::Chinese:
            path = "romfs:/lang/ch.json";
            break;
        case Language::French:
            path = "romfs:/lang/fr.json";
            break;
        case Language::Dutch:
            path = "romfs:/lang/nl.json";
            break;
        case Language::Italian:
            path = "romfs:/lang/it.json";
            break;
        case Language::German:
            path = "romfs:/lang/de.json";
            break;
        case Language::Spanish:
            path = "romfs:/lang/es.json";
            break;
        case Language::English:
        case Language::Default:
        default:
            path = "romfs:/lang/en.json";
            break;
    }

    auto *fp = fopen(path, "r");
    if (!fp)
        return 1;

    fseek(fp, 0, SEEK_END);
    std::size_t size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    std::string contents(size, 0);

    if (auto read = fread(contents.data(), 1, size, fp); read != size)
        return read;
    fclose(fp);

    lang_json = json::parse(contents);

    return 0;
}

Result initializeToSystemLanguage() {
    if (auto rc = setInitialize(); R_FAILED(rc)) {
        setExit();
        return rc;
    }

    u64 l;
    if (auto rc = setGetSystemLanguage(&l); R_FAILED(rc)) {
        setExit();
        return rc;
    }

    SetLanguage sl;
    if (auto rc = setMakeLanguage(l, &sl); R_FAILED(rc)) {
        setExit();
        return rc;
    }

    switch (sl) {
        case SetLanguage_ENGB:
        case SetLanguage_ENUS:
            return setLanguage(Language::English);
        case SetLanguage_ZHCN:
        case SetLanguage_ZHHANS:
            return setLanguage(Language::Chinese);
        case SetLanguage_FR:
            return setLanguage(Language::French);
        case SetLanguage_NL:
            return setLanguage(Language::Dutch);
        case SetLanguage_IT:
            return setLanguage(Language::Italian);
        case SetLanguage_DE:
            return setLanguage(Language::German);
        case SetLanguage_ES:
            return setLanguage(Language::Spanish);
        default:
            return setLanguage(Language::Default);
    }
}

std::string getString(std::string key, const json &json) {
    return json.value(key, key);
}

} // namespace lang
