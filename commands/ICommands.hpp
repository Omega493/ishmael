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

#ifndef ICOMMANDS_HPP
#define ICOMMANDS_HPP

#pragma once

// Main registration functions to be called from `Ishmael.cpp`
void register_all_commands();
void register_all_select_handlers();

// Command specific registration functions
void register_ping_command();
void register_stats_command();
void register_role_add_command();

#endif // ICOMMANDS_HPP
