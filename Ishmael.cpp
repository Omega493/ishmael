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

/*
 * The following includes are performed:
 * #include <iostream>
 * #include <sstream>
 * #include <unordered_map>
 * #include <chrono>
 * #include <thread>
 * #include <string>
 * #include <atomic>
 * #include <exception>
 * #include <stdexcept>
 * #include <future>
 * #include <variant>
 * #include <memory>
 * #include <mutex>
 * #include <cstdlib>
 * #include <dpp/snowflake.h>
 * #include <dpp/cluster.h>
 * #include <dpp/dispatcher.h>
 * #include <dpp/exception.h>
 * #include <dpp/appcommand.h>
 * #include <dpp/once.h>
 * #include <dpp/message.h>
 * #include <dpp/misc-enum.h>
 * #include <dpp/presence.h>
 * #include <dpp/restresults.h>
 * #include <dpp/permissions.h>
 * #include <Ishmael.hpp>
 * #include <ICommands.hpp>
 * #include <logger/logger.hpp>
 * #include <secrets/secrets.hpp>
 * #include <moderation/mod_utils.hpp>
 * #include <other_utils/other_utils.hpp>
 * #include <console_utils/console_utils.hpp>
 * #include <exception/exception.hpp>
 */

#include <pch.hpp>

std::chrono::steady_clock::time_point session_start_time;

std::unordered_map<std::string, command_t> commands;
std::unordered_map<std::string, select_handler_t> select_handlers;

// Global flag and pointer to signal the main loop
static std::atomic_bool shutting_down{ false };
static dpp::cluster* bot_ptr{ nullptr };
static std::thread shutdown_thread;

static void initiate_shutdown() {
	if (!bot_ptr) return;

	Logger::warn(true, "Shutdown signal received. Cleaning up commands.");

	std::promise<void> global_delete_promise, guild_delete_promise;
	std::future<void> global_delete_future{ global_delete_promise.get_future() }, guild_delete_future{ guild_delete_promise.get_future() };

	// Set presence to DND
	bot_ptr->set_presence(dpp::presence{ dpp::ps_dnd, dpp::at_listening, "shutdown signal" });

	// Before trying to delete guild commands, first check if any are even registered
	const std::atomic_bool are_guild_commands_registered{ []() -> bool {
		for (const auto& pair : commands) if (pair.second.is_restricted_to_owners) return true;
		return false;
	}()};
	
	if (are_guild_commands_registered.load()) {
		try {
			bot_ptr->guild_bulk_command_delete(dpp::snowflake{ secrets.at("DEV_GUILD_ID") }, [&](const dpp::confirmation_callback_t& callback) {
				if (callback.is_error()) Logger::exception(true, "Failed to bulk delete guild commands: {}", callback.get_error().message);
				else Logger::info(true, "Successfully bulk deleted guild commands.");
				guild_delete_promise.set_value();
			});
		}
		catch (const std::exception& e) {
			Logger::exception(true, "Exception during guild command deletion: {}", std::string{ e.what() });
			guild_delete_promise.set_value();
		}
	}
	else {
		Logger::info(true, "Skipped deleting guild commands as none were registered.");
		guild_delete_promise.set_value();
	}

	// Delete global commands
	bot_ptr->global_bulk_command_delete([&](const dpp::confirmation_callback_t& callback) {
		if (callback.is_error()) Logger::error(true, "Failed to bulk delete global commands: {}", callback.get_error().message);
		else Logger::info(true, "Successfully bulk deleted all global commands.");
		global_delete_promise.set_value();
	});

	Logger::info(true, "Waiting for command deletions to complete");
	global_delete_future.wait();
	guild_delete_future.wait();
	Logger::info(true, "Command cleanups finished!");

	// Now that cleanup is done, we can safely shut down the cluster
	std::this_thread::sleep_for(std::chrono::seconds{ 2 });
	bot_ptr->shutdown();
	std::this_thread::sleep_for(std::chrono::seconds{ 2 });
}

static void shutdown_signal_handler(const int signum) {
	// shutting_down.exchange() to ensure this runs only once
	if (shutting_down.exchange(true)) return;
	// Offload the blocking cleanup work to a new thread
	// so the signal handler can return immediately
	shutdown_thread = std::thread{ initiate_shutdown }; // Assign to the global thread
}

