#ifndef NUANSA_WEBSOCKET_SERVER_H
#define NUANSA_WEBSOCKET_SERVER_H

#include "nuansa/pch/pch.h"
#include "nuansa/handler/websocket_client.h"
#include "nuansa/messages/message_types.h"

namespace nuansa::handler {

	class WebSocketServer {
	public:
		WebSocketServer() = default;

		void AddClient(const std::string& username, const WebSocketClient& client);
		void RemoveClient(const std::string& username);
		void BroadcastMessage(const std::string& message);
		void StoreMessage(const nuansa::messages::Message& message);

		// Public members for easy access (could be made private with accessors)
		std::map<std::string, WebSocketClient> clients;
		std::map<std::string, nuansa::messages::Message> messages;
	};

} // namespace nuansa::handler

#endif // NUANSA_WEBSOCKET_SERVER_H