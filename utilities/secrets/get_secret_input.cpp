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

#if defined (_WIN32)
	#include <Windows.h>
	#include <conio.h>
	#include <processenv.h>
	#include <consoleapi.h>
#else
	#include <termios.h>
	#include <unistd.h>
#endif

#include "get_secret_input.hpp"

std::string get_secret_input() {
	std::string key = "";

#if defined(_WIN32)
	HANDLE h_stdin = GetStdHandle(STD_INPUT_HANDLE);
	DWORD mode = 0;
	GetConsoleMode(h_stdin, &mode);
	SetConsoleMode(h_stdin, mode & (~ENABLE_ECHO_INPUT)); // Disable console echo

	char ch;
	while ((ch = _getch()) != '\r') { // \r -> Enter key
		if (ch == '\b') { // \b -> backspace
			key.pop_back();
			std::cout << "\b \b"; // Erase the character from the console
		}
		else {
			key.push_back(ch);
			std::cout << "*";
		}
	}
	std::cout << std::endl;
	SetConsoleMode(h_stdin, mode); // Restore original mode

#else
	termios oldt;
	tcgetattr(STDIN_FILENO, &oldt);
	termios newt = oldt;
	newt.c_lflag &= ~ECHO; // Turn off echo
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);

	std::getline(std::cin, key);

	tcsetattr(STDIN_FILENO, TCSANOW, &oldt); // Restore original mode
	std::cout << std::endl;
#endif

	return key;
}