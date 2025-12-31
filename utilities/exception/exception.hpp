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

#ifndef CUSTOM_EXCEPTIONS_HPP
#define CUSTOM_EXCEPTIONS_HPP

#pragma once

/*
 * The following include is performed:
 * #include <stdexcept>
 */

#include <pch.hpp>

/*
 * @brief Base class for all fatal, unrecoverable application errors
 *
 * Catching this type in main() should always result in a
 * non-restarting application shutdown
 */
class FatalError : public std::runtime_error {
public:
	using std::runtime_error::runtime_error;
};

/*
 * @brief A fatal error that occurs during Logger initialization
 */
class LoggerError : public FatalError {
public:
	using FatalError::FatalError;
};

/*
 * @brief A fatal error that occurs when setting the console control handler
 */
class ControlHandlerError : public FatalError {
public:
	using FatalError::FatalError;
};

#endif // CUSTOM_EXCEPTIONS_HPP
