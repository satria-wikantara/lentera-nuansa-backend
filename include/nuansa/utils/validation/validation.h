//
// Created by I Gede Panca Sutresna on 01/12/24.
//

#ifndef NUANSA_UTILS_VALIDATION_VALIDATION_H
#define NUANSA_UTILS_VALIDATION_VALIDATION_H

#include "nuansa/pch/pch.h"

namespace nuansa::utils::validation {
	bool ValidateUsername(const std::string &username);

	bool ValidateEmail(const std::string &email);

	bool ValidatePassword(const std::string &password);
}

#endif //NUANSA_UTILS_VALIDATION_VALIDATION_H