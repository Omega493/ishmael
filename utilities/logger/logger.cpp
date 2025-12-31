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
 * #include <ios>
 * #include <chrono>
 * #include <format>
 * #include <filesystem>
 * #include <thread>
 * #include <cstdlib>
 * #define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_INFO
 * #include <spdlog/spdlog.h>
 * #include <spdlog/sinks/stdout_color_sinks.h>
 * #include <spdlog/sinks/basic_file_sink.h>
 * #include <spdlog/sinks/base_sink.h>
 * #include <logger.hpp>
 */

#include <pch.hpp>

static std::shared_ptr<spdlog::logger> combined_logger;
static std::shared_ptr<spdlog::logger> file_logger;
static std::atomic_bool is_logger_init{ false };
static std::atomic_bool is_first_run{ true };

static std::mutex state_mtx;

std::shared_ptr<spdlog::logger> Logger::get_logger(const bool console_output) {
	if (!is_logger_init.load()) {
		static std::mutex init_mtx;
		std::scoped_lock lock{ init_mtx };
		if (!is_logger_init.load()) init();
	}
	std::scoped_lock lock{ state_mtx };
	return console_output ? combined_logger : file_logger;
}

void Logger::info(const bool console_output, const std::string& msg) {
	if (const auto log{ get_logger(console_output) }) log->info(msg);
}

void Logger::warn(const bool console_output, const std::string& msg) {
	if (const auto log{ get_logger(console_output) }) log->warn(msg);
}

void Logger::error(const bool console_output, const std::string& msg) {
	if (const auto log{ get_logger(console_output) }) log->error(msg);
}

void Logger::exception(const bool console_output, const std::string& msg) {
	if (const auto log{ get_logger(console_output) }) log->critical(msg);
}

void Logger::unknown(const bool console_output, const std::string& msg) {
	if (const auto log{ get_logger(console_output) }) log->critical(msg);
}

void Logger::info(const std::string& msg) {
	std::cout << std::format("[{:%Y-%m-%d %a %H:%M:%S}] [{}info{}] {}",
		std::chrono::floor<std::chrono::seconds>(std::chrono::utc_clock::now()), ConsoleColour::Cyan, ConsoleColour::Reset, msg)
		<< std::endl;
}

void Logger::success(const std::string& msg) {
	std::cout << std::format("[{:%Y-%m-%d %a %H:%M:%S}] [{}success{}] {}",
		std::chrono::floor<std::chrono::seconds>(std::chrono::utc_clock::now()), ConsoleColour::Green, ConsoleColour::Reset, msg)
		<< std::endl;
}

void Logger::warn(const std::string& msg) {
	std::cout << std::format("[{:%Y-%m-%d %a %H:%M:%S}] [{}warn{}] {}",
		std::chrono::floor<std::chrono::seconds>(std::chrono::utc_clock::now()), ConsoleColour::Yellow, ConsoleColour::Reset, msg)
		<< std::endl;
}

void Logger::error(const std::string& msg) {
	std::cout << std::format("[{:%Y-%m-%d %a %H:%M:%S}] [{}error{}] {}",
		std::chrono::floor<std::chrono::seconds>(std::chrono::utc_clock::now()), ConsoleColour::Red, ConsoleColour::Reset, msg)
		<< std::endl;
}

void Logger::exception(const std::string& msg) {
	std::cout << std::format("[{:%Y-%m-%d %a %H:%M:%S}] [{}exception{}] {}",
		std::chrono::floor<std::chrono::seconds>(std::chrono::utc_clock::now()), ConsoleColour::Red, ConsoleColour::Reset, msg)
		<< std::endl;
}

void Logger::unknown(const std::string& msg) {
	std::cout << std::format("[{:%Y-%m-%d %a %H:%M:%S}] [{}unknown{}] {}",
		std::chrono::floor<std::chrono::seconds>(std::chrono::utc_clock::now()), ConsoleColour::Red, ConsoleColour::Reset, msg)
		<< std::endl;
}

