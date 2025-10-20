/*
* Copyright (C) 2025 Omega493

* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.

* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.

* You should have received a copy of the GNU General Public License
* along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include <string>
#include <cstdint>
#include <format>

#include "../Ishmael/utilities/utilities.hpp"

std::string convert_time(uint64_t total_seconds) {
    uint64_t days = total_seconds / 86400;
    total_seconds %= 86400;

    uint64_t hours = total_seconds / 3600;
    total_seconds %= 3600;

    uint64_t minutes = total_seconds / 60;
    uint64_t seconds = total_seconds % 60;

    return std::format("{}d {}h {}m {}s", days, hours, minutes, seconds);
}