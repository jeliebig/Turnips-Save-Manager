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

#pragma once

#include <string>
#include <json.hpp>

namespace lang {

enum class Language {
    English,
    Chinese,
    French,
    Dutch,
    Italian,
    German,
    Spanish,
    Default,
};

const nlohmann::json &getJson();

Language getCurrentLanguage();
Result setLanguage(Language lang);
Result initializeToSystemLanguage();

std::string getString(std::string key, const nlohmann::json &json = getJson());

namespace literals {

inline std::string operator ""_lang(const char *key, size_t size) {
    return getString(std::string(key, size));
}

} // namespace literals

}; // namespace lang
