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
 * #include <corecrt.h>
 * #include <fstream>
 * #include <string>
 * #include <optional>
 * #include <filesystem>
 * #include <chrono>
 * #include <mutex>
 * #include <functional>
 * #include <format>
 * #include <memory>
 * #include <ctime>
 * #include <cstdint>
 * #include <dpp/cluster.h>
 * #include <dpp/timer.h>
 * #include <dpp/nlohmann/json.hpp>
 * #include <dpp/nlohmann/json_fwd.hpp>
 * #include <other_utils.hpp>
 * #include <Ishmael.hpp>
 * #include <utilities/logger/logger.hpp>
 */

#include <pch.hpp>

using json = nlohmann::json;

static json guild_settings_json;
static std::mutex settings_mutex;
static const std::string guild_settings_file_path{ "data/guild_settings.json" };

inline std::string convert_time(uint64_t total_seconds) {
	const uint64_t days{ total_seconds / 86400 };
    total_seconds %= 86400;

	const uint64_t hours{ total_seconds / 3600 };
    total_seconds %= 3600;

	const uint64_t minutes{ total_seconds / 60 };
	const uint64_t seconds{ total_seconds % 60 };

    return std::format("{}d {}h {}m {}s", days, hours, minutes, seconds);
}

std::optional<uint64_t> get_log_channel(const uint64_t guild_id, const CommandType command_type) {
	std::scoped_lock lock{ settings_mutex };
	const std::string guild_id_str{ std::to_string(guild_id) };

	if (guild_settings_json.contains(guild_id_str)) { // Check if the Guild ID exists in our JSON
		const std::string channel_type{ [command_type]() -> std::string {
			switch (command_type) {
			case CommandType::RoleEdit: return "role_edit_logging_channel_id";
			case CommandType::BanEdit: return "ban_edit_logging_channel_id";
			default: return "general_logging_channel_id";
			}
		}() };

		if (guild_settings_json[guild_id_str].contains(channel_type)) { // Check if the specific key exists in our JSON
			return guild_settings_json[guild_id_str][channel_type].get<uint64_t>();
		}
	}
	return std::nullopt; // Not found
}

static void write_guild_settings() {
	std::string json_data{};
	{
		std::scoped_lock lock{ settings_mutex };
		json_data = guild_settings_json.dump(4);
	}

	std::ofstream file{ guild_settings_file_path };
	if (!file.is_open()) {
		Logger::exception(true, "Couldn't open {} for writing", guild_settings_file_path);
		return;
	}

	file << json_data;
	file.close();

	backup_guild_settings("backups/backup-instant-1/guild_settings.json");
	backup_guild_settings("backups/backup-instant-2/guild_settings.json");
}

void save_log_channel(const uint64_t guild_id, const uint64_t channel_id, const CommandType command_type) {
	std::string guild_id_str{ std::to_string(guild_id) };

	const std::string channel_type{ [command_type]() -> std::string {
		switch (command_type) {
		case CommandType::RoleEdit: return "role_edit_logging_channel_id";
		case CommandType::BanEdit: return "ban_edit_logging_channel_id";
		default: return "general_logging_channel_id";
		}
	}() };

	// Update the in-memory object
	{
		std::scoped_lock lock{ settings_mutex };
		guild_settings_json[guild_id_str][channel_type] = channel_id;
	}

	write_guild_settings();
}

void load_guild_settings() {
	std::unique_lock lock{ settings_mutex };
	std::ifstream file{ guild_settings_file_path };

	if (!file.is_open()) {
		Logger::info(true, "{} not found, creating a new one", guild_settings_file_path);

		try {
			std::filesystem::create_directory("data");
		}
		catch (const std::filesystem::filesystem_error& e) {
			Logger::exception(true, "Failed to create data directory: {}", e.what());
		}

		guild_settings_json = json::object();
		lock.unlock();
		write_guild_settings();
	}
	else {
		try {
			file >> guild_settings_json;
			Logger::info(true, "Guild settings loaded from {}", guild_settings_file_path);
		}
		catch (const json::parse_error& e) {
			Logger::exception(true, "Exception thrown while parsing {}: {}. Initialized empty settings.", guild_settings_file_path, e.what());
			guild_settings_json = json::object();
		}
	}
}

