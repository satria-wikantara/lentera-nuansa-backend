#ifndef NUANSA_WEBSOCKET_CLIENT_H
#define NUANSA_WEBSOCKET_CLIENT_H

#include "nuansa/utils/pch.h"
#include "nuansa/messages/message_types.h"
#include "nuansa/services/auth/auth_status.h"

namespace nuansa::handler {
	enum class ClientState {
		Initial,
		AwaitingAuth,
		Authenticated,
		Disconnected
	};

	class WebSocketClient {
	public:
		WebSocketClient(std::string id, const std::shared_ptr<websocket::stream<tcp::socket> > &ws)
			: authStatus(), ws(ws), clientId(std::move(id)), state() {
			// Initialize any other members here
		}

		// Getters and setters
		[[nodiscard]] std::shared_ptr<websocket::stream<tcp::socket> > GetWebSocket() const { return ws; }
		void SetState(const ClientState newState) { state = newState; }
		[[nodiscard]] ClientState GetState() const { return state; }

		[[nodiscard]] bool IsAuthenticated() const {
			return authStatus == nuansa::services::auth::AuthStatus::Authenticated;
		}

		void SetAuthStatus(const nuansa::services::auth::AuthStatus status) { authStatus = status; }
		void SetAuthToken(const std::string &token) { authToken = token; }

		// Public members (could be made private with getters/setters)
		std::string username;
		std::optional<std::string> authToken;
		nuansa::services::auth::AuthStatus authStatus;

	private:
		std::shared_ptr<websocket::stream<tcp::socket> > ws;
		std::string clientId;


		ClientState state;
	};
} // namespace nuansa::handler

#endif // NUANSA_WEBSOCKET_CLIENT_H
