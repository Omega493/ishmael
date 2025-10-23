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
#include <ctime>
#include <exception>
#include <cstdint>
#include <variant>
#include <optional>
#include <type_traits>

#include "../Ishmael/commands/moderation/mod_utils.hpp"
#include "../Ishmael/Ishmael.hpp"
#include "../Ishmael/utilities/logger/logger.hpp"
#include "../Ishmael/utilities/other_utils/other_utils.hpp"

#include <dpp/appcommand.h>
#include <dpp/cache.h>
#include <dpp/cluster.h>
#include <dpp/message.h>
#include <dpp/dispatcher.h>
#include <dpp/permissions.h>
#include <dpp/exception.h>
#include <dpp/guild.h>
#include <dpp/role.h>
#include <dpp/snowflake.h>

dpp::permission calculate_permissions(const dpp::guild_member& member) {
	// Find the guild from cache
	dpp::guild* g = dpp::find_guild(member.guild_id);

	// If guild isn't cached, we can't get @everyone role.
	// We have to fallback to this logic, even thought it may be inaccurate
	if (!g) {
		dpp::permission perms;
		for (const dpp::snowflake role_id : member.get_roles()) {
			dpp::role* r = dpp::find_role(role_id);
			if (r) {
				perms |= r->permissions;
			}
		}
		return perms;
	}

	// If the guild is cached, we start with the @everyone role's perms
	// The @everyone role always has the same ID as the guild.
	dpp::role* everyone_role = dpp::find_role(g->id);
	// If the @everyone role is found, we start from its perms, else we start with 0
	dpp::permission perms = everyone_role ? everyone_role->permissions : dpp::permission(0);

	// Now we OR all other roles the member has, as
	// member.get_roles() does not include the @everyone role.
	for (const dpp::snowflake role_id : member.get_roles()) {
		dpp::role* r = dpp::find_role(role_id);
		if (r) {
			perms |= r->permissions;
		}
	}
	return perms;
}

uint16_t get_highest_role_position(const dpp::guild_member& member) {
	uint16_t highest_pos = 0;
	for (const auto& role_id : member.get_roles()) {
		dpp::role* r = dpp::find_role(role_id);
		if (r && r->position > highest_pos) {
			highest_pos = r->position;
		}
	}
	return highest_pos;
}

std::string get_reason_from_event(const dpp::slashcommand_t& event) {
	auto reason_param = event.get_parameter("reason");
	if (std::holds_alternative<std::string>(reason_param)) {
		return std::get<std::string>(reason_param);
	}
	return "No reason provided.";
}

void send_audit_log(dpp::cluster& bot, const dpp::slashcommand_t& event, CommandType command_type, uint64_t colour,
	const std::string& title, const dpp::guild_member& target_user, const dpp::guild_member& issuer_member,
	AuditLogTarget target_obj, const std::string& reason) {
	try {
		auto log_channel_id_opt = get_log_channel(event.command.guild_id, command_type);

		if (!log_channel_id_opt.has_value()) return;

		const dpp::snowflake log_channel_id = log_channel_id_opt.value();

		std::string event_type = [command_type]() -> std::string {
			switch (command_type) {
			case CommandType::RoleEdit: return "Role";
			case CommandType::BanEdit: return "Ban";
			default: return "Unknown";
			}
		}();

		dpp::embed log_embed = dpp::embed()
			.set_color(colour)
			.set_title(title)
			.set_thumbnail(target_user.get_avatar_url())
			.add_field("Target User", target_user.get_mention(), true)
			.add_field("Moderator", issuer_member.get_mention(), true);

		// We visit the target_obj variant and the lambda handles the type
		std::visit([&log_embed](auto&& arg) {
			// gett the type of the object inside the variant
			using T = std::decay_t<decltype(arg)>;
			if constexpr (std::is_same_v<T, const dpp::role*>) {
				log_embed.add_field("Role", arg->get_mention(), false);
			}
			else if constexpr (std::is_same_v<T, std::monostate>) {
				// It's monostate, we add no field
			}
		}, target_obj);

		// Now we add other fields
		log_embed.add_field("Reason", reason, false)
			.set_footer(dpp::embed_footer()
				.set_text(event.command.get_issuing_user().username)
				.set_icon(event.command.get_issuing_user().get_avatar_url()))
			.set_timestamp(time(0));

		bot.message_create(dpp::message(log_channel_id, log_embed));
	}
	catch (const dpp::exception& e) {
		get_logger().log(LogLevel::Exception, "D++ exception thrown while trying to send audit log embed: " + std::string(e.what()), false);
	}
	catch (const std::exception& e) {
		get_logger().log(LogLevel::Exception, "Standard exception thrown while trying to send audit log embed: `" + std::string(e.what()), false);
	}
	catch (...) {
		get_logger().log(LogLevel::Exception, "Unknown exception thrown while trying to send audit log embed", false);
	}
}