template<typename Mutex>
class MaxLevelSink : public spdlog::sinks::base_sink<Mutex> {
	std::shared_ptr<spdlog::sinks::sink> target_sink;
	spdlog::level::level_enum max_level;

public:
	MaxLevelSink(std::shared_ptr<spdlog::sinks::sink> target, const spdlog::level::level_enum max_lvl)
		: target_sink{ std::move(target) },
		  max_level{ max_lvl } {}

protected:
	void sink_it_(const spdlog::details::log_msg& msg) override {
		if (msg.level <= max_level) target_sink->log(msg);
	}

	void flush_() override {
		target_sink->flush();
	}
};

using MaxLevelSink_mt = MaxLevelSink<std::mutex>;

void Logger::init() {
	if (is_logger_init.load()) return;

	log_rotator();

	is_logger_init.exchange(true);

	std::thread t{ log_scheduler };
	t.detach();
}

void Logger::log_scheduler() {
	log_worker();

	// Defaults to approx. 23:59:33 of the same (current) day for some reason
	// Aka, a time gap of 27 seconds
	auto next_rot_tp{ std::chrono::floor<std::chrono::days>(std::chrono::utc_clock::now()) + std::chrono::days{ 1 } };

	while (is_logger_init.load()) {
		std::this_thread::sleep_for(std::chrono::seconds{ 30 });

		if (!is_logger_init.load()) break;

		if (const auto now{ std::chrono::utc_clock::now() }; now >= next_rot_tp) {
			log_rotator();
			log_worker();

			next_rot_tp = std::chrono::floor<std::chrono::days>(now) + std::chrono::days{ 1 };
		}
		else {
			std::scoped_lock lock{ state_mtx };
			if (combined_logger) combined_logger->flush();
			if (file_logger) file_logger->flush();
		}
	}
}

void Logger::log_rotator() {
	std::scoped_lock lock{ state_mtx };

	try {
		if (!std::filesystem::exists("logs")) std::filesystem::create_directory("logs");

		const std::string filename{ std::format("logs/log_{:%d-%m-%Y_%H-%M-%S}.txt", std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::utc_clock::now())) };
		const std::string pattern{ "[%Y-%m-%d %a %H:%M:%S] [%^%l%$] %v" };

		const auto file_sink{ std::make_shared<spdlog::sinks::basic_file_sink_mt>(filename, true) };
		file_sink->set_level(spdlog::level::info);

		const auto stderr_sink{ std::make_shared<spdlog::sinks::stderr_color_sink_mt>() };
		stderr_sink->set_level(spdlog::level::err);

		const auto raw_stdout_sink{ std::make_shared<spdlog::sinks::stdout_color_sink_mt>() };
		raw_stdout_sink->set_pattern(pattern);
		raw_stdout_sink->set_color(spdlog::level::info, 0x0003);

		const auto stdout_sink{ std::make_shared<MaxLevelSink_mt>(raw_stdout_sink, spdlog::level::warn) };
		stdout_sink->set_level(spdlog::level::info);

		std::vector<spdlog::sink_ptr> combined_sinks{ stdout_sink, stderr_sink, file_sink };

		const auto new_combined{ std::make_shared<spdlog::logger>("combined", combined_sinks.begin(), combined_sinks.end()) };
		const auto new_file_only{ std::make_shared<spdlog::logger>("file_only", file_sink) };

		new_combined->set_pattern(pattern);
		new_file_only->set_pattern(pattern);

		new_combined->flush_on(spdlog::level::warn);
		new_file_only->flush_on(spdlog::level::err);

		combined_logger = new_combined;
		file_logger = new_file_only;

		if (is_first_run) {
			combined_logger->info("Logger started. File: `{}`", filename);
			is_first_run.exchange(false);
		}
		else {
			combined_logger->info("Logger rotated. File: `{}`", filename);
		}
	}
	catch (const std::exception& e) {
		std::cerr << ConsoleColour::Red << "Logger rotation failed: " << e.what() << ConsoleColour::Reset << std::endl;
	}
}

