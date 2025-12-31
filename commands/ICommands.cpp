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
 * The following include is performed:
 * #include <ICommands.hpp>
 */

#include <pch.hpp>

// This creates a central registry
// Whenever a new command is created, its registration function is called here
void register_all_commands() {

	// From `/utility/`
	register_ping_command();
	register_stats_command();

	// From `/moderation/`
	register_role_add_command();
	// TODO: Add other command registration calls here
}

// This creates a central registry for select handlers
// Whenever a new select menu is created, its registration function is called here
void register_all_select_handlers() {

	// From `/moderation/`
	register_role_add_command(); // This function now *also* registers its select handler
	// TODO: Add other select handler registration calls here
}
