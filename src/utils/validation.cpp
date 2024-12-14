//
// Created by I Gede Panca Sutresna on 05/12/24.
//
#include "nuansa/utils/validation.h"
#include "nuansa/utils/crypto/crypto_util.h"

bool nuansa::utils::Validation::ValidateUsername(const std::string &username) {
	// Username cannot be empty
	if (username.empty()) {
		return false;
	}

	// Username length between 3 and 32 characters
	if (username.length() < 3 || username.length() > 32) {
		return false;
	}

	// Username can only contain alphanumeric characters and underscores
	return std::ranges::all_of(username, [](const char c) {
		return std::isalnum(c) || c == '_';
	});
}

bool nuansa::utils::Validation::ValidateEmail(const std::string &email) {
	if (email.empty()) {
		return false;
	}

	// Basic email format validation
	const std::regex emailRegex(R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})");
	return std::regex_match(email, emailRegex);
}


bool nuansa::utils::Validation::ValidatePassword(const std::string &password) {
	// Password length requirements
	if (password.empty() || password.length() < 8 || password.length() > 128) {
		return false;
	}

	// Password complexity requirements
	bool hasUpper = false;
	bool hasLower = false;
	bool hasDigit = false;
	bool hasSpecial = false;

	for (const char c: password) {
		if (std::isupper(c)) hasUpper = true;
		else if (std::islower(c)) hasLower = true;
		else if (std::isdigit(c)) hasDigit = true;
		else hasSpecial = true;
	}

	LOG_DEBUG << "Password complexity: " << (hasUpper ? "upper" : "") << (hasLower ? "lower" : "") << (
		         hasDigit ? "digit" : "") << (hasSpecial ? "special" : "");
	std::string hashedPassword = nuansa::utils::crypto::CryptoUtil::HashPassword(password, "");
	LOG_DEBUG << "Hashed password: " << hashedPassword;

	return hasUpper && hasLower && hasDigit && hasSpecial;
}

std::filesystem::path nuansa::utils::Validation::NormalizePath(
	const std::filesystem::path& path,
	const PathValidationOptions& options) {
	
	std::filesystem::path normalizedPath;
	try {
		// Convert to canonical path (resolves "..", ".", and symbolic links)
		normalizedPath = std::filesystem::canonical(path);
		
		// Verify the path is within allowed boundaries
		std::filesystem::path canonicalBaseDir = std::filesystem::canonical(options.baseDir);
		
		if (!options.allowOutsideBaseDir) {
			// Check if the normalized path is within the base directory
			auto rel = std::filesystem::relative(normalizedPath, canonicalBaseDir);
			if (rel.string().find("..") != std::string::npos) {
				throw std::runtime_error("Access denied: Path must be within the base directory");
			}
		}

		if (options.mustBeRegularFile) {
			if (!std::filesystem::is_regular_file(normalizedPath)) {
				throw std::runtime_error("Path must be a regular file");
			}
		}

		// Check file permissions (on Unix-like systems)
		#ifndef _WIN32
		if (options.checkWorldReadable) {
			struct stat st;
			if (stat(normalizedPath.c_str(), &st) == 0) {
				// Ensure file is not world-readable
				if (st.st_mode & S_IROTH) {
					throw std::runtime_error("File has unsafe permissions");
				}
			}
		}
		#endif

		// Check file size if maximum is specified
		if (options.maxFileSize > 0) {
			const auto fileSize = std::filesystem::file_size(normalizedPath);
			if (fileSize > options.maxFileSize) {
				throw std::runtime_error("File is too large");
			}
		}

		return normalizedPath;

	} catch (const std::filesystem::filesystem_error& e) {
		throw std::runtime_error("Invalid path: " + std::string(e.what()));
	}
}
