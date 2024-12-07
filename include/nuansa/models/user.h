#ifndef NUANSA_MODELS_USER_H
#define NUANSA_MODELS_USER_H

#include "nuansa/utils/pch.h"

namespace nuansa::models {
	class User {
	public:
		User() = default;

		User(const std::string &username, const std::string &email,
		     const std::string &passwordHash, const std::string &salt);

		std::string GetUsername() const { return username; }
		std::string GetEmail() const { return email; }
		std::string GetPasswordHash() const { return passwordHash; }
		const std::string &GetSalt() const { return salt; }
		bool IsActive() const { return isActive; }


		nlohmann::json ToJson() const;

		static User FromJson(const nlohmann::json &json);

		// Database operations
		bool Save();

		bool Update();

		bool Delete();

		static std::optional<User> FindByUsername(const std::string &username);

		static std::optional<User> FindByEmail(const std::string &email);

		static bool Authenticate(const std::string &username, const std::string &password);

		static std::string GenerateSalt(size_t length = 32);

		static std::string HashPassword(const std::string &password, const std::string &salt);

		static bool VerifyPassword(const std::string &password, const std::string &salt,
		                           const std::string &hashedPassword);

	private:
		std::string username;
		std::string email;
		std::string passwordHash;
		std::string salt;
		bool isActive{};

		// Database helpers
		static pqxx::connection CreateConnection();
	};
} // namespace nuansa::models

#endif // NUANSA_MODELS_USER_H
