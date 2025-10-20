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
#include <string>
#include <ctime>
#include <exception>
#include <cstdint>
#include <variant>
#include <fstream>
#include <optional>
#include <type_traits>
#include <filesystem>
#include <chrono>
#include <mutex>
#include <functional>

#include "../Ishmael/commands/moderation/moderation_utils.hpp"
#include "../Ishmael/Ishmael.hpp"
#include "../Ishmael/logger/logger.hpp"
#include "../Ishmael/utilities/utilities.hpp"

#include <dpp/appcommand.h>
#include <dpp/cache.h>
#include <dpp/cluster.h>
#include <dpp/message.h>
#include <dpp/dispatcher.h>
#include <dpp/permissions.h>
#include <dpp/exception.h>
#include <dpp/guild.h>
#include <dpp/role.h>
#include <dpp/timer.h>
#include <dpp/snowflake.h>
#include <dpp/nlohmann/json.hpp>
#include <dpp/nlohmann/json_fwd.hpp>

using json = nlohmann::json;

static json guild_settings_json;
static std::mutex settings_mutex;
static const std::string guild_settings_file_path = "data/guild_settings.json";

void backup_guild_settings(const std::string& backup_file_path) {
	std::scoped_lock lock(settings_mutex);
	try {
		std::filesystem::path path(backup_file_path);
		if (path.has_parent_path()) std::filesystem::create_directories(path.parent_path());

		std::ofstream backup_file(backup_file_path);
		if (!backup_file.is_open()) {
			const std::string msg = "Couldn't open " + backup_file_path + " for writing";
			get_logger().log(LogLevel::Critical, msg);
			std::cerr << msg << std::endl;
			return;
		}

		backup_file << guild_settings_json.dump(4);
	}
	catch (const std::filesystem::filesystem_error& e) {
		const std::string msg = "File system exception while creating backup directory for: " + backup_file_path + ": " + e.what();
		get_logger().log(LogLevel::Exception, msg);
		std::cerr << msg << std::endl;
	}
}

void load_guild_settings() {
	std::scoped_lock lock(settings_mutex);
	std::ifstream file(guild_settings_file_path);
	if (!file.is_open()) {
		const std::string msg = guild_settings_file_path + " not found, creating a new one";
		get_logger().log(LogLevel::Info, msg);
		std::cout << msg << std::endl;

		std::filesystem::create_directory("data");
		// File doesn't exist so initialize with empty JSON object
		guild_settings_json = json::object();
		lock.~scoped_lock();
		write_guild_settings_to_file(); // Create empty file
	}
	else {
		try {
			file >> guild_settings_json;
			const std::string msg = "Guild settings loaded from " + guild_settings_file_path;
			get_logger().log(LogLevel::Info, msg);
			std::cout << msg << std::endl;
		}
		catch (json::parse_error& e) {
			const std::string msg = "Exception thrown while parsing " + guild_settings_file_path + ": " + e.what() + ". Initialized empty settings.";
			get_logger().log(LogLevel::Exception, msg);
			std::cerr << msg << std::endl;
			guild_settings_json = json::object(); // Use empty settings to prevent crash
		}
	}
}

void initialize_backups(dpp::cluster& bot) {
	const std::string msg = "Performing initial backups on startup";
	get_logger().log(LogLevel::Info, msg);
	std::cout << msg << std::endl;
	backup_guild_settings("backups/backup-daily/guild_settings.json");
	backup_guild_settings("backups/backup-per-12h/guild_settings.json");
	backup_guild_settings("backups/backup-hourly/guild_settings.json");

	/*
	* Schedule future backups
	* Helper lambda to schedule a recurring backup
	* It starts a one-shot timer. Whenever it fires, backups are performed
	* and a new repeating timer is started for all future backups
	*/
	auto schedule_recurring_backup = [&](const std::string& path, uint64_t initial_delay_s, uint64_t repeat_interval_s) {

		// Use shared_ptr to manage the callback's lifetime
		auto recurring_callback_ptr = std::make_shared<std::function<void(dpp::timer)>>();

		// Use a weak_ptr inside the lambda to avoid a reference-counting cycle
		std::weak_ptr<std::function<void(dpp::timer)>> weak_callback_ptr = recurring_callback_ptr;

		*recurring_callback_ptr =
			[&bot, path, repeat_interval_s, weak_callback_ptr](dpp::timer handle) {

			const std::string recurring_msg = "Performing recurring backup for: " + path;
			get_logger().log(LogLevel::Info, recurring_msg);
			std::cout << recurring_msg << std::endl;
			backup_guild_settings(path);

			// Reschedule this same callback to run again
			// Lock the weak_ptr to get a shared_ptr. If it still exists, reschedule.
			if (auto locked_callback_ptr = weak_callback_ptr.lock()) {
				bot.start_timer(*locked_callback_ptr, repeat_interval_s);
			}
		};

		bot.start_timer(
			[&bot, path, repeat_interval_s, recurring_callback_ptr](dpp::timer one_shot_timer) {

				bot.stop_timer(one_shot_timer);

				const std::string msg = "Performing first scheduled backup for: " + path;
				get_logger().log(LogLevel::Info, msg);
				std::cout << msg << std::endl;
				backup_guild_settings(path);

				// Start the recurring timer
				bot.start_timer(*recurring_callback_ptr, repeat_interval_s);
			},
			initial_delay_s // Initial delay
			);
		};

	const auto now = std::chrono::system_clock::now();
	std::time_t now_t = std::chrono::system_clock::to_time_t(now);
	std::tm current_utc_time;
#ifdef _WIN32
	gmtime_s(&current_utc_time, &now_t);
#else
	gmtime_r(&now_t, &current_utc_time);
#endif

	constexpr uint64_t one_hour = 60 * 60;
	constexpr uint64_t one_day = one_hour * 24;
	constexpr uint64_t twelve_hours = one_day / 2;

	// Hourly backup (backup-hourly)
	uint64_t seconds_past_hour = current_utc_time.tm_min * 60 + current_utc_time.tm_sec;
	uint64_t seconds_to_next_hour = one_hour - (seconds_past_hour % one_hour);

	// 00:00 UTC backup (backup-daily)
	uint64_t seconds_past_midnight = current_utc_time.tm_hour * one_hour + current_utc_time.tm_min * 60 + current_utc_time.tm_sec;
	uint64_t seconds_to_next_midnight = one_day - (seconds_past_midnight % one_day);

	// 00:00 UTC & 12:00 UTC backup (backup-per-12h)
	uint64_t seconds_to_next_12_hr_mark = twelve_hours - (seconds_past_midnight % twelve_hours);

	const std::string schedule_backup_msg = "Scheduling backups";
	get_logger().log(LogLevel::Info, schedule_backup_msg);
	std::cout << schedule_backup_msg << std::endl;

	const std::string hourly_backup_msg = "Next hourly backup (backup-hourly) in " + convert_time(seconds_to_next_hour) + " seconds";
	get_logger().log(LogLevel::Info, hourly_backup_msg);
	std::cout << hourly_backup_msg << std::endl;

	const std::string per_12_hour_backup_msg = "Next 12-hour backup (backup-per-12h) in " + convert_time(seconds_to_next_12_hr_mark) + " seconds.";
	get_logger().log(LogLevel::Info, per_12_hour_backup_msg);
	std::cout << per_12_hour_backup_msg << std::endl;

	const std::string daily_backup_msg = "Next 00:00 UTC backup (backup-daily) in " + convert_time(seconds_to_next_midnight) + " seconds.";
	get_logger().log(LogLevel::Info, daily_backup_msg);
	std::cout << daily_backup_msg << std::endl;

	// Perform the scheduled backups
	schedule_recurring_backup("backups/backup-hourly/guild_settings.json", seconds_to_next_hour, one_hour);
	schedule_recurring_backup("backups/backup-per-12h/guild_settings.json", seconds_to_next_12_hr_mark, twelve_hours);
	schedule_recurring_backup("backups/backup-daily/guild_settings.json", seconds_to_next_midnight, one_day);
}

