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

#include <string>
#include <fstream>

enum class LogLevel {
	Info,
	Warn,
	Error,
	Critical,
	Exception,
	Unknown
};

class Logger {
public:
	Logger();
	~Logger();

	/*
	* @brief The loging function
	* @param level The log level of the message
	* @param message The message itself
	* @param to_be_printed If true, prints the message to the terminal
	*/
	void log(const LogLevel level, const std::string& message, const bool to_be_printed);
private:
	std::ofstream log_file;
};