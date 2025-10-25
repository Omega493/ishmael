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

#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <exception>
#include <stdexcept>
#include <cstdlib>
#include <map>

#include "secrets.hpp"
#include "get_secret_input.hpp"
#include "utilities/console_utils/console_utils.hpp"

#include <sodium/core.h>
#include <sodium/crypto_secretstream_xchacha20poly1305.h>
#include <sodium/utils.h>

static std::map<std::string, std::string> load_and_decrypt_secrets() {
	if (sodium_init() < 0) {
		throw std::runtime_error("Couldn't initialize libsodium");
	}

	std::ifstream input_file("secrets.enc", std::ios::binary);
	if (!input_file.is_open()) {
		throw std::runtime_error("Coudn't open `secrets.enc`");
	}

	std::cout << "Enter the secret key (hex): ";
	std::string key_hex = get_secret_input();

	if (key_hex.empty()) throw std::runtime_error("No key entered");

	unsigned char key[crypto_secretstream_xchacha20poly1305_KEYBYTES];
	if (sodium_hex2bin(key, sizeof(key), key_hex.c_str(), key_hex.length(), NULL, NULL, NULL) != 0) {
		throw std::runtime_error("Invalid hex key format");
	}

	// Wipe key from memory
	sodium_memzero(key_hex.data(), key_hex.size());

	unsigned char header[crypto_secretstream_xchacha20poly1305_HEADERBYTES];
	input_file.read(reinterpret_cast<char*>(header), sizeof(header));

	if (input_file.gcount() != sizeof(header)) {
		throw std::runtime_error("Encrypted file is too small or header is missing");
	}

	crypto_secretstream_xchacha20poly1305_state crypto_state;
	if (crypto_secretstream_xchacha20poly1305_init_pull(&crypto_state, header, key) != 0) {
		sodium_memzero(key, sizeof(key));
		throw std::runtime_error("Invalid header or key");
	}

	// Wipe key from memory
	sodium_memzero(key, sizeof(key));

	std::stringstream decrypted_content;
	constexpr size_t CHUNK_SIZE{ 4096 }; // 4096 bytes = 4 KB

	std::vector<unsigned char> ciphertext_chunk(CHUNK_SIZE + crypto_secretstream_xchacha20poly1305_ABYTES);
	std::vector<unsigned char> decrypted_chunk(CHUNK_SIZE);
	unsigned long long decrypted_len;
	unsigned char tag;

	do {
		input_file.read(reinterpret_cast<char*>(ciphertext_chunk.data()), ciphertext_chunk.size());
		size_t bytes_read = input_file.gcount();

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

	// Parse the decrypted contents into a std::map
	std::map<std::string, std::string> loaded_secrets;
	std::string line;
	std::string plaintext = decrypted_content.str();
	std::stringstream content_stream(plaintext);

	while (std::getline(content_stream, line)) {
		if (!line.empty() && line.back() == '\r') {
			line.pop_back();
		}

		if (line.empty() || line[0] == '#') continue;
		size_t pos = line.find('=');
		if (pos != std::string::npos && pos > 0) {
			loaded_secrets[line.substr(0, pos)] = line.substr(pos + 1);
		}
	}

	return loaded_secrets;
}

const std::map<std::string, std::string> secrets = [] {
	try {
		return load_and_decrypt_secrets();
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
}();