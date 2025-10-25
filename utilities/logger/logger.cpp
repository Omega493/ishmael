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
#include <iostream>
#include <string>
#include <fstream>
#include <chrono>
#include <format>
#include <filesystem>

#include "logger.hpp"
#include "utilities/console_utils/console_utils.hpp"

Logger::Logger() {
	std::filesystem::create_directory("logs");

	const auto current_utc_time = std::chrono::utc_clock::now();
	std::string file_name = std::format("logs/logs_{:%d-%m-%Y_%H-%M-%S}.txt", current_utc_time);

	log_file.open(file_name);

	if (log_file.is_open()) log(LogLevel::Info, "Logger initialized", false);
	else std::cerr << ConsoleColour::Yellow
		<< std::format("Error: Couldn't open log file `{}`\nThe bot would run, but without logging features\n", file_name) << ConsoleColour::Reset;
}

Logger::~Logger() {
	if (log_file.is_open()) log(LogLevel::Info, "Logger shutting down", false);
}

void Logger::log(const LogLevel level, const std::string& message, const bool to_be_printed) {
	if (!log_file.is_open()) return;

	std::scoped_lock lock(log_file_mutex);

	const auto now = std::chrono::system_clock::now();
	std::string timestamp = std::format("[{:%d %m %Y %a %H:%M:%S}]", now);

	std::string level_to_str = "";

	switch (level) {
		case LogLevel::Info: {
			level_to_str = "[INFO]";
			if (to_be_printed) std::cout << ConsoleColour::Cyan << level_to_str + ' ' + message << ConsoleColour::Reset << std::endl;
			break;
		}
		case LogLevel::Warn: {
			level_to_str = "[WARN]";
			if (to_be_printed) std::cout << ConsoleColour::Yellow << level_to_str + ' ' + message << ConsoleColour::Reset << std::endl;
			break;
		}
		case LogLevel::Error: {
			level_to_str = "[ERROR]";
			if (to_be_printed) std::cerr << ConsoleColour::Red << level_to_str + ' ' + message << ConsoleColour::Reset << std::endl;
			break;
		}
		case LogLevel::Critical: {
			level_to_str = "[CRITICAL]";
			if (to_be_printed) std::cerr << ConsoleColour::Red << level_to_str + ' ' + message << ConsoleColour::Reset << std::endl;
			break;
		}
		case LogLevel::Exception: {
			level_to_str = "[EXCEPTION THROWN]";
			if (to_be_printed) std::cerr << ConsoleColour::Red << level_to_str + ' ' + message << ConsoleColour::Reset << std::endl;
			break;
		}
		default: {
			level_to_str = "[LEVEL UNKNOWN]";
			if (to_be_printed) std::cerr << ConsoleColour::Red << level_to_str + ' ' + message << ConsoleColour::Reset << std::endl;
			break;
		}
	}
	log_file << std::format("{} {} {}", timestamp, level_to_str, message) << std::endl;
}