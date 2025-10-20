# Ishmael

A C++ Discord bot built using the [dpp](https://dpp.dev/) library.

## Prerequisites (Windows)

Before building, you must have the following tools installed on your system:

  * **Git:** For cloning this repository and `vcpkg`.
  * **Visual Studio 2019 or 2022:** With the "Desktop development with C++" workload installed.
  * **CMake:** Version 3.16 or newer.
  * **vcpkg:** Used to install the required dependencies.

## Dependencies

This project requires the following libraries:

  * dpp (D++ Library)
  * libsodium (via the `unofficial-sodium` vcpkg package on Windows)

## Build Instructions (Windows)

1.  **Clone the Repository**

    ```bash
    git clone https://github.com/Omega493/ishmael-prod.git
    cd ishmael-prod
    ```

2.  **Set Up vcpkg**

    If you do not already have `vcpkg` installed, clone and bootstrap it.

    ```bash
    git clone https://github.com/microsoft/vcpkg.git
    .\vcpkg\bootstrap-vcpkg.bat
    .\vcpkg\vcpkg integrate install
    ```

3.  **Install Dependencies via vcpkg**

    Use `vcpkg` to install the required libraries. For a 64-bit build, use the `x64-windows` triplet.

    ```bash
    .\vcpkg\vcpkg install dpp:x64-windows unofficial-sodium:x64-windows
    ```

4.  **Configure with CMake**

    Create a build directory and run CMake to generate the build files. You must provide the path to the `vcpkg.cmake` toolchain file (replace [path-to-vcpkg] with the correct path to your vcpkg installation).

    ```bash
    cmake -B build -S . -A x64 -DCMAKE_TOOLCHAIN_FILE="[path-to-vcpkg]/scripts/buildsystems/vcpkg.cmake"
    ```

5.  **Build the Project**

    Compile the project using CMake's build command.

    Build in Debug configuration (default):

    ```bash
    cmake --build build
    ```
    Or, to build in Release configuration:
    
    ```bash
    cmake --build build --config Release
    ```

6.  **Run**

    The executable will be located in the `build` directory, inside the configuration folder (e.g., `Debug` or `Release`).

    ```bash
    .\build\Debug\Ishmael.exe
    ```

This project hasn't yet been tested on Debian / Ubuntu, so no build instructions for now.