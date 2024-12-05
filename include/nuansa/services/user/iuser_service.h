#ifndef NUANSA_SERVICES_USER_IUSER_SERVICE_H
#define NUANSA_SERVICES_USER_IUSER_SERVICE_H

#include "nuansa/pch/pch.h"

#include "nuansa/models/user.h"

namespace nuansa::services::user {
	class IUserService {
	public:

		virtual ~IUserService() = default;

		virtual void Initialize() = 0;

		virtual bool AuthenticateUser(const std::string& username, const std::string& password) = 0;
		virtual bool UpdateUserEmail(const std::string& username, const std::string& newEmail) = 0;
		virtual bool UpdateUserPassword(const std::string& username, const std::string& newPassword) = 0;
		virtual bool DeleteUser(const std::string& username) = 0;

		virtual std::optional<nuansa::models::User> GetUserByUsername(const std::string& username) = 0;
		virtual std::optional<nuansa::models::User> GetUserByEmail(const std::string& email) = 0;
		virtual bool IsEmailTaken(const std::string& email) const = 0;
		virtual bool IsUsernameTaken(const std::string& username) const = 0;
		virtual bool CreateUser(const nuansa::models::User& user) = 0;
	};

} // namespace nuansa::services::user

#endif // NUANSA_SERVICES_USER_IUSER_SERVICE_H