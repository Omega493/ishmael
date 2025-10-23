cmake_minimum_required(VERSION 3.16)

project("Ishmael")

add_executable(Ishmael "Ishmael.cpp" "Ishmael.hpp"
    # Bot's utilities
    "utilities/secrets/get_secret_input.hpp" "utilities/secrets/get_secret_input.cpp" "utilities/secrets/secrets.hpp"
    "utilities/secrets/secrets.cpp" "utilities/logger/logger.hpp" "utilities/logger/logger.cpp"
    "utilities/other_utils/other_utils.hpp" "utilities/other_utils/other_utils.cpp"
    
    # Bot's command handler
    "commands/ICommands.hpp" "commands/ICommands.cpp"

    # Utility commands
    "commands/utility/ping.cpp" "commands/utility/stats.cpp"

    # Moderation commands utilities
    "commands/moderation/mod_utils.hpp" "commands/moderation/mod_utils.cpp"

    # Moderation commands
    "commands/moderation/role_edits/role_add.cpp")

target_include_directories(Ishmael PUBLIC ${CMAKE_SOURCE_DIR})

# DPP
find_package(dpp REQUIRED)

target_link_libraries(Ishmael PRIVATE dpp::dpp)

# sodium
if(WIN32)
    find_package(unofficial-sodium REQUIRED)

    if(NOT TARGET sodium::sodium)
        add_library(sodium::sodium ALIAS unofficial-sodium::sodium)
    endif()

    target_link_libraries(Ishmael PRIVATE sodium::sodium)
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SODIUM REQUIRED libsodium)

    target_include_directories(Ishmael PRIVATE ${SODIUM_INCLUDE_DIRS})

    target_link_libraries(Ishmael PRIVATE ${SODIUM_LIBRARIES})
endif()

set_property(TARGET Ishmael PROPERTY CXX_STANDARD 20)