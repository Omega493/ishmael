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
 * #include <vector>
 * #include <string>
 * #include <algorithm>
 * #include <exception>
 * #include <variant>
 * #include <cstdint>
 * #include <dpp/appcommand.h>
 * #include <dpp/cache.h>
 * #include <dpp/cluster.h>
 * #include <dpp/message.h>
 * #include <dpp/dispatcher.h>
 * #include <dpp/permissions.h>
 * #include <dpp/exception.h>
 * #include <dpp/guild.h>
 * #include <dpp/restresults.h>
 * #include <dpp/role.h>
 * #include <dpp/channel.h>
 * #include <dpp/snowflake.h>
 * #include <Ishmael.hpp>
 * #include <utilities/secrets/secrets.hpp>
 * #include <commands/moderation/mod_utils.hpp>
 * #include <commands/ICommands.hpp>
 * #include <utilities/console_utils/console_utils.hpp>
 */

#include <pch.hpp>

static void handle_role_log_select(dpp::cluster& bot, const dpp::select_click_t& event) {
	event.co_thinking(true);

	const uint64_t channel_id{ std::stoull(event.values[0]) };
	const uint64_t guild_id{ event.command.guild_id };
	try {
		save_log_channel(guild_id, channel_id, CommandType::RoleEdit);
		event.co_edit_original_response(dpp::message{ "Log channel set to <#" + std::to_string(channel_id) + ">. Please run your command again." }.set_flags(dpp::m_ephemeral));
	}
	catch (const dpp::exception& e) {
		const std::string error_msg{ "Failed to save log channel to JSON: " + std::string{ e.what() } };
		try {
			Logger::exception(false, error_msg);
		}
		catch (std::exception& log_e) {
			std::cerr << ConsoleColour::Red << error_msg << ConsoleColour::Reset << std::endl;
			std::cerr << ConsoleColour::Red << "Failed to get the logger: " + std::string{ log_e.what() } << ConsoleColour::Reset << std::endl;

		}
		event.co_edit_original_response(dpp::message{ "An error occurred while saving your selection. Please inform <@" + secrets.at("OWNER_ID") + "> of this." }.set_flags(dpp::m_ephemeral));
	}
}