#ifdef _WIN32
static BOOL WINAPI CtrlHandler(const DWORD FdwCtrlType) {
	switch (FdwCtrlType) {
	case CTRL_C_EVENT:
	case CTRL_BREAK_EVENT:
	case CTRL_CLOSE_EVENT:
		shutdown_signal_handler(0);
		return TRUE;
	default:
		return FALSE;
	}
}
#endif // _WIN32

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
		Logger::info("main() startup");

		if (secrets.find("BOT_TOKEN") == secrets.end()) throw std::runtime_error("BOT_TOKEN not found in decrypted secrets");
		if (secrets.find("DEV_GUILD_ID") == secrets.end()) throw std::runtime_error("DEV_GUILD_ID not found in decrypted secrets");
		if (secrets.find("OWNER_ID") == secrets.end()) throw std::runtime_error("OWNER_ID not found in decrypted secrets");
		if (const int rc{ std::system(std::string{ "where 7z > nul 2>&1" }.c_str()) })
			throw std::runtime_error("7-Zip not found in system path, `where 7z` return code: " + std::to_string(rc));

		Logger::success("Secrets loaded successfully!");
	}
	catch (const std::exception& e) {
		Logger::exception("Exception before startup: " + std::string{ e.what() });
		return EXIT_FAILURE;
	}
	catch (...) {
		Logger::exception("Unknown exception before startup");
		return EXIT_FAILURE;
	}

	int exit_code{ EXIT_SUCCESS };

	/*
	* Bot Restart Loop
	* Should any exception be thrown, the bot would automatically try to recover itself
	* by restarting 10 seconds after the exception throw
	*/
	while (!shutting_down.load()) {
		dpp::cluster bot{ secrets.at("BOT_TOKEN") };
		bot_ptr = &bot; // Assign the bot instance to the global ptr

		try {
			Logger::info(true, "Ishmael session starting");
			session_start_time = std::chrono::steady_clock::now(); // dpp::discord_client::get_uptime() probably could also be used

#ifdef _WIN32
			if (!SetConsoleCtrlHandler(CtrlHandler, TRUE)) {
				Logger::error(true, "Couldn't set Windows CTRL handler");
				throw ControlHandlerError("Couldn't set Windows CTRL handler");
			}
#else // ^^^ _WIN32 || !_WIN32
			signal(SIGINT, shutdown_signal_handler);
			signal(SIGTERM, shutdown_signal_handler);
#endif // _WIN32

			bot.on_log([&](const dpp::log_t& event) {
				if (event.severity == dpp::ll_warning) Logger::warn(false, event.message);
				if (event.severity == dpp::ll_error) Logger::error(false, event.message);
				if (event.severity == dpp::ll_critical) Logger::error(false, event.message);
			});

			load_guild_settings();
			register_all_commands();
			register_all_select_handlers();

			// This event is fired when a user uses a slash command
			bot.on_slashcommand([&bot](const dpp::slashcommand_t& event) {
				const std::string command_name{ event.command.get_command_name() };

				auto it{ commands.find(command_name) };

				if (it != commands.end()) it->second.function(bot, event);
				else {
					event.reply(dpp::message("Unknown command").set_flags(dpp::m_ephemeral));
					Logger::warn(false, "Received an unknown command: {}", command_name);
				}
			});

			bot.on_select_click([&bot](const dpp::select_click_t& event) {
				if (auto it{ select_handlers.find(event.custom_id) }; it != select_handlers.end()) {
					// Found a handler
					const auto& handler{ it->second };

					// Check permissions
					const dpp::permission issuer_perms{ calculate_permissions(event.command.member) };
					const bool is_owner{ event.command.member.is_guild_owner() };
					const bool is_admin{ issuer_perms.has(dpp::p_administrator) };
					const bool has_required_perms{ (issuer_perms & handler.required_permissions) == handler.required_permissions };

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
						dpp::slashcommand cmd{ pair.first, pair.second.description, bot.me.id };

						for (const auto& opt : pair.second.options) cmd.add_option(opt);

						cmd.set_default_permissions(pair.second.permissions);
						cmd.set_dm_permission(false);

						// If a command is restricted to only owners, create the command in their server
						if (pair.second.is_restricted_to_owners) bot.guild_command_create(cmd, dpp::snowflake{ secrets.at("DEV_GUILD_ID") });
						// Else, it is a global command
						else bot.global_command_create(cmd);
					}
				}

				if (dpp::run_once<struct initialize_backup_once>()) initialize_backups(bot);

				// Log remaining connections on session startup
				if (dpp::run_once<struct log_connections>()) {
					bot.get_gateway_bot([&bot](const dpp::confirmation_callback_t& gateway_callback) {
						if (gateway_callback.is_error()) {
							Logger::error(true, "Failed to get gateway details: {}", gateway_callback.get_error().message);
							return;
						}

						try {
							const dpp::gateway gw{ gateway_callback.get<dpp::gateway>() };

							// Build a single string to prevent race conditions in the output
							std::stringstream ss{};
							ss << "Gateway Details:\n"
								<< "  - Session Start Max Concurrency: " << gw.session_start_max_concurrency << "\n"
								<< "  - Sessions Remaining: " << gw.session_start_remaining << "/" << gw.session_start_total << "\n"
								<< "  - Session Start Reset After: " << convert_time(gw.session_start_reset_after / 1000);

							Logger::info(true, ss.str());
						}
						catch (const std::bad_variant_access& e) {
							Logger::exception(true, "Bad variant access on gateway callback: {}", std::string{ e.what() });
						}
					});
				}

				// Sets the bot's status. Here, "Listening to birds!"
				bot.set_presence(dpp::presence{ dpp::ps_online, dpp::at_listening, "birds!" });

				Logger::info(true, "Logged in as {}", bot.me.format_username());
			});
			bot.start(dpp::st_wait);
		}
		catch (const FatalError& e) {
			exit_code = EXIT_FAILURE;
			try {
				Logger::exception(true, "Unrecoverable error: {}", e.what());
				Logger::exception(true, "Bot will not restart. Fix the environment and start the service manually.");
			}
			catch (const std::exception& log_e) {
				std::cerr << ConsoleColour::Red << "Unrecoverable error: " << e.what() << ConsoleColour::Reset << std::endl;
				std::cerr << ConsoleColour::Red << "Logger failed during exception handling: " << log_e.what() << ConsoleColour::Reset << std::endl;
				std::cerr << ConsoleColour::Red << "Bot will not restart. Fix the environment and start the service manually." << ConsoleColour::Reset << std::endl;
			}
			break; // Break the `while(!shutting_down.load())` loop
		}
		catch (const dpp::exception& e) {
			exit_code = EXIT_FAILURE;
			try {
				Logger::exception(true, "D++ Exception: {}", std::string{ e.what() });
			}
			catch (const std::exception& log_e) {
				std::cerr << ConsoleColour::Red << "D++ Exception: " << e.what() << ConsoleColour::Reset << std::endl;
				std::cerr << ConsoleColour::Red << "Logger failed during exception handling: " << log_e.what() << ConsoleColour::Reset << std::endl;
			}
		}
		catch (const std::exception& e) {
			exit_code = EXIT_FAILURE;
			try {
				Logger::exception(true, "Standard Exception: {}", std::string{ e.what() });
			}
			catch (const std::exception& log_e) {
				std::cerr << ConsoleColour::Red << "Standard Exception: " + std::string{ e.what() } << ConsoleColour::Reset << std::endl;
				std::cerr << ConsoleColour::Red << "Logger failed during exception handling: " + std::string{ log_e.what() } << ConsoleColour::Reset << std::endl;
			}
		}
		catch (...) {
			exit_code = EXIT_FAILURE;
			try {
				Logger::exception(true, "Unknown exception thrown");
			}
			catch (const std::exception& log_e) {
				std::cerr << ConsoleColour::Red << "Unknown exception thrown" << ConsoleColour::Reset << std::endl;
				std::cerr << ConsoleColour::Red << "Logger failed during exception handling: " + std::string{ log_e.what() } << ConsoleColour::Reset << std::endl;
			}
		}

		Logger::warn(true, "Bot session ended");

		// If `shutting_down` is false, the bot crashed
		// We are to wait 10 seconds before trying to reconnect
		if (!shutting_down.load()) {
			Logger::warn(true, "Reconnecting in 10 seconds");
			std::this_thread::sleep_for(std::chrono::seconds{ 10 });
#ifdef _WIN32
			if (std::system("cls")) Logger::error("'cls' failed");
#else // ^^^ _WIN32 || !_WIN32
			if (std::system("clear")) Logger::error("'clear' failed");
#endif // _WIN32
		}
	}

	// Wait for the shutdown thread to finish its work before exiting
	if (shutdown_thread.joinable()) shutdown_thread.join();
	Logger::info(true, "Bot has shutdown");
	return exit_code;
}
