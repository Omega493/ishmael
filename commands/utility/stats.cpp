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
 * #include <chrono>
 * #include <exception>
 * #include <cstdint>
 * #include <ctime>
 * #include <dpp/cache.h>
 * #include <dpp/cluster.h>
 * #include <dpp/dispatcher.h>
 * #include <dpp/message.h>
 * #include <dpp/exception.h>
 * #include <dpp/version.h>
 * #include <Ishmael.hpp>
 * #include <utilities/other_utils/other_utils.hpp>
 */

#include <pch.hpp>

extern std::chrono::steady_clock::time_point session_start_time;

static void handle_stats(dpp::cluster& bot, const dpp::slashcommand_t& event) {
	try {
		dpp::embed embed{ dpp::embed()
			.set_colour(10298559) // Hex: #9D24BF
			.set_title("Bot Stats")
			.set_thumbnail(bot.me.get_avatar_url())
			.add_field("Uptime", convert_time((std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - session_start_time)).count()), true)
			.add_field("Servers", std::to_string(dpp::get_guild_count()), true)
			.add_field("Users", std::to_string(dpp::get_user_count()), true)
			.add_field("API Latency", std::format("`{} ms`", static_cast<int>(bot.rest_ping * 1000)), true)
			.add_field("Shard", std::format("{} / {}", (event.command.guild_id >> 22) % bot.numshards, bot.numshards), true)
			.add_field("Bot Version", "1.2.0", true)
			.add_field("D++ Version", DPP_VERSION_TEXT, true)
			.set_footer(dpp::embed_footer()
				.set_text(event.command.get_issuing_user().username)
				.set_icon(event.command.get_issuing_user().get_avatar_url()))
			.set_timestamp(time(0)) };
		event.reply(dpp::message(event.command.channel_id, embed).set_flags(dpp::m_ephemeral));
	}
	catch (const dpp::exception& e) {
		Logger::exception(false, "D++ exception thrown in `/stats`: {}", std::string{ e.what() });
		event.reply(dpp::message("An exception was thrown while processing this command.").set_flags(dpp::m_ephemeral));
	}
	catch (const std::exception& e) {
		Logger::exception(false, "Standard exception thrown in `/stats`: {}", std::string{ e.what() });
		event.reply(dpp::message("An exception was thrown while processing this command.").set_flags(dpp::m_ephemeral));
	}
	catch (...) {
		Logger::exception(false, "Unknown exception thrown in `/stats`");
		event.reply(dpp::message("An exception was thrown while processing this command.").set_flags(dpp::m_ephemeral));
	}
}

void register_stats_command() {
	commands["stats"] = {
		.function = handle_stats,
		.description = "Display the current statistics of the bot",
		.permissions = dpp::p_manage_guild,
		.is_restricted_to_owners = true
	};
}