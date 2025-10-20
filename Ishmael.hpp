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
#include <map>
#include <functional>
#include <cstdint>

#include "../Ishmael/logger/logger.hpp"

#include <dpp/dispatcher.h>
#include <dpp/cluster.h>

// Type alias for the function that will handle a command
using command_function = std::function<void(dpp::cluster&, const dpp::slashcommand_t&)>;

// A struct to hold information about a command
struct command_t {
	command_function function;
	std::string description;
	uint64_t permissions;
	bool is_restricted_to_owners; // Restriction to dev guild
	std::vector<dpp::command_option> options;
};

// The global map that stores all commands
// The key is the command's name
extern std::map<std::string, command_t> commands;

// Type alias for a function that handles a select menu click
using select_handler_function = std::function<void(dpp::cluster& bot, const dpp::select_click_t&)>;

// A struct to hold the information of select handlers
struct select_handler_t {
	select_handler_function function;
	uint64_t required_permissions;
};

// The global map that stores all select menu handlers
// Its key is the custom ID (ex. `setup_role_edits_logging_channel`)
extern std::map<std::string, select_handler_t> select_handlers;

// Function to get the global logger instance
Logger& get_logger();