# Changelogs

### 31 Dec. 2025
  #### Additions
    - (Impl.) **Precompiled Headers (PCH):** Added `include/pch.hpp`
    - (Impl.) **Spdlog Integration:** Replaced the custom logger with `spdlog`
    - (Impl.) **Log Archiving:** Implemented automatic log rotation and compression using 7-Zip
    - (Impl.) **Custom Exceptions:** Added `utilities/exception/exception.hpp`
    - (Docs) **Third-Party Licenses:** Added `THIRD-PARTY.md` to document license compliance for `libsodium`, `spdlog`, and `dpp`

  #### Changed
    - (Refactor) **Project Structure:** Switched command registries from `std::map` to `std::unordered_map`
    - (Refactor) **Secrets Handling:** Moved and refactored secret input logic directly into `secrets.cpp`
    - (Build) **CMake Configuration:** Updated `CMakeLists.txt` to include `spdlog` and conditionally handle 64-bit/32-bit architecture definitions
    - (Build) **Presets:** Streamlined `CMakePresets.json` to focus on Windows MSVC (x64 Debug/Release) workflows

  #### Removed
    - (Cleanup) Removed the custom `std::ofstream`-based logging implementation in favor of `spdlog`
    - (Cleanup) Removed `utilities/secrets/get_secret_input.hpp` and `.cpp`

### 25 Oct. 2025
  - (Impl.) The logger now runs on a mutex and scoped_lock, preventing race conditions
  - (Impl.) A `ConsoleColour` namespace to format the colour of the letters appearing in the console
  - (Fix) Removed hardcoded `#include`s
