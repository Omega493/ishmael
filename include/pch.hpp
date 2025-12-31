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

#ifndef PCH_HPP
#define PCH_HPP

#pragma once

#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#define NOMINMAX
	#include <Windows.h>
	#include <conio.h>
	#include <processenv.h>
	#include <consoleapi.h>
	#include <corecrt.h>
#else
	#include <termios.h>
	#include <unistd.h>
#endif

#include <iostream>
#include <fstream>
#include <sstream>
#include <ios>

#include <string>
#include <vector>
#include <unordered_map>

#include <algorithm>
#include <exception>
#include <stdexcept>
#include <future>
#include <variant>
#include <memory>
#include <mutex>
#include <chrono>
#include <thread>
#include <atomic>
#include <optional>
#include <type_traits>
#include <filesystem>
#include <format>
#include <utility>

#include <cstdlib>
#include <cstdint>
#include <ctime>

#include <sodium/core.h>
#include <sodium/crypto_secretstream_xchacha20poly1305.h>
#include <sodium/utils.h>

#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_INFO
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/base_sink.h>

#include <dpp/appcommand.h>
#include <dpp/cache.h>
#include <dpp/channel.h>
#include <dpp/cluster.h>
#include <dpp/discordclient.h>
#include <dpp/dispatcher.h>
#include <dpp/exception.h>
#include <dpp/guild.h>
#include <dpp/message.h>
#include <dpp/misc-enum.h>
#include <dpp/nlohmann/json.hpp>
#include <dpp/nlohmann/json_fwd.hpp>
#include <dpp/once.h>
#include <dpp/permissions.h>
#include <dpp/presence.h>
#include <dpp/restresults.h>
#include <dpp/role.h>
#include <dpp/snowflake.h>
#include <dpp/timer.h>
#include <dpp/user.h>
#include <dpp/version.h>

#include <secrets/secrets.hpp>
#include <other_utils/other_utils.hpp>
#include <logger/logger.hpp>
#include <exception/exception.hpp>
#include <console_utils/console_utils.hpp>

#include <ICommands.hpp>
#include <moderation/mod_utils.hpp>

#include <Ishmael.hpp>

#endif // PCH_HPP
