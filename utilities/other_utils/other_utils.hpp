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

#ifndef OTHER_UTILS_HPP
#define OTHER_UTILS_HPP

#pragma once

/*
 * The following includes are performed:
 * #include <string>
 * #include <optional>
 * #include <cstdint>
 * #include <dpp/cluster.h>
 */

#include <pch.hpp>

/*
 * @brief Formats a duration in seconds into a human-readable string (d:h:m:s)
 * @param total_seconds The total number of seconds to format
 * @return A formatted string in the format "Xd Yh Zm Ws"
*/
inline std::string convert_time(uint64_t total_seconds);

enum class CommandType {
	RoleEdit,
	BanEdit,
	Unknown
};

std::optional<uint64_t> get_log_channel(const uint64_t guild_id, const CommandType command_type);
void save_log_channel(const uint64_t guild_id, const uint64_t channel_id, const CommandType command_type);

void load_guild_settings();

// Backup handling
void backup_guild_settings(const std::string& backup_file_path);
void initialize_backups(dpp::cluster& bot);

#endif // OTHER_UTILS_HPP