void backup_guild_settings(const std::string& backup_file_path) {
	std::scoped_lock lock{ settings_mutex };
	try {
		const std::filesystem::path path{ backup_file_path };
		if (path.has_parent_path()) { std::filesystem::create_directories(path.parent_path()); }

		std::ofstream backup_file{ backup_file_path };
		if (!backup_file.is_open()) {
			Logger::error(true, "Couldn't open `{}` for writing", backup_file_path);
			return;
		}
		backup_file << guild_settings_json.dump(4);
	}
	catch (const std::filesystem::filesystem_error& e) {
		Logger::exception(true, "File system exception while creating backup directory for `{}`: {}", backup_file_path, e.what());
	}
	catch (const std::exception& e) {
		Logger::exception(true, "Exception during backup for `{}`: {}", backup_file_path, e.what());
	}
}

void initialize_backups(dpp::cluster& bot) {
	Logger::info(true, "Performing initial backups on startup");

	backup_guild_settings("backups/backup-daily/guild_settings.json");
	backup_guild_settings("backups/backup-per-12h/guild_settings.json");
	backup_guild_settings("backups/backup-hourly/guild_settings.json");

	const auto now{ std::chrono::system_clock::now() };
	const std::time_t now_t{ std::chrono::system_clock::to_time_t(now) };
	std::tm current_utc_time;

#ifdef _WIN32
	gmtime_s(&current_utc_time, &now_t);
#else
	gmtime_r(&now_t, &current_utc_time);
#endif

	const uint64_t seconds_past_hour{ static_cast<uint64_t>(current_utc_time.tm_min * 60) + static_cast<uint64_t>(current_utc_time.tm_sec) };
	const uint64_t seconds_to_next_hour{ one_hour - (seconds_past_hour % one_hour) };

	const uint64_t seconds_past_midnight{ static_cast<uint64_t>(current_utc_time.tm_hour) * one_hour + static_cast<uint64_t>(current_utc_time.tm_min * 60) + static_cast<uint64_t>(current_utc_time.tm_sec) };
	const uint64_t seconds_to_next_midnight{ one_day - (seconds_past_midnight % one_day) };

	const uint64_t seconds_to_next_12_hr_mark{ twelve_hours - (seconds_past_midnight % twelve_hours) };

	Logger::info(true, "Scheduling backups");
	Logger::info(true, "Next hourly backup in {}", convert_time(seconds_to_next_hour));
	Logger::info(true, "Next 12h backup in {}", convert_time(seconds_to_next_12_hr_mark));
	Logger::info(true, "Next 00:00 UTC backup in {}", convert_time(seconds_to_next_midnight));

	auto schedule_backup = [&](const std::string& path, uint64_t initial_delay, uint64_t interval) {
		bot.start_timer([&bot, path, interval](dpp::timer) {
			Logger::info(true, "Performing first scheduled backup for: {}", path);
			backup_guild_settings(path);

			class RecurringBackupTask {
			private:
				dpp::cluster& bot;
				const std::string path;
				const uint64_t interval;
			public:
				RecurringBackupTask(dpp::cluster& bot, const std::string& path, const uint64_t interval) : bot{ bot }, path{ path }, interval{interval} {}

				void operator()(dpp::timer) const {
					Logger::info(true, "Performing recurring backup for: {}", path);
					backup_guild_settings(path);
					bot.start_timer(*this, interval);
				}
			};

			bot.start_timer(RecurringBackupTask{ bot, path, interval }, interval);
		}, initial_delay);
	};

	schedule_backup("backups/backup-hourly/guild_settings.json", seconds_to_next_hour, one_hour);
	schedule_backup("backups/backup-per-12h/guild_settings.json", seconds_to_next_12_hr_mark, twelve_hours);
	schedule_backup("backups/backup-daily/guild_settings.json", seconds_to_next_midnight, one_day);
}
