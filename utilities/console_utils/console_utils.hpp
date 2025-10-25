/*
* Copyright (C) 2025 Omega493

* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.

* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.f

* You should have received a copy of the GNU General Public License
* along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include <string>

namespace ConsoleColour {
    const std::string Reset = "\033[0m"; // Reset text colour
    const std::string Red = "\033[31m"; // Used for exceptions / errors
    const std::string Green = "\033[32m"; // Used for success-category messages
    const std::string Yellow = "\033[33m"; // Used for warnings
    const std::string Cyan = "\033[36m"; // Used for info
}

#ifdef _WIN32
void setup_console();
#endif