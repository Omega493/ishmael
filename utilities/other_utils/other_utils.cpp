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

#include <corecrt.h>
#include <string>
#include <ctime>
#include <cstdint>
#include <fstream>
#include <optional>
#include <filesystem>
#include <chrono>
#include <mutex>
#include <functional>
#include <format>
#include <memory>

#include "other_utils.hpp"
#include "../Ishmael/Ishmael.hpp"
#include "../Ishmael/utilities/logger/logger.hpp"

#include <dpp/cluster.h>
#include <dpp/timer.h>
#include <dpp/nlohmann/json.hpp>
#include <dpp/nlohmann/json_fwd.hpp>

std::string convert_time(uint64_t total_seconds) {
    uint64_t days = total_seconds / 86400;
    total_seconds %= 86400;

    uint64_t hours = total_seconds / 3600;
    total_seconds %= 3600;

    uint64_t minutes = total_seconds / 60;
    uint64_t seconds = total_seconds % 60;

    return std::format("{}d {}h {}m {}s", days, hours, minutes, seconds);
}

using json = nlohmann::json;

static json guild_settings_json;
static std::mutex settings_mutex;
static const std::string guild_settings_file_path = "data/guild_settings.json";

static void backup_guild_settings(const std::string& backup_file_path) {
    std::scoped_lock lock(settings_mutex);
    try {
        std::filesystem::path path(backup_file_path);
        if (path.has_parent_path()) std::filesystem::create_directories(path.parent_path());

        std::ofstream backup_file(backup_file_path);
        if (!backup_file.is_open()) {
            get_logger().log(LogLevel::Critical, "Couldn't open `" + backup_file_path + "` for writing", true);
            return;
        }
        backup_file << guild_settings_json.dump(4);
    }
    catch (const std::filesystem::filesystem_error& e) {
        get_logger().log(LogLevel::Exception, "File system exception while creating backup directory for `" + backup_file_path + "`: " + e.what(), true);
    }
}

void load_guild_settings() {
	std::scoped_lock lock(settings_mutex);
	std::ifstream file(guild_settings_file_path);
	if (!file.is_open()) {
		get_logger().log(LogLevel::Info, guild_settings_file_path + " not found, creating a new one", true);

		std::filesystem::create_directory("data");

		// File doesn't exist so initialize with empty JSON object
		guild_settings_json = json::object();
		lock.~scoped_lock();
		write_guild_settings_to_file(); // Create empty file
	}
	else {
		try {
			file >> guild_settings_json;
			get_logger().log(LogLevel::Info, "Guild settings loaded from " + guild_settings_file_path, true);
		}
		catch (json::parse_error& e) {
			get_logger().log(LogLevel::Exception, "Exception thrown while parsing " + guild_settings_file_path + ": " + e.what() + ". Initialized empty settings.", true);
			guild_settings_json = json::object(); // Use empty settings to prevent crash
		}
	}
}

void initialize_backups(dpp::cluster& bot) {
	get_logger().log(LogLevel::Info, "Performing initial backups on startup", true);
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
				get_logger().log(LogLevel::Info, "Performing recurring backup for: " + path, true);
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

				get_logger().log(LogLevel::Info, "Performing first scheduled backup for: " + path, true);
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

	get_logger().log(LogLevel::Info, "Scheduling backups", true);

	get_logger().log(LogLevel::Info, "Next hourly backup (backup-hourly) in " + convert_time(seconds_to_next_hour) + " seconds", true);
	
	get_logger().log(LogLevel::Info, "Next 12-hour backup (backup-per-12h) in " + convert_time(seconds_to_next_12_hr_mark) + " seconds.", true);

	get_logger().log(LogLevel::Info, "Next 00:00 UTC backup (backup-daily) in " + convert_time(seconds_to_next_midnight) + " seconds.", true);

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
		get_logger().log(LogLevel::Critical, "Couldn't open " + guild_settings_file_path + " for writing", true);
		return;
	}
	// Write with an indent of 4 spaces
	file << json_data;
	file.close();

	// Create backups for every write
	backup_guild_settings("backups/backup-instant-1/guild_settings.json");
	backup_guild_settings("backups/backup-instant-2/guild_settings.json");
}

std::optional<uint64_t> get_log_channel(uint64_t guild_id, CommandType command_type) {
	std::scoped_lock lock(settings_mutex);
	std::string guild_id_str = std::to_string(guild_id);

	if (guild_settings_json.contains(guild_id_str)) { // Check if the Guild ID exists in our JSON
		std::string channel_type = [command_type]() -> std::string {
			switch (command_type) {
			case CommandType::RoleEdit: return "role_edit_logging_channel_id";
			case CommandType::BanEdit: return "ban_edit_logging_channel_id";
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
		case CommandType::BanEdit: return "ban_edit_logging_channel_id";
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