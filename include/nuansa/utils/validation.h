//
// Created by I Gede Panca Sutresna on 01/12/24.
//

#ifndef NUANSA_UTILS_VALIDATION_VALIDATION_H
#define NUANSA_UTILS_VALIDATION_VALIDATION_H

#include "nuansa/utils/pch.h"
#include <filesystem>
#include <optional>

namespace nuansa::utils {
	class Validation {
	public:
		static bool ValidateUsername(const std::string &username);

		static bool ValidateEmail(const std::string &email);

		static bool ValidatePassword(const std::string &password);

		struct PathValidationOptions {
			size_t maxFileSize = 0;
			bool mustBeRegularFile = false;
			bool checkWorldReadable = false;
			bool allowOutsideBaseDir = true;
			std::filesystem::path baseDir;
			std::vector<std::string> allowedExtensions;
		};

		// Returns normalized path if valid, throws std::runtime_error if invalid
		static std::filesystem::path NormalizePath(
			const std::filesystem::path& path,
			const PathValidationOptions& options = PathValidationOptions{});
	};
}

#endif //NUANSA_UTILS_VALIDATION_VALIDATION_H
