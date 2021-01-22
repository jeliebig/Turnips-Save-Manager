// This file was modified by jeliebig in 2021.
//
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
#include <algorithm>
#include <array>
#include <vector>
#include <utility>

#include <switch.h>

#include "fs.hpp"
#include "sead.hpp"

namespace sv {

// From NHSE
static std::array<std::uint8_t, 0x10> getParam(const std::vector<std::uint32_t> &crypt_data, std::size_t idx) {
    auto sead = sead::Random(crypt_data[crypt_data[idx] & 0x7f]);
    auto roll_count = (crypt_data[crypt_data[idx + 1] & 0x7f] & 0xf) + 1;

    for (std::uint32_t i = 0; i < roll_count; ++i)
        sead.getU64();

    std::array<std::uint8_t, 0x10> res;
    for (std::size_t i = 0; i < res.size(); i++)
        res[i] = sead.getU32() >> 24;
    return res;
}

static std::pair<std::array<std::uint8_t, 0x10>, std::array<std::uint8_t, 0x10>> getKeys(fs::File &header) {
    constexpr std::size_t crypt_data_size = 0x200;

    std::vector<std::uint32_t> crypt_data(0x200, 0);
    if (auto read = header.read(crypt_data.data(), crypt_data.size(), 0x100); read != crypt_data_size)
        printf("Failed to read header encryption data (got %#lx bytes, expected %#lx)\n", read, crypt_data_size);

    auto key = getParam(crypt_data, 0);
    auto ctr = getParam(crypt_data, 2);
    return {std::move(key), std::move(ctr)};
}

static std::vector<std::uint8_t> decrypt(fs::File &main, std::size_t size,
        const std::array<std::uint8_t, 0x10> &key, const std::array<std::uint8_t, 0x10> ctr) {
    Aes128CtrContext ctx;
    aes128CtrContextCreate(&ctx, key.data(), ctr.data());

    std::size_t offset = 0, read = 0, buf_size = std::clamp(size, 0x1000ul, 0x80000ul);
    std::vector<std::uint8_t> buf(buf_size, 0);
    std::vector<std::uint8_t> res(size, 0);

    while (offset < size) {
        read = main.read(buf.data(), buf.size(), offset);
        aes128CtrCrypt(&ctx, &res[offset], buf.data(), read);
        offset += read;
        if (read != buf.size())
            break;
    }

    return res;
}

} // namespace sv
