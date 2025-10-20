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

#include "logger.hpp"

#include <iostream>
#include <string>
#include <fstream>
#include <chrono>
#include <format>
#include <filesystem>

Logger::Logger() {
	std::filesystem::create_directory("logs");

	const auto current_utc_time = std::chrono::utc_clock::now();
	std::string file_name = std::format("logs/logs_{:%d-%m-%Y_%H-%M-%S}.txt", current_utc_time);

	log_file.open(file_name);

	if (log_file.is_open()) log(LogLevel::Info, "Logger initialized");
	else std::cerr << std::format("Error: Couldn't open log file `{}`\nThe bot would run, but without logging features", file_name);
}

Logger::~Logger() {
	if (log_file.is_open()) log(LogLevel::Info, "Logger shutting down");
}

void Logger::log(LogLevel level, const std::string& message) {
	if (!log_file.is_open()) return;

	const auto now = std::chrono::system_clock::now();
	std::string timestamp = std::format("[{:%d %m %Y %a %H:%M:%S}]", now);

	std::string level_to_str;

	switch (level) {
	case LogLevel::Info: {
		level_to_str = "[INFO]";
		break;
	}
	case LogLevel::Warn: {
		level_to_str = "[WARN]";
		break;
	}
	case LogLevel::Error: {
		level_to_str = "[ERROR]";
		break;
	}
	case LogLevel::Critical: {
		level_to_str = "[CRITICAL]";
		break;
	}
	case LogLevel::Exception: {
		level_to_str = "[EXCEPTION THROWN]";
		break;
	}
	default: {
		level_to_str = "[LEVEL UNKNOWN]";
		break;
	}
	}

	log_file << std::format("{} {} {}", timestamp, level_to_str, message) << std::endl;
}