static void handle_role_add(dpp::cluster& bot, const dpp::slashcommand_t& event) {
	try {
		event.co_thinking(true);

		const dpp::guild* g{ dpp::find_guild(event.command.get_guild().id) };
		if (!g) {
			event.co_edit_original_response(dpp::message{ "Error: Couldn't retrieve the server information." }.set_flags(dpp::m_ephemeral));
			return;
		}

		// Find the role from the guild's cache using its ID
		const dpp::role* role_to_add{ dpp::find_role(std::get<dpp::snowflake>(event.get_parameter("role"))) };
		if (!role_to_add) {
			event.co_edit_original_response(dpp::message("Error: Specific role couldn't be found on this server.").set_flags(dpp::m_ephemeral));
			return;
		}

		// Get the snowflake (ID) of the user from the command parameter
		const dpp::snowflake target_by_id{ [&event] {
			auto user_param{ event.get_parameter("user") };
			// std::holds_alternative<T> checks whether the variable currently hold a value of type T
			if (std::holds_alternative<dpp::snowflake>(user_param)) return std::get<dpp::snowflake>(user_param);
			else return event.command.get_issuing_user().id;
		}()};
		
		bot.guild_get_member(g->id, target_by_id,
			[&bot, event, g, role_to_add, target_by_id](const dpp::confirmation_callback_t& target_callback) {
				if (target_callback.is_error()) {
					event.co_edit_original_response(dpp::message{ "Error: The user is not a member of this server." }.set_flags(dpp::m_ephemeral));
					return;
				}

				const dpp::guild_member target_user{ target_callback.get<dpp::guild_member>() };
				const dpp::guild_member issuer_member{ event.command.member };

				if (issuer_member.user_id == 0) {
					event.co_edit_original_response(dpp::message("Error: Could not retrieve your member information.").set_flags(dpp::m_ephemeral));
					return;
				}

				const dpp::permission issuer_perms{ calculate_permissions(issuer_member) };

				if (!(issuer_member.is_guild_owner() || issuer_perms & dpp::p_administrator || issuer_perms & dpp::p_moderate_members)) {
					event.co_edit_original_response(dpp::message{ "You don't have permission to use this command." }.set_flags(dpp::m_ephemeral));
					return;
				}

				if (role_to_add->is_managed()) {
					event.co_edit_original_response(dpp::message{ "Error: This role is managed by an integration and cannot be assigned manually." }.set_flags(dpp::m_ephemeral));
					return;
				}

				if (role_to_add->has_administrator()) {
					event.co_edit_original_response(dpp::message{ "For security reasons, roles with `Administrator` permission can't be assigned with this command." }
						.set_flags(dpp::m_ephemeral));
					return;
				}

				if (role_to_add->id == g->id) {
					event.co_edit_original_response(dpp::message{ "Error: Everyone inherently possesses the `@everyone` role. It can't be added." }.set_flags(dpp::m_ephemeral));
					return;
				}

				const dpp::guild_member bot_member{ dpp::find_guild_member(g->id, bot.me.id) };
				const dpp::permission bot_perms{ calculate_permissions(bot_member) };

				// Check whether the bot has all the permissions of the role to be assigned
				if (!(bot_perms & dpp::p_administrator)) {
					// Bot is not admin, so check if it has all perms of the role
					if ((bot_perms & role_to_add->permissions) != role_to_add->permissions) {
						event.co_edit_original_response(dpp::message{ "I can't assign this role as I lack some of its permissions." }.set_flags(dpp::m_ephemeral));
						return;
					}
				}

				// Check the bot's role heirarchy
				if (get_highest_role_position(bot_member) <= role_to_add->position) {
					event.co_edit_original_response(dpp::message{ "I can't assign this role as it is higher than or equal to my own highest role." }
						.set_flags(dpp::m_ephemeral));
					return;
				}

				// The server owner bypasses role heirarchy and permission checks
				if (!issuer_member.is_guild_owner()) {
					// User's highest role must be higher than the role to be added
					if (get_highest_role_position(issuer_member) <= role_to_add->position) {
						event.co_edit_original_response(dpp::message{ "You can't assign a role that is higher than or equal to your own highest role." }
							.set_flags(dpp::m_ephemeral));
						return;
					}

					if (!(issuer_perms & dpp::p_administrator)) {
						if ((issuer_perms & role_to_add->permissions) != role_to_add->permissions) {
							event.co_edit_original_response(dpp::message{ "You can't assign a role that has permissions you don't possess." }.set_flags(dpp::m_ephemeral));
							return;
						}
					}
				}

				// Check if the target already has the role
				const auto& target_roles{ target_user.get_roles() };
				if (std::find(target_roles.begin(), target_roles.end(), role_to_add->id) != target_roles.end()) {
					event.co_edit_original_response(dpp::message{ "User <@" + std::to_string(target_by_id) + "> already has the role <@&" + std::to_string(role_to_add->id) + ">." }
						.set_flags(dpp::m_ephemeral));
					return;
				}

				// Before the role is added, check the existence of the logging channel
				const auto log_channel_id_opt{ get_log_channel(g->id, CommandType::RoleEdit) };

				if (!log_channel_id_opt.has_value()) {
					// Logging channel isn't set. We stop and prompt the user
					if (!(issuer_member.is_guild_owner() || issuer_perms & dpp::p_administrator || issuer_perms & dpp::p_manage_guild)) {
						event.co_edit_original_response(dpp::message{ "Error: You cannot add this role as a role edits logging channel isn't set up. Please ask an administrator to set one." }
							.set_flags(dpp::m_ephemeral));
						return;
					}

					dpp::message msg{ "Before you can add this role, a logging channel must be set up." };
					msg.set_flags(dpp::m_ephemeral);

					// Create the select menu
					dpp::component select_menu{ dpp::component()
						.set_type(dpp::cot_channel_selectmenu)
						.set_placeholder("Select a channel for role logs")
						.set_id("setup_role_log_channel") };

					// Add a filter to only show text channels
					select_menu.add_channel_type(dpp::channel_type::CHANNEL_TEXT);

					msg.add_component_v2(dpp::component().add_component_v2(select_menu));
					event.co_edit_original_response(msg);
					return;
				}

				// Add the role
				bot.guild_member_add_role(g->id, target_by_id, role_to_add->id,
					[&bot, event, role_to_add, target_by_id, target_user, issuer_member](const dpp::confirmation_callback_t& add_role_callback) {
						if (add_role_callback.is_error()) {
							Logger::error(false, "Failed to add role: {}", add_role_callback.get_error().message);
							event.co_edit_original_response(dpp::message{ "An error occured while trying to add the role. Please inform <@" + std::string{secrets.at("OWNER_ID")} + "> of this." });
							return;
						}

						event.co_edit_original_response(dpp::message{ "Successfully added the <@&" + std::to_string(role_to_add->id) + "> role to <@" + std::to_string(target_by_id) + ">!" }
							.set_flags(dpp::m_ephemeral));

						send_audit_log(bot, event, CommandType::RoleEdit, 3265892, "Role Added", target_user, issuer_member, role_to_add, get_reason_from_event(event)); // 3265892 = Hex: #31D564
					}
				);
			});
	}
	catch (const dpp::exception& e) {
		Logger::exception(false, "D++ exception thrown in `/role_add`: {}", std::string(e.what()));
		event.co_edit_original_response(dpp::message("An exception was thrown while processing this command. Please inform <@" + std::string(secrets.at("OWNER_ID")) + "> of this.")
			.set_flags(dpp::m_ephemeral));
	}
	catch (const std::exception& e) {
		Logger::exception(false, "Standard exception thrown in `/role_add`: {}", std::string(e.what()));
		event.co_edit_original_response(dpp::message("An exception was thrown while processing this command. Please inform <@" + std::string(secrets.at("OWNER_ID")) + "> of this.")
			.set_flags(dpp::m_ephemeral));
	}
	catch (...) {
		Logger::exception(false, "Unknown exception thrown in `/role_add`");
		event.co_edit_original_response(dpp::message("An exception was thrown while processing this command. Please inform <@" + std::string(secrets.at("OWNER_ID")) + "> of this.")
			.set_flags(dpp::m_ephemeral));
	}
}

void register_role_add_command() {
	commands["role_add"] = {
		.function = handle_role_add,
		.description = "Adds a role to a user.",
		.permissions = dpp::p_moderate_members, // Base permission
		.is_restricted_to_owners = false, // Not restricted to dev guild
		.options = {
			dpp::command_option(dpp::co_role, "role", "The role to add", true),
			dpp::command_option(dpp::co_user, "user", "The user to add the role to (defaults to you)", false),
			dpp::command_option(dpp::co_string, "reason", "The reason", false)
		}
	};

	select_handlers["setup_role_log_channel"] = {
		.function = handle_role_log_select,
		.required_permissions = dpp::p_manage_guild
	};
}
