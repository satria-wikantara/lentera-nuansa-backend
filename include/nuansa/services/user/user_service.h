#ifndef NUANSA_SERVICES_USER_USER_SERVICE_H
#define NUANSA_SERVICES_USER_USER_SERVICE_H

#include "nuansa/utils/pch.h"

#include "nuansa/services/user/iuser_service.h"
#include "nuansa/utils/pattern/circuit_breaker.h"
#include "nuansa/models/user.h"

namespace nuansa::services::user {
	class UserService final : public IUserService {
	public:
		// TODO: use config and factory method to initialize circuit breaker
		UserService()
			: circuitBreaker_(utils::pattern::CircuitBreakerSettings{
				.failureThreshold = 5,
				.successThreshold = 3,
				.resetTimeout = std::chrono::seconds(60),
				.timeout = std::chrono::seconds(2)
			}) {
		}

		static UserService &GetInstance();

		void Initialize() override;

		std::optional<nuansa::models::User> GetUserByUsername(const std::string &username) override;

		std::optional<nuansa::models::User> GetUserByEmail(const std::string &email) override;

		bool IsEmailTaken(const std::string &email) const override;

		bool IsUsernameTaken(const std::string &username) const override;

		bool CreateUser(const nuansa::models::User &user) override;

		bool AuthenticateUser(const std::string &username, const std::string &password) override;

		bool UpdateUserEmail(const std::string &username, const std::string &newEmail) override;

		bool UpdateUserPassword(const std::string &username, const std::string &newPassword) override;

		bool DeleteUser(const std::string &username) override;

		static std::string HashPassword(const std::string &password);

	private:
		std::shared_ptr<pqxx::connection> fallbackConnection_;

		pqxx::connection *GetConnection() const;

		utils::pattern::CircuitBreaker circuitBreaker_;
	};
} // namespace nuansa::services::user

#endif // NUANSA_SERVICES_USER_USER_SERVICE_H
