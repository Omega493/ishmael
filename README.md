# Ishmael

A C++ Discord bot built using the [dpp](https://dpp.dev/) library. This project includes moderation and utility commands.

## Prerequisites (Windows)

Before building, you must have the following tools installed on your system:

  * **Git:** For cloning this repository and `vcpkg`
  * **Visual Studio 2019 or 2022:** With the "Desktop development with C++" workload installed
  * **CMake:** Version 3.16 or newer
  * **Ninja:** The build system
   - Note: The easiest way to get CMake and Ninja is to select "C++ CMake tools for Windows" in the Visual Studio Installer's "Individual components" tab.
  * A C++ compiler that supports C++20
  * **vcpkg:** The C++ package manager from Microsoft

## Dependencies

This project requires the following libraries:

  * dpp (D++ library)
  * libsodium

## Build Instructions (Windows)

1.  **Clone the Repository**

    Run the following command in a PowerShell / Windows Terminal session:
    ```powershell
    git clone https://github.com/Omega493/ishmael.git
    cd ishmael
    ```

2.  **Install vcpkg and Dependencies**:

    * Choose a directory to install `vcpkg`, such as `C:\dev\vcpkg`. Avoid paths with spaces like `Program Files`.
    * Using Windows Terminal, clone and bootstrap `vcpkg`:
      ```powershell
      git clone https://github.com/microsoft/vcpkg.git
      cd vcpkg
      .\bootstrap-vcpkg.bat
      ```
    * Install the dependencies using the `x64-windows` triplet.
      ```powershell
      .\vcpkg.exe install dpp:x64-windows libsodium:x64-windows
      ```
    * Create a new system environment variable:
        - Variable name: `CMAKE_TOOLCHAIN_FILE`
        - Variable value: `[path-to-vcpkg]/scripts/buildsystems/vcpkg.cmake`
    * Restart your terminal session for the new environment variable to take effect.

3. **Set Up Build Environment**
    
    All subsequent commands must be run from a `Developer Command Prompt for VS 2022`. This ensures that the C++ compiler (`cl.exe`), CMake, and Ninja are all available in your terminal's PATH. You can find this in your Start Menu (e.g., "x64 Native Tools Command Prompt for VS 2022").

4.  **Configure with CMake**

    In the developer command prompt, head to the your `Ishmael` directory. Run CMake using a preset to generate the build files. This will automatically select the generator (Ninja) and create the correct build directory as defined in `CMakePresets.json`.
    Choose one of the following presets to configure:

    * **To Configure for Debug:**
      ```powershell
      cmake --preset x64-debug
      ```

    * **To Configure for Release:**
      ```powershell
      cmake --preset x64-release
      ```

5.  **Build the Project**

    Compile the project using the build preset that matches the configuration you just set up.

    * **If you configured for Debug:** 
    ```powershell
    cmake --build --preset x64-debug
    ```
    * **If you configured for Release:**
    ```powershell
    cmake --build --preset x64-release
    ```

6.  **Run**

    The executable will be located in the `build` directory, inside the configuration folder (e.g., `Debug` or `Release`).

    ```bash
    .\build\Debug\Ishmael.exe
    ```

This project hasn't yet been tested on Debian / Ubuntu, so no build instructions for now.