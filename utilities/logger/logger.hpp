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

#ifndef LOGGER_HPP
#define LOGGER_HPP

#pragma once

/*
 * The following includes are performed:
 * #include <memory>
 * #include <format>
 * #include <spdlog/spdlog.h>
 */

#include <pch.hpp>

class Logger {
public:
    Logger() = delete;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;

    static void info(const std::string& msg); // Directly writes to stdout
    static void success(const std::string& msg); // Directly writes to stdout
    static void warn(const std::string& msg); // Directly writes to stdout
    static void error(const std::string& msg); // Directly writes to stderr
    static void exception(const std::string& msg); // Directly writes to stderr
    static void unknown(const std::string& msg); // Directly writes to stderr

    // Uses spdlog and writes to file (and stdout depending on the flag console_output)
    static void info(const bool console_output, const std::string& msg);
    // Uses spdlog and writes to file (and stdout depending on the flag console_output)
    static void warn(const bool console_output, const std::string& msg);
    // Uses spdlog and writes to file (and stderr depending on the flag console_output)
    static void error(const bool console_output, const std::string& msg);
    // Uses spdlog and writes to file (and stderr depending on the flag console_output)
    static void exception(const bool console_output, const std::string& msg);
    // Uses spdlog and writes to file (and stderr depending on the flag console_output)
    static void unknown(const bool console_output, const std::string& msg);

    // Uses spdlog and writes to file (and stdout depending on the flag console_output)
    template<typename... Args>
    static void info(const bool console_output, spdlog::format_string_t<Args...> fmt, Args&&... args) {
        if (const auto log{ get_logger(console_output) }) log->info(fmt, std::forward<Args>(args)...);
    }

    // Uses spdlog and writes to file (and stdout depending on the flag console_output)
    template<typename... Args>
    static void warn(const bool console_output, spdlog::format_string_t<Args...> fmt, Args&&... args) {
        if (const auto log{ get_logger(console_output) }) log->error(fmt, std::forward<Args>(args)...);
    }

    // Uses spdlog and writes to file (and stderr depending on the flag console_output)
    template<typename... Args>
    static void error(const bool console_output, spdlog::format_string_t<Args...> fmt, Args&&... args) {
        if (const auto log{ get_logger(console_output) }) log->error(fmt, std::forward<Args>(args)...);
    }

    // Uses spdlog and writes to file (and stderr depending on the flag console_output)
    template<typename... Args>
    static void exception(const bool console_output, spdlog::format_string_t<Args...> fmt, Args&&... args) {
        if (const auto log{ get_logger(console_output) }) log->critical(fmt, std::forward<Args>(args)...);
    }

    // Uses spdlog and writes to file (and stderr depending on the flag console_output)
    template<typename... Args>
    static void unknown(const bool console_output, spdlog::format_string_t<Args...> fmt, Args&&... args) {
        if (const auto log{ get_logger(console_output) }) log->critical(fmt, std::forward<Args>(args)...);
    }

private:
    static std::shared_ptr<spdlog::logger> get_logger(const bool console_output);

    static void init();
    static void log_scheduler();
    static void log_rotator();
    static void log_worker();
};

#endif // LOGGER_HPP
