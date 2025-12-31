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
 * The following includes are performed:
 * #include <string>
 * #if defined (_WIN32)
 *     #include <Windows.h>
 *     #include <conio.h>
 *     #include <processenv.h>
 *     #include <consoleapi.h>
 * #else
 *     #include <termios.h>
 *     #include <unistd.h>
 * #endif
 * #include <iostream>
 * #include <fstream>
 * #include <sstream>
 * #include <vector>
 * #include <unordered_map>
 * #include <atomic>
 * #include <string>
 * #include <exception>
 * #include <algorithm>
 * #include <stdexcept>
 * #include <cstdlib>
 * #include <sodium/core.h>
 * #include <sodium/crypto_secretstream_xchacha20poly1305.h>
 * #include <sodium/utils.h>
 * #include <secrets/secrets.hpp>
 * #include <secrets/get_secret_input.hpp>
 * #include <console_utils/console_utils.hpp>
 */

#include <pch.hpp>

const std::unordered_map<std::string, std::string> secrets{ []() -> std::unordered_map<std::string, std::string> {
	Logger::info("secrets population");
#ifdef _WIN32
	console_setup_success.exchange([]() -> bool {
		const HANDLE h_out{ GetStdHandle(STD_OUTPUT_HANDLE) };
		if (h_out == INVALID_HANDLE_VALUE) return false;

		const HANDLE h_err{ GetStdHandle(STD_ERROR_HANDLE) };
		if (h_err == INVALID_HANDLE_VALUE) return false;

		DWORD dw_mode_out{ 0 };
		DWORD dw_mode_err{ 0 };

		if (!GetConsoleMode(h_out, &dw_mode_out)) return false;
		if (!GetConsoleMode(h_err, &dw_mode_err)) return false;

		dw_mode_out |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
		dw_mode_err |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;

		if (!SetConsoleMode(h_out, dw_mode_out)) return false;
		if (!SetConsoleMode(h_err, dw_mode_err)) return false;

		return true;
	}());

	if (console_setup_success.load()) {
		ConsoleColour::Reset = "\033[0m";
		ConsoleColour::Red = "\033[31m";
		ConsoleColour::Green = "\033[32m";
		ConsoleColour::Yellow = "\033[33m";
		ConsoleColour::Cyan = "\033[36m";
		Logger::success("Console colour setup successful");
	}
	else Logger::warn("Console colour setup failure");
#endif // _WIN32

	try {
		if (const int init_code{ sodium_init() }; init_code < 0) throw std::runtime_error("Couldn't initialize libsodium. sodium_init() code: " + std::to_string(init_code));

		std::ifstream input_file{ "secrets.enc", std::ios::binary };
		if (!input_file.is_open()) throw std::runtime_error("Couldn't open `secrets.enc`");

		std::cout << "Enter the secret key: ";
		std::string key_hex{ []() -> std::string {
			std::string key{};

			#ifdef _WIN32
				const HANDLE h_stdin{ GetStdHandle(STD_INPUT_HANDLE) };
				DWORD t_mode{ 0 };
				GetConsoleMode(h_stdin, &t_mode);
				SetConsoleMode(h_stdin, t_mode & (~ENABLE_ECHO_INPUT)); // Disable console echo

				char ch;
				while ((ch = _getch()) != '\r') {
					if (ch == '\b') {
						if (!key.empty()) {
							key.pop_back();
							std::cout << "\b \b"; // Erase the character from the console
						}
					}
					else if (ch == 0 || ch == static_cast<char>(0xE0)) static_cast<void>(_getch());
					else if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9')) {
						key.push_back(ch);
						std::cout << "*";
					}
				}
				std::cout << std::endl;
				SetConsoleMode(h_stdin, t_mode); // Restore original mode
			#else // ^^^ _WIN32 || !_WIN32 vvv
				termios t_old;
				tcgetattr(STDIN_FILENO, &t_old);
				termios t_new{ t_old };
				t_new.c_lflag &= ~ECHO; // Turn off echo
				tcsetattr(STDIN_FILENO, TCSANOW, &t_new);

				std::getline(std::cin, key);
				key.erase(std::remove_if(key.begin(), key.end(), [](const char c) {
					return !((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'));
				}), key.end());

				tcsetattr(STDIN_FILENO, TCSANOW, &t_old); // Restore original t_mode
				std::cout << std::endl;
			#endif // defined _WIN32
			return key;
		}()};

		if (key_hex.empty()) throw std::runtime_error("No key entered");

		unsigned char key[crypto_secretstream_xchacha20poly1305_KEYBYTES];
		if (sodium_hex2bin(key, sizeof(key), key_hex.c_str(), key_hex.length(), NULL, NULL, NULL) != 0) throw std::runtime_error("Invalid hex key format");

		sodium_memzero(key_hex.data(), key_hex.size()); // Wipe key from memory

		unsigned char header[crypto_secretstream_xchacha20poly1305_HEADERBYTES]{};
		input_file.read(reinterpret_cast<char*>(header), sizeof(header));

		if (input_file.gcount() != sizeof(header)) throw std::runtime_error("Encrypted file is too small or header is missing");

		crypto_secretstream_xchacha20poly1305_state crypto_state{};
		if (crypto_secretstream_xchacha20poly1305_init_pull(&crypto_state, header, key) != 0) {
			sodium_memzero(key, sizeof(key));
			throw std::runtime_error("Invalid header or key");
		}

		sodium_memzero(key, sizeof(key));

		std::stringstream decrypted_content;
		constexpr unsigned int CHUNK_SIZE{ 4096 }; // 4096 bytes = 4 KB

		std::vector<unsigned char> ciphertext_chunk(CHUNK_SIZE + crypto_secretstream_xchacha20poly1305_ABYTES);
		std::vector<unsigned char> decrypted_chunk(CHUNK_SIZE);
		unsigned long long decrypted_len;
		unsigned char tag;

		do {
			input_file.read(reinterpret_cast<char*>(ciphertext_chunk.data()), ciphertext_chunk.size());
			const unsigned long long bytes_read{ static_cast<unsigned long long>(input_file.gcount()) };

			if (bytes_read == 0) break;

			if (crypto_secretstream_xchacha20poly1305_pull(
				&crypto_state, decrypted_chunk.data(),
				&decrypted_len, &tag,
				ciphertext_chunk.data(), bytes_read,
				NULL, 0) != 0) {
				throw std::runtime_error("Decryption failed. The file may be corrupt");
			}

			decrypted_content.write(reinterpret_cast<const char*>(decrypted_chunk.data()), decrypted_len);
		} while (tag != crypto_secretstream_xchacha20poly1305_TAG_FINAL);

		std::unordered_map<std::string, std::string> loaded_secrets{};
		std::string line{};
		std::string plaintext{ decrypted_content.str() };
		std::stringstream content_stream(plaintext);

		while (std::getline(content_stream, line)) {
			if (!line.empty() && line.back() == '\r') line.pop_back();

			if (line.empty() || line[0] == '#') continue;
			const size_t pos{ line.find('=') };
			if (pos != std::string::npos && pos > 0) loaded_secrets[line.substr(0, pos)] = line.substr(pos + 1);
		}

		return loaded_secrets;
	}
	catch (std::exception& e) {
		std::cerr << ConsoleColour::Red << "Exception thrown during secrets initialization: " << e.what()
			<< "\nProgram will now terminate" << ConsoleColour::Reset << std::endl;
		std::exit(EXIT_FAILURE);
	}
	catch (...) {
		std::cerr << ConsoleColour::Red << "Unknown exception during secrets initialization"
			<< "\nProgram will now terminate" << ConsoleColour::Reset << std::endl;
		std::exit(EXIT_FAILURE);
	}
}() };