static bool try_fs_action(const std::function<void()>& action, const std::string& action_name) {
	constexpr int max_retries{ 3 };
	for (int i{ 0 }; i < max_retries; ++i) {
		try {
			action();
			return true;
		}
		catch (const std::exception& e) {
			if (i == max_retries - 1) {
				Logger::error(true, std::format("Filesystem Action '{}' failed after {} retries: {}", action_name, max_retries, e.what()));
			}
			std::this_thread::sleep_for(std::chrono::milliseconds{ 300 });
		}
	}
	return false;
}

void Logger::log_worker() {
	std::this_thread::sleep_for(std::chrono::seconds{ 1 });

	if (!std::filesystem::exists("logs")) return;

	const auto now{ std::chrono::system_clock::now() };
	constexpr auto seven_days{ std::chrono::hours{24 * 7} };
	constexpr auto fourteen_days{ std::chrono::hours{24 * 14} };

	try {
		if (!std::filesystem::exists("logs/logs_old")) std::filesystem::create_directory("logs/logs_old");

		for (const auto& file : std::filesystem::directory_iterator{ "logs" }) {
			if (!file.is_regular_file()) continue;

			if (file.path().filename() == "logs_old") continue;

			const auto sys_ftime{ std::chrono::clock_cast<std::chrono::system_clock>(file.last_write_time()) };

			if ((now - sys_ftime) > seven_days) {
				const auto src_path{ file.path() };
				const auto dest_path{ std::filesystem::path("logs/logs_old") / src_path.filename() };

				try_fs_action([&]() {
					if (std::filesystem::exists(dest_path)) std::filesystem::remove(dest_path);
					std::filesystem::rename(src_path, dest_path);
				}, std::format("Move {}", src_path.string()));
			}

		}
	}
	catch (const std::exception& e) {
		Logger::exception(true, std::format("Filesystem move exception: {}", e.what()));
	}

	try {
		if (!std::filesystem::exists("logs/logs_old")) return;

		for (const auto& file : std::filesystem::directory_iterator("logs/logs_old")) {
			if (!file.is_regular_file() || file.path().extension() == ".7z") continue;

			const auto sys_ftime{ std::chrono::clock_cast<std::chrono::system_clock>(file.last_write_time()) };

			if ((now - sys_ftime) > fourteen_days) {
				[&file] {
					const std::string dir_str{ std::filesystem::absolute(file.path().parent_path()).make_preferred().string() };
					const std::string file_str{ file.path().filename().string() };
					
					Logger::info(true, "Archiving files");
					
					#if !defined(NDEBUG) || defined(_DEBUG)
						// Unsupressed errors
						const std::string cmd{ std::format("cd /d \"{}\" && 7z a -t7z -mx=7 -m0=lzma2 -ssw -y \"logs_old.7z\" \"{}\"", dir_str, file_str) };
					#else // ^^^ Debug mode || Release mode vvv
						// Suppressed errors
						const std::string cmd{ std::format("cd /d \"{}\" && 7z a -t7z -mx=7 -m0=lzma2 -ssw -y \"logs_old.7z\" \"{}\" > nul 2>&1", dir_str, file_str) };
					#endif
					
					const int result{ std::system(cmd.c_str()) };

					if (result == 0) {
						try_fs_action([&]() {
							std::filesystem::remove(file.path());
						}, std::format("Delete archived {}", file_str));
					}
					else Logger::error(true, std::format("7z failed for {} with code {}", file_str, result));
				}();
			}
		}
	}
	catch (const std::exception& e) {
		Logger::exception(true, std::format("Archive fatal error: {}", e.what()));
	}
}
