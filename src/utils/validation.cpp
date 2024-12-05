//
// Created by I Gede Panca Sutresna on 05/12/24.
//
#include "nuansa/utils/validation/validation.h"
#include "nuansa/utils/crypto/crypto_util.h"

bool nuansa::utils::validation::ValidateUsername(const std::string &username) {
	// Username cannot be empty
	if (username.empty()) {
		return false;
	}

	// Username length between 3 and 32 characters
	if (username.length() < 3 || username.length() > 32) {
		return false;
	}

	// Username can only contain alphanumeric characters and underscores
	return std::all_of(username.begin(), username.end(), [](char c) {
		return std::isalnum(c) || c == '_';
	});
}

bool nuansa::utils::validation::ValidateEmail(const std::string &email) {
	if (email.empty()) {
		return false;
	}

	// Basic email format validation
	const std::regex emailRegex(R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})");
	return std::regex_match(email, emailRegex);
}


bool nuansa::utils::validation::ValidatePassword(const std::string &password) {
	// Password length requirements
	if (password.empty() || password.length() < 8 || password.length() > 128) {
		return false;
	}

	// Password complexity requirements
	bool hasUpper = false;
	bool hasLower = false;
	bool hasDigit = false;
	bool hasSpecial = false;

	for (char c: password) {
		if (std::isupper(c)) hasUpper = true;
		else if (std::islower(c)) hasLower = true;
		else if (std::isdigit(c)) hasDigit = true;
		else hasSpecial = true;
	}

	BOOST_LOG_TRIVIAL(debug) << "Password complexity: " << (hasUpper ? "upper" : "") << (hasLower ? "lower" : "") << (
		                        hasDigit ? "digit" : "") << (hasSpecial ? "special" : "");
	std::string hashedPassword = nuansa::utils::crypto::HashPassword(password, "");
	BOOST_LOG_TRIVIAL(debug) << "Hashed password: " << hashedPassword;

	return hasUpper && hasLower && hasDigit && hasSpecial;
}
