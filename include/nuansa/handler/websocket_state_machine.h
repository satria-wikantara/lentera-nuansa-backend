#ifndef NUANSA_WEBSOCKET_STATE_MACHINE_H
#define NUANSA_WEBSOCKET_STATE_MACHINE_H

#include "nuansa/pch/pch.h"
#include "nuansa/handler/websocket_client.h"
#include "nuansa/handler/websocket_server.h"

namespace nuansa::handler {
	class WebSocketStateMachine {
	public:
		WebSocketStateMachine(std::shared_ptr<WebSocketClient> client, std::shared_ptr<WebSocketServer> server);

		void ProcessMessage(const nlohmann::json &msgData);

		ClientState GetCurrentState() const { return state; }

		void HandleAuthMessage(const nlohmann::json &msgData);

		void TransitionTo(ClientState newState);

		[[nodiscard]] bool HandleLogin(const nlohmann::json &msgData) const;

	private:
		std::shared_ptr<WebSocketClient> client;
		ClientState state;
		std::shared_ptr<WebSocketServer> websocketServer;

		// State handlers
		void HandleInitialState(const nlohmann::json &msgData);

		static void HandleAwaitingAuthState(const nlohmann::json &msgData);

		void HandleAuthenticatedState(const nlohmann::json &msgData);

		// Message handlers
		void HandleRegistration(const nlohmann::json &msgData) const;

		static void HandleAuth(const std::string &message);

		static void HandleLogout();

		static void HandleNewMessage(const nlohmann::json &msgData);

		static void HandleEditMessage(const nlohmann::json &msgData);

		static void HandleDeleteMessage(const nlohmann::json &msgData);

		static void HandleDirectMessage(const nlohmann::json &msgData);

		static void HandlePluginMessage(const nlohmann::json &msgData);

		// Helper methods
		static void AddAuthenticatedClient();

		static void SendAuthRequiredMessage();

		static void SendErrorMessage(const std::string &msgData);

		void SendMessage(const std::string &msgData) const;
	};
} // namespace nuansa::handler

#endif // NUANSA_WEBSOCKET_STATE_MACHINE_H
