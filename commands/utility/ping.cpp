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
 * #include <string>
 * #include <format>
 * #include <exception>
 * #include <cstdint>
 * #include <ctime>
 * #include <dpp/dispatcher.h>
 * #include <dpp/discordclient.h>
 * #include <dpp/message.h>
 * #include <dpp/cluster.h>
 * #include <dpp/user.h>
 * #include <dpp/permissions.h>
 * #include <dpp/exception.h>
 * #include <Ishmael.hpp>
 * #include <utilities/secrets/secrets.hpp>
 */

#include <pch.hpp>

static void handle_ping(dpp::cluster& bot, const dpp::slashcommand_t& event) {
	try {
		const dpp::discord_client* shard{ bot.get_shard((event.command.guild_id >> 22) % bot.numshards) };

		std::string gateway_latency_str{ "N/A" };
		if (shard) gateway_latency_str = std::format("`{} ms`", std::to_string(static_cast<int>(shard->websocket_ping * 1000)));

		dpp::embed embed{ dpp::embed()
			.set_colour(232700) // Hex: #038CFC
			.set_title("Pong! :ping_pong:")
			.add_field("Gateway Latency", gateway_latency_str, true)
			.add_field("API Latency", std::format("`{} ms`", std::to_string(static_cast<int>(bot.rest_ping * 1000))), true)
			.set_footer(dpp::embed_footer()
				.set_text(event.command.get_issuing_user().username)
				.set_icon(event.command.get_issuing_user().get_avatar_url()))
			.set_timestamp(time(0)) };
		event.reply(dpp::message(event.command.channel_id, embed).set_flags(dpp::m_ephemeral));
	}
	catch (const dpp::exception& e) {
		Logger::exception(false, "D++ exception thrown in `/ping`: {}", std::string{ e.what() });
		event.reply(dpp::message("An exception was thrown while processing this command. Please inform <@" + std::string{ secrets.at("OWNER_ID") } + "> of this.")
			.set_flags(dpp::m_ephemeral));
	}
	catch (const std::exception& e) {
		Logger::exception(false, "Standard exception thrown in `/ping`: {}", std::string{ e.what() });
		event.reply(dpp::message("An exception was thrown while processing this command. Please inform <@" + std::string(secrets.at("OWNER_ID")) + "> of this.")
			.set_flags(dpp::m_ephemeral));
	}
	catch (...) {
		Logger::exception(false, "Unknown exception thrown in `/ping`");
		event.reply(dpp::message("An exception was thrown while processing this command. Please inform <@" + std::string(secrets.at("OWNER_ID")) + "> of this.")
			.set_flags(dpp::m_ephemeral));
	}
}

void register_ping_command() {
	commands["ping"] = {
		.function = handle_ping,
		.description = "Pong!",
		.permissions = dpp::p_moderate_members,
		.is_restricted_to_owners = false
	};
}