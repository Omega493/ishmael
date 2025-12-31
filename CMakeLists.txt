cmake_minimum_required(VERSION 3.16)

project("Ishmael")

add_executable(Ishmael "Ishmael.cpp" "Ishmael.hpp" "include/pch.hpp"
    # Bot's utilities
    "utilities/secrets/secrets.hpp" "utilities/secrets/secrets.cpp" "utilities/exception/exception.hpp"
    "utilities/logger/logger.hpp" "utilities/logger/logger.cpp" "utilities/console_utils/console_utils.hpp"
    "utilities/other_utils/other_utils.hpp" "utilities/other_utils/other_utils.cpp"
    
    # Bot's command handler
    "commands/ICommands.hpp" "commands/ICommands.cpp"

    # Utility commands
    "commands/utility/ping.cpp" "commands/utility/stats.cpp"

    # Moderation commands utilities
    "commands/moderation/mod_utils.hpp" "commands/moderation/mod_utils.cpp"

    # Moderation commands
    "commands/moderation/role_edits/role_add.cpp"
)

target_include_directories(Ishmael PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/utilities ${CMAKE_CURRENT_SOURCE_DIR}/commands
)

# DPP
find_package(dpp REQUIRED)
target_link_libraries(Ishmael PRIVATE dpp::dpp)

# libsodium
if(WIN32)
    find_package(unofficial-sodium CONFIG REQUIRED)
    target_link_libraries(Ishmael PRIVATE unofficial-sodium::sodium)
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SODIUM REQUIRED libsodium)

    target_include_directories(Ishmael PRIVATE ${SODIUM_INCLUDE_DIRS})

    target_link_libraries(Ishmael PRIVATE ${SODIUM_LIBRARIES})
endif()

# spdlog
find_package(spdlog CONFIG REQUIRED)
target_link_libraries(Ishmael PRIVATE spdlog::spdlog)

set_property(TARGET Ishmael PROPERTY CXX_STANDARD 20)
