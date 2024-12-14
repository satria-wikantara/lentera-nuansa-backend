//
// Created by I Gede Panca Sutresna on 01/12/24.
//

#ifndef NUANSA_CRYPTO_UTIL_H
#define NUANSA_CRYPTO_UTIL_H

#include "nuansa/utils/pch.h"

namespace nuansa::utils::crypto {
	class CryptoUtil {
	public:
		CryptoUtil();

		~CryptoUtil();

		static std::string GenerateSHA256Hash(const std::string &content);

		static std::string GenerateRandomSalt();

		static std::string HashPassword(const std::string &password, const std::string &salt);

		static std::string Base64Encode(const std::string& input);
		static std::string Base64Decode(const std::string& input);
	};
}

#endif //NUANSA_CRYPTO_UTIL_H
