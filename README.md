# Ishmael

A C++ Discord bot built using the [dpp](https://dpp.dev/) library. This project includes moderation and utility commands.

## Prerequisites

You need a C++ compiler that supports C++20, the `libsodium` library, openSSL, `dpp`, `spdlog` and 7-Zip.

### On Windows

The `CMakePresets.json` file provides two presets: 64-bit compilation in debug mode and 64-bit compilation in release mode.

#### MSVC

1. **Install Visual Studio:**
  * Download and install Visual Studio from the [Visual Studio website](https://visualstudio.microsoft.com/downloads/).
  * During installation, select the "Desktop development with C++" workload. This will install the compiler and CMake.

2. **Install vcpkg:**
  * Choose a directory to install `vcpkg`, such as `C:\dev\vcpkg`. Avoid paths with spaces like `Program Files`.
  * Using Windows Terminal, clone and bootstrap `vcpkg`:
  ```powershell
  git clone https://github.com/microsoft/vcpkg.git; cd vcpkg; .\bootstrap-vcpkg.bat
  ```
  * Put the path to your `vcpkg` installation as a system environment variable and restart your terminal.
  
3. **Install the dependencies:**
  * Execute the following in a terminal:
    ```powershell
    vcpkg install libsodium:x64-windows dpp:x64-windows spdlog:x64-windows
    ```

4. **Install 7-Zip:**
  * Go to the [official site of 7-Zip](https://www.7-zip.org/) and download and install it.

4. **Set Environment Variable:**
  * In your system environment variables, create a new variable:
    - Variable name: `CMAKE_TOOLCHAIN_FILE`
    - Variable value: `[path-to-vcpkg]/scripts/buildsystems/vcpkg.cmake`
  * Add the path to your 7-Zip installation in the system paths.
  * Restart your terminal session for the new environment variable to take effect.

## Build Instructions 

1. **Clone the Repository:**
  Run the following command in a PowerShell, Windows Terminal or Bash session:
  ```bash
  git clone https://github.com/Omega493/ishmael.git
  ```

2. **Set Up Build Environment:**
  * Open the cloned folder in Visual Studio, select the preset and press Control-Shift-B to build.

3. **Locate the Executable:**
  The executable will be in the `binaryDir` specified in the preset. `./build/windows/<preset_name>/Ishmael.exe`

## Usage

The program is a single executable named `Ishmael`.

1. Make sure you have a `secrets.enc` file in the same directory as the executable. It should be of the following format:
  ```txt
  BOT_TOKEN=
  DEV_GUILD_ID=
  OWNER_ID=
  ```
  To get the `BOT_TOKEN` head over to [Discord Developer Portal](https://discord.com/developers/applications). To get the `DEV_GUILD_ID` and `OWNER_TOKEN` you need to have "Developer Mode" enabled in your Discord client. Make sure it is encrypted using XChaCha20-Poly1305 AEAD. You can encrypt the text file using [my other tool](https://github.com/Omega493/crypto-utils).

2. Make sure the program has write access to the directory where it is currently located. Logging and creation of `guild_settings.json` will fail otherwise.

3. Run the program. As the `secrets` map is initialized, you'll be prompted to enter the secret key to the file. Just type the key or paste it in the field.
