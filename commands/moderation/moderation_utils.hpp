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

#pragma once

#include <string>
#include <cstdint>
#include <optional>
#include <variant>

#include <dpp/cluster.h>
#include <dpp/dispatcher.h>
#include <dpp/guild.h>
#include <dpp/permissions.h>
#include <dpp/role.h>

enum class CommandType {
	RoleEdit,
	Ban,
	Unknown
};

// For the JSON
void load_guild_settings();
void write_guild_settings_to_file();
std::optional<uint64_t> get_log_channel(uint64_t guild_id, CommandType command_type);
void save_log_channel(uint64_t guild_id, uint64_t channel_id, CommandType command_type);

// Backup handling
void backup_guild_settings(const std::string& backup_file_path);
void initialize_backups(dpp::cluster& bot);

// Utility functions
dpp::permission calculate_permissions(const dpp::guild_member& member);
uint16_t get_highest_role_position(const dpp::guild_member& member);
std::string get_reason_from_event(const dpp::slashcommand_t& event);

// For the audit log embeds
using AuditLogTarget = std::variant<std::monostate, const dpp::role*>; // TODO: Add other datatypes here
void send_audit_log(dpp::cluster& bot, const dpp::slashcommand_t& event, CommandType command_type, uint64_t colour,
	const std::string& title, const dpp::guild_member& target_user, const dpp::guild_member& issuer_member,
	AuditLogTarget target_obj, const std::string& reason);