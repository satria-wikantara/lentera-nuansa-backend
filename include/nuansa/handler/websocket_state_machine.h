#ifndef NUANSA_WEBSOCKET_STATE_MACHINE_H
#define NUANSA_WEBSOCKET_STATE_MACHINE_H

#include "nuansa/pch/pch.h"
#include "nuansa/handler/websocket_client.h"
#include "nuansa/messages/message_types.h"
#include "nuansa/handler/websocket_server.h"

namespace nuansa::handler {

	class WebSocketStateMachine {
	public:
		WebSocketStateMachine(std::shared_ptr<WebSocketClient> client, std::shared_ptr<WebSocketServer> server);
		void ProcessMessage(const nlohmann::json& msgData);
		ClientState GetCurrentState() const { return state; }
		void HandleAuthMessage(const nlohmann::json& msgData);
		void TransitionTo(ClientState newState);
		bool HandleLogin(const nlohmann::json& msgData);

	private:
		std::shared_ptr<WebSocketClient> client;
		ClientState state;
		std::shared_ptr<WebSocketServer> websocketServer;

		// State handlers
		void HandleInitialState(const nlohmann::json& msgData);
		void HandleAwaitingAuthState(const nlohmann::json& msgData);
		void HandleAuthenticatedState(const nlohmann::json& msgData);

		// Message handlers
		void HandleRegistration(const nlohmann::json& msgData);
		void HandleAuth(const std::string& message);
		void HandleLogout();
		void HandleNewMessage(const nlohmann::json& msgData);
		void HandleEditMessage(const nlohmann::json& msgData);
		void HandleDeleteMessage(const nlohmann::json& msgData);
		void HandleDirectMessage(const nlohmann::json& msgData);
		void HandlePluginMessage(const nlohmann::json& msgData);

		// Helper methods
		void AddAuthenticatedClient();
		void SendAuthRequiredMessage();
		void SendErrorMessage(const std::string& message);
		void SendMessage(const std::string& message);
	};

} // namespace nuansa::handler

#endif // NUANSA_WEBSOCKET_STATE_MACHINE_H