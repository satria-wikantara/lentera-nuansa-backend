//
// Created by I Gede Panca Sutresna on 07/12/24.
//

#ifndef NUANSA_SERVICES_AUTH_AUTH_STATUS_H
#define NUANSA_SERVICES_AUTH_AUTH_STATUS_H

#include "nuansa/utils/pch.h"

namespace nuansa::services::auth {
	enum class AuthStatus {
		Authenticated,
		InvalidCredentials,
		TokenExpired,
		NotAuthenticated
	};

	inline std::string AuthStatusToString(const AuthStatus authStatus) {
		switch (authStatus) {
			case AuthStatus::Authenticated: return "authenticated";
			case AuthStatus::InvalidCredentials: return "invalid_credentials";
			case AuthStatus::TokenExpired: return "token_expired";
			case AuthStatus::NotAuthenticated: return "not_authenticated";
			default: return "unknown";
		}
	}

	NLOHMANN_JSON_SERIALIZE_ENUM(AuthStatus, {
	                             {AuthStatus::Authenticated, "authenticated"},
	                             {AuthStatus::InvalidCredentials, "invalid_credentials"},
	                             {AuthStatus::TokenExpired, "token_expired"},
	                             {AuthStatus::NotAuthenticated, "not_authenticated"}
	                             });
}

#endif //NUANSA_SERVICES_AUTH_AUTH_STATUS_H