void write_guild_settings_to_file() {
	// Dump the JSON to a string while under lock, then release the lock
	// This avoids holding the mutex during slow file I/O operations.
	std::string json_data;
	{
		std::scoped_lock lock(settings_mutex);
		json_data = guild_settings_json.dump(4);
	}

	std::ofstream file(guild_settings_file_path);
	if (!file.is_open()) {
		const std::string msg = "Couldn't open " + guild_settings_file_path + " for writing";
		get_logger().log(LogLevel::Critical, msg);
		std::cerr << msg << std::endl;
		return;
	}
	// Write with an indent of 4 spaces
	file << json_data;
	file.close();

	// Create backups for every write
	backup_guild_settings("backups/backup-instant-1/guild_settings.json");
	backup_guild_settings("backups/backup-instant-2/guild_settings.json");
}

// Get a log channel
std::optional<uint64_t> get_log_channel(uint64_t guild_id, CommandType command_type) {
	std::scoped_lock lock(settings_mutex);
	std::string guild_id_str = std::to_string(guild_id);

	if (guild_settings_json.contains(guild_id_str)) { // Check if the Guild ID exists in our JSON
		std::string channel_type = [command_type]() -> std::string {
			switch (command_type) {
			case CommandType::RoleEdit: return "role_edit_logging_channel_id";
			case CommandType::Ban: return "ban_edit_logging_channel_id";
			default: return "general_logging_channel_id";
			}
			}();

		if (guild_settings_json[guild_id_str].contains(channel_type)) { // Check if the specific key exists in our JSON
			return guild_settings_json[guild_id_str][channel_type].get<uint64_t>();
		}
	}
	return std::nullopt; // Not found
}

void save_log_channel(uint64_t guild_id, uint64_t channel_id, CommandType command_type) {
	std::string guild_id_str = std::to_string(guild_id);

	std::string channel_type = [command_type]() -> std::string {
		switch (command_type) {
		case CommandType::RoleEdit: return "role_edit_logging_channel_id";
		case CommandType::Ban: return "ban_edit_logging_channel_id";
		default: return "general_logging_channel_id";
		}
		}();

	// Update the in-memory object
	{
		std::scoped_lock lock(settings_mutex);
		guild_settings_json[guild_id_str][channel_type] = channel_id;
	}

	// Save the changes to file
	write_guild_settings_to_file();
}

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

		if (!log_channel_id_opt.has_value()) {
			get_logger().log(LogLevel::Warn, "No role log channel set for guild " + std::to_string(event.command.guild_id));
			return;
		}

		const dpp::snowflake log_channel_id = log_channel_id_opt.value();

		std::string event_type = [command_type]() -> std::string {
			switch (command_type) {
			case CommandType::RoleEdit: return "Role";
			case CommandType::Ban: return "Ban";
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
		get_logger().log(LogLevel::Exception, "D++ exception thrown while trying to send audit log embed: " + std::string(e.what()));
	}
	catch (const std::exception& e) {
		get_logger().log(LogLevel::Exception, "Standard exception thrown while trying to send audit log embed: `" + std::string(e.what()));
	}
	catch (...) {
		get_logger().log(LogLevel::Exception, "Unknown exception thrown while trying to send audit log embed");
	}
}