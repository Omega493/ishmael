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

#include "console_utils.hpp"

#ifdef _WIN32
#include <Windows.h>

void setup_console() {
	// Get the standard output handle
	HANDLE h_out = GetStdHandle(STD_OUTPUT_HANDLE);
	if (h_out == INVALID_HANDLE_VALUE) return;
	
	// Get the standard error handle
	HANDLE h_err = GetStdHandle(STD_ERROR_HANDLE);
	if (h_err == INVALID_HANDLE_VALUE) return;

	DWORD dw_mode_out = 0;
	DWORD dw_mode_err = 0;

	if (!GetConsoleMode(h_out, &dw_mode_out)) return;
	if (!GetConsoleMode(h_err, &dw_mode_err)) return;

	dw_mode_out |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	dw_mode_err |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;

	if (!SetConsoleMode(h_out, dw_mode_out)) return;
	if (!SetConsoleMode(h_err, dw_mode_err)) return;
}
#endif