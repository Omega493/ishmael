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

#include <iostream>
#include <chrono>
#include <thread>
#include <string>
#include <map>
#include <atomic>
#include <exception>
#include <stdexcept>
#include <future>
#include <sstream>
#include <variant>

#include "../Ishmael/Ishmael.hpp"
#include "../Ishmael/commands/ICommands.hpp"
#include "../Ishmael/logger/logger.hpp"
#include "../Ishmael/secrets/secrets.hpp"
#include "../Ishmael/commands/moderation/moderation_utils.hpp"
#include "../Ishmael/utilities/utilities.hpp"

#include <dpp/snowflake.h>
#include <dpp/cluster.h>
#include <dpp/dispatcher.h>
#include <dpp/exception.h>
#include <dpp/appcommand.h>
#include <dpp/once.h>
#include <dpp/message.h>
#include <dpp/misc-enum.h>
#include <dpp/presence.h>
#include <dpp/restresults.h>
#include <dpp/permissions.h>

static Logger* g_current_logger = nullptr;
Logger& get_logger() {
	if (!g_current_logger) throw std::runtime_error("Logger accessed before it was initialized for the current session");
	return *g_current_logger;
}

std::chrono::steady_clock::time_point session_start_time;

std::map<std::string, command_t> commands;
std::map<std::string, select_handler_t> select_handlers;

// Global flag and pointer to signal the main loop
static std::atomic_bool shutting_down = false;
static dpp::cluster* bot_ptr = nullptr;
static std::thread shutdown_thread;

static void initiate_shutdown() {
	if (!bot_ptr) return;

	const std::string shutdown_init_msg = "Shutdown signal received. Cleaning up commands.";
	get_logger().log(LogLevel::Warn, shutdown_init_msg);
	std::cout << shutdown_init_msg << std::endl;

	std::promise<void> global_delete_promise, guild_delete_promise;
	std::future<void> global_delete_future = global_delete_promise.get_future();
	std::future<void> guild_delete_future = guild_delete_promise.get_future();

	// Set presence to DND
	dpp::presence shutdown_presence(dpp::ps_dnd, dpp::at_listening, "shutdown signal");
	bot_ptr->set_presence(shutdown_presence);

	// Before trying to delete guild commands, first check if any are even registered
	bool are_guild_commands_registered = false;
	for (const auto& pair : commands) {
		if (pair.second.is_restricted_to_owners) {
			are_guild_commands_registered = true;
			break;
		}
	}
	if (are_guild_commands_registered) {
		try {
			dpp::snowflake dev_guild_id(secrets.at("DEV_GUILD_ID"));
			bot_ptr->guild_bulk_command_delete(dev_guild_id, [&](const dpp::confirmation_callback_t& callback) {
				if (callback.is_error()) {
					const std::string fail_bulk_delete_guild_commands = "Failed to bulk delete guild commands: " + callback.get_error().message;
					get_logger().log(LogLevel::Error, fail_bulk_delete_guild_commands);
					std::cerr << fail_bulk_delete_guild_commands << std::endl;
				}
				else {
					const std::string success_bulk_delete_guild_commands = "Successfully bulk deleted guild commands.";
					get_logger().log(LogLevel::Info, success_bulk_delete_guild_commands);
					std::cout << success_bulk_delete_guild_commands << std::endl;
				}
				guild_delete_promise.set_value();
				});
		}
		catch (const std::exception& e) {
			const std::string exception_bulk_delete_guild_commands = "Exception during guild command deletion: " + std::string(e.what());
			get_logger().log(LogLevel::Exception, exception_bulk_delete_guild_commands);
			std::cerr << exception_bulk_delete_guild_commands << std::endl;
			guild_delete_promise.set_value();
		}
	}
	else {
		const std::string skip_bulk_delete_guild_commands = "Skipped deleting guild commands as none were registered.";
		get_logger().log(LogLevel::Info, skip_bulk_delete_guild_commands);
		std::cout << skip_bulk_delete_guild_commands << std::endl;
		guild_delete_promise.set_value();
	}

	// Delete global commands
	bot_ptr->global_bulk_command_delete([&](const dpp::confirmation_callback_t& callback) {
		if (callback.is_error()) {
			get_logger().log(LogLevel::Error, "Failed to bulk delete global commands: " + callback.get_error().message);
		}
		else {
			get_logger().log(LogLevel::Info, "Successfully bulk deleted all global commands.");
		}
		global_delete_promise.set_value();
		});

	get_logger().log(LogLevel::Info, "Waiting for command deletions to complete");
	global_delete_future.wait();
	guild_delete_future.wait();
	get_logger().log(LogLevel::Info, "Command cleanups finished.");

	// Now that cleanup is done, we can safely shut down the cluster
	std::this_thread::sleep_for(std::chrono::seconds(2));
	bot_ptr->shutdown();
	std::this_thread::sleep_for(std::chrono::seconds(2));
}

