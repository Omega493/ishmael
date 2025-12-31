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

#ifndef CONSOLE_UTILS_HPP
#define CONSOLE_UTILS_HPP

#pragma once

/*
 * The following include is performed:
 * #include <string>
 */

#include <pch.hpp>

inline std::atomic_bool console_setup_success{ false }; // Flag for ConsoleColour

namespace ConsoleColour {
    inline std::string Reset{}; // Reset text colour
    inline std::string Red{}; // Used for exceptions / errors
    inline std::string Green{}; // Used for success-category messages
    inline std::string Yellow{}; // Used for warnings
    inline std::string Cyan{}; // Used for info
}

#endif // CONSOLE_UTILS_HPP