static void shutdown_signal_handler(int signum) {
	// shutting_down.exchange() to ensure this runs only once
	if (shutting_down.exchange(true)) {
		return;
	}
	// Offload the blocking cleanup work to a new thread
	// so the signal handler can return immediately
	shutdown_thread = std::thread(initiate_shutdown); // Assign to the global thread
}

#ifdef _WIN32
static BOOL WINAPI CtrlHandler(DWORD FdwCtrlType) {
	switch (FdwCtrlType) {
	case CTRL_C_EVENT:
	case CTRL_CLOSE_EVENT:
	case CTRL_BREAK_EVENT:
		shutdown_signal_handler(0);
		return TRUE;
	default:
		return FALSE;
	}
}
#endif

int main() {
	/*
	* One Time Setup
	* This outer try-catch handles the `secrets` map
	* If decryption fails, the bot can't run
	*/
	try {
		/*
		* Accessing the global 'secrets' map for the first time triggers the
		* decryption process, which will prompt for the hex key
		*/
		if (secrets.find("BOT_TOKEN") == secrets.end()) throw std::runtime_error("BOT_TOKEN not found in decrypted secrets");
		if (secrets.find("DEV_GUILD_ID") == secrets.end()) throw std::runtime_error("DEV_GUILD_ID not found in decrypted secrets");

		std::cout << "Secrets loaded successfully!" << std::endl;
	}
	catch (const std::exception& e) {
		std::cerr << "Fatal exception before startup: " << e.what() << std::endl;
		return 1;
	}
	catch (...) {
		std::cerr << "Unknown exception occured" << std::endl;
		return 1;
	}
	/*
	* Bot Restart Loop
	* Should any exception be thrown, the bot would automatically try to recover itself
	* by restarting 10 seconds after the exception throw
	*/

	// A shared_ptr outside the loop to manage logger lifetime.
	std::shared_ptr<Logger> session_logger_ptr;
	while (!shutting_down) {
		/*
		* Create a new `Logger` object on the stack for this session
		* A new log file would be created. When this loop ineration ends,
		* the object is automatically destroyed, closing the log file.
		*/
		session_logger_ptr = std::make_shared<Logger>();
		g_current_logger = session_logger_ptr.get(); // Update the global pointer
		const std::string BOT_TOKEN = secrets.at("BOT_TOKEN");

		dpp::cluster bot(BOT_TOKEN);
		bot_ptr = &bot; // Assign the bot instance to the global ptr

		try {
			std::cout << "Ishmael session starting" << std::endl;
			get_logger().log(LogLevel::Info, "Ishmael session starting");

			session_start_time = std::chrono::steady_clock::now(); // dpp::discord_client::get_uptime() probably could also be used

		#ifdef _WIN32
			if (!SetConsoleCtrlHandler(CtrlHandler, TRUE)) {
				const std::string win_ctrl_handler_failure = "Couldn't set Windows control handler";
				get_logger().log(LogLevel::Critical, win_ctrl_handler_failure);
				std::cerr << win_ctrl_handler_failure << std::endl;
				return 1;
			}
		#else
			signal(SIGINT, shutdown_signal_handler);
			signal(SIGTERM, shutdown_signal_handler);
		#endif

			bot.on_log([&](const dpp::log_t& event) {
				if (event.severity == dpp::ll_warning) get_logger().log(LogLevel::Warn, event.message);
				if (event.severity == dpp::ll_error) get_logger().log(LogLevel::Error, event.message);
				if (event.severity == dpp::ll_critical) get_logger().log(LogLevel::Critical, event.message);
				});

			// Load guild settings from JSON file
			load_guild_settings();
			// Populate the `commands` map by calling all registration functions
			register_all_commands();
			// Populate the `select_handlers` map by calling all registration functions
			register_all_select_handlers();

			// This event is fired when a user uses a slash command
			bot.on_slashcommand([&bot](const dpp::slashcommand_t& event) {

				// Get the name of the command used
				std::string command_name = event.command.get_command_name();

				// Find the command in our map
				auto it = commands.find(command_name);

				if (it != commands.end()) it->second.function(bot, event);
				else {
					event.reply(dpp::message("Unknown command").set_flags(dpp::m_ephemeral));
					get_logger().log(LogLevel::Warn, "Received an unknown command: " + command_name);
				}
				});

			bot.on_select_click([&bot](const dpp::select_click_t& event) {
				std::string custom_id = event.custom_id;
				auto it = select_handlers.find(custom_id);

				if (it != select_handlers.end()) {
					// Found a handler
					const auto& handler = it->second;

					// Check permissions
					dpp::permission issuer_perms = calculate_permissions(event.command.member);
					bool is_owner = event.command.member.is_guild_owner();
					bool is_admin = issuer_perms.has(dpp::p_administrator);
					bool has_required_perms = (issuer_perms & handler.required_permissions) == handler.required_permissions;

					if (!is_owner && !is_admin && !has_required_perms) {
						event.reply(dpp::message("You don't have the required permissions to use this menu.").set_flags(dpp::m_ephemeral));
						return;
					}

					// Run the specific function
					handler.function(bot, event);
				}
				// If no handler is found, we simply ignore the click
				});

			bot.on_ready([&bot](const dpp::ready_t& event) {

				if (dpp::run_once<struct register_bot_commands>()) {
					for (const auto& pair : commands) {
						dpp::slashcommand cmd(pair.first, pair.second.description, bot.me.id);

						for (const auto& opt : pair.second.options) {
							cmd.add_option(opt);
						}

						cmd.set_default_permissions(pair.second.permissions);
						cmd.set_dm_permission(false);

						// If a command is restricted to only owners, create the command in their server
						if (pair.second.is_restricted_to_owners) {
							dpp::snowflake dev_guild_id(secrets.at("DEV_GUILD_ID"));
							bot.guild_command_create(cmd, dev_guild_id);
						}
						// Else, it is a global command
						else {
							bot.global_command_create(cmd);
						}
					}
				}

				if (dpp::run_once<struct initialize_backup_once>()) {
					initialize_backups(bot);
				}

				// Log remaining connections on session startup
				if (dpp::run_once<struct log_connections>()) {
					bot.get_gateway_bot([&bot](const dpp::confirmation_callback_t& gateway_callback) {
						if (gateway_callback.is_error()) {
							const std::string msg = "Failed to get gateway details: " + gateway_callback.get_error().message;
							get_logger().log(LogLevel::Error, msg);
							std::cout << msg << std::endl;
							return;
						}

						try {
							dpp::gateway gw = gateway_callback.get<dpp::gateway>();

							// Build a single string to prevent race conditions in the output
							std::stringstream ss;
							ss << "Gateway Details:\n"
								<< "  - Session Start Max Concurrency: " << gw.session_start_max_concurrency << "\n"
								<< "  - Sessions Remaining: " << gw.session_start_remaining << "/" << gw.session_start_total << "\n"
								<< "  - Session Start Reset After: " << convert_time(gw.session_start_reset_after / 1000);

							std::string gateway_info = ss.str();
							get_logger().log(LogLevel::Info, gateway_info);
							std::cout << gateway_info << std::endl;

						}
						catch (const std::bad_variant_access& e) {
							const std::string msg = "Bad variant access on gateway callback: " + std::string(e.what());
							get_logger().log(LogLevel::Error, msg);
							std::cerr << msg << std::endl;
						}
						});
				}

				// Sets the bot's status. Here, "Listening to birds!"
				dpp::presence birds(dpp::ps_online, dpp::at_listening, "birds!");
				bot.set_presence(birds);

				std::cout << "Logged in as " << bot.me.username << std::endl;
				get_logger().log(LogLevel::Info, "Logged in as " + bot.me.username);
				});

			bot.start(dpp::st_wait);
		}
		catch (const dpp::exception& e) {
			get_logger().log(LogLevel::Exception, "D++ Exception: " + std::string(e.what()));
			std::cerr << "D++ Exception: " << e.what() << std::endl;
		}
		catch (const std::exception& e) {
			get_logger().log(LogLevel::Exception, "Standard Exception: " + std::string(e.what()));
			std::cerr << "Standard Exception: " << e.what() << std::endl;
		}
		catch (...) {
			get_logger().log(LogLevel::Exception, "Unknown exception thrown");
			std::cerr << "Unknown exception thrown" << std::endl;
		}

		// bot.start() has returned, so the bot is disconnected
		get_logger().log(LogLevel::Warn, "Bot session ended");
		std::cout << "Bot session ended" << std::endl;

		// If `shutting_down` is false, the bot crashed
		// We are to wait 10 seconds before trying to reconnect
		if (!shutting_down) {
			g_current_logger = nullptr; // Invalidate the pointer

			std::cout << "Reconnecting in 10 seconds..." << std::endl;
			std::this_thread::sleep_for(std::chrono::seconds(10));
		}
	}
	// Wait for the shutdown thread to finish its work before exiting
	if (shutdown_thread.joinable()) {
		shutdown_thread.join();
	}
	g_current_logger = nullptr;
	std::cout << "The bot has shut down" << std::endl;
	return 0;
}