//
// Created by I Gede Panca Sutresna on 05/12/24.
//
#include "nuansa/pch/pch.h"
#include "nuansa/handler/websocket_handler.h"
#include "nuansa/handler/websocket_state_machine.h"

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace http = beast::http;

namespace nuansa::handler {
    WebSocketHandler::WebSocketHandler(std::shared_ptr<WebSocketServer> server)
        : websocketServer(server) {
    }

    void WebSocketHandler::HandleSession(std::shared_ptr<websocket::stream<tcp::socket> > ws) {
        std::shared_ptr<WebSocketClient> client;
        try {
            BOOST_LOG_TRIVIAL(debug) << "Starting new WebSocket session";

            // Set timeout options
            BOOST_LOG_TRIVIAL(debug) << "Setting timeout options";
            ws->set_option(websocket::stream_base::timeout::suggested(
                beast::role_type::server));

            // Set a decorator to handle the WebSocket handshake
            BOOST_LOG_TRIVIAL(debug) << "Setting WebSocket decorator";
            ws->set_option(websocket::stream_base::decorator(
                [](websocket::response_type &res) {
                    res.set(http::field::server, "Kudeta WebSocket Server");
                    res.set(http::field::access_control_allow_origin, "*");
                }));

            BOOST_LOG_TRIVIAL(debug) << "Generating client UUID";
            boost::uuids::random_generator generator;
            boost::uuids::uuid uuid = generator();
            std::string clientId = boost::uuids::to_string(uuid);
            BOOST_LOG_TRIVIAL(debug) << "Generated client ID: " << clientId;

            BOOST_LOG_TRIVIAL(debug) << "Creating WebSocket client and state machine";
            client = std::make_shared<WebSocketClient>(clientId, ws);
            auto stateMachine = std::make_shared<WebSocketStateMachine>(client, websocketServer);

            beast::flat_buffer buffer;
            BOOST_LOG_TRIVIAL(debug) << "Entering message processing loop";
            while (stateMachine->GetCurrentState() != ClientState::Disconnected) {
                boost::system::error_code ec;
                buffer.consume(buffer.size());

                BOOST_LOG_TRIVIAL(debug) << "Waiting for next message";
                // Add timeout for read operations
                ws->read(buffer, ec);

                if (ec) {
                    if (ec != websocket::error::closed) {
                        BOOST_LOG_TRIVIAL(error) << "Read error: " << ec.message();
                    }
                    BOOST_LOG_TRIVIAL(debug) << "Breaking message loop due to error: " << ec.message();
                    break;
                }

                std::string message = beast::buffers_to_string(buffer.data());
                BOOST_LOG_TRIVIAL(debug) << "Received message: " << message;

                try {
                    BOOST_LOG_TRIVIAL(debug) << "Parsing message JSON";
                    nlohmann::json msgData = nlohmann::json::parse(message);
                    BOOST_LOG_TRIVIAL(debug) << "Processing message through state machine";
                    stateMachine->ProcessMessage(msgData);
                } catch (const nlohmann::json::exception &e) {
                    BOOST_LOG_TRIVIAL(error) << "JSON parsing error: " << e.what();
                    BOOST_LOG_TRIVIAL(debug) << "Sending error message to client";
                    SendErrorMessage(client, "Invalid message format");
                }
            }

            // Perform clean shutdown
            BOOST_LOG_TRIVIAL(debug) << "Performing clean WebSocket shutdown";
            ws->close(websocket::close_code::normal);
        } catch (const beast::system_error &se) {
            if (se.code() != websocket::error::closed) {
                BOOST_LOG_TRIVIAL(error) << "WebSocket error: " << se.code().message();
            }
            BOOST_LOG_TRIVIAL(debug) << "Caught beast::system_error: " << se.what();
        } catch (const std::exception &e) {
            BOOST_LOG_TRIVIAL(error) << "Session error: " << e.what();
            BOOST_LOG_TRIVIAL(debug) << "Caught std::exception: " << e.what();
        }
        // Ensure client cleanup happens
        if (client) {
            BOOST_LOG_TRIVIAL(debug) << "Cleaning up client connection";
            HandleClientDisconnection(client);
        }
    }

    void WebSocketHandler::SendMessage(std::shared_ptr<WebSocketClient> client,
                                       const std::string &message) {
        if (!client || !client->GetWebSocket()) {
            BOOST_LOG_TRIVIAL(error) << "Invalid client";
            return;
        }

        try {
            auto ws = client->GetWebSocket();
            ws->text(true);
            ws->write(net::buffer(message));
        } catch (const std::exception &e) {
            BOOST_LOG_TRIVIAL(error) << "Error sending message: " << e.what();
        }
    }

    void WebSocketHandler::SendErrorMessage(std::shared_ptr<WebSocketClient> client,
                                            const std::string &errorMessage,
                                            const std::string &errorCode) {
        nlohmann::json errorJson = {
            {"type", "error"},
            {"code", errorCode},
            {"message", errorMessage}
        };
        SendMessage(client, errorJson.dump());
    }

    void WebSocketHandler::HandleClientDisconnection(std::shared_ptr<WebSocketClient> client) {
        if (!client) return;

        // Find and remove the client from the clients map
        auto it = websocketServer->clients.find(client->username);
        if (it != websocketServer->clients.end()) {
            websocketServer->clients.erase(it);

            BOOST_LOG_TRIVIAL(info) << "Client disconnected: " << client->username;

            // Broadcast disconnection message
            nlohmann::json disconnectMsg = {
                {"type", "system"},
                {"content", client->username + " has disconnected"}
            };

            BroadcastMessage("system", disconnectMsg.dump());
        }
    }

    void WebSocketHandler::BroadcastMessage(const std::string &sender, const std::string &message) {
        nlohmann::json broadcastMsg = {
            {"type", "broadcast"},
            {"sender", sender},
            {"content", message}
        };

        std::string msgStr = broadcastMsg.dump();

        for (auto &[username, client]: websocketServer->clients) {
            try {
                auto ws = client.GetWebSocket();
                if (ws) {
                    ws->text(true);
                    ws->write(net::buffer(msgStr));
                    BOOST_LOG_TRIVIAL(debug) << "Broadcast message sent to " << username;
                }
            } catch (const std::exception &e) {
                BOOST_LOG_TRIVIAL(error) << "Error broadcasting to " << username << ": " << e.what();
            }
        }
    }

    void WebSocketHandler::NotifyMentionedUsers(const nuansa::messages::Message &msg) {
        nlohmann::json notification = {
            {"type", "mention"},
            {"messageId", msg.id},
            {"sender", msg.sender},
            {"content", msg.content}
        };

        for (const auto &mention: msg.mentions) {
            BOOST_LOG_TRIVIAL(debug) << "Attempting to notify user: " << mention;

            if (auto it = websocketServer->clients.find(mention);
                it != websocketServer->clients.end()) {
                try {
                    auto &client = it->second;
                    auto ws = client.GetWebSocket();
                    if (ws) {
                        ws->text(true);
                        ws->write(net::buffer(notification.dump()));
                        BOOST_LOG_TRIVIAL(debug) << "Notification sent to " << mention;
                    }
                } catch (const std::exception &e) {
                    BOOST_LOG_TRIVIAL(error) << "Error sending notification to "
                                           << mention << ": " << e.what();
                }
            } else {
                BOOST_LOG_TRIVIAL(debug) << "Mentioned user " << mention
                                       << " not found or offline";
            }
        }
    }

    std::vector<std::string> WebSocketHandler::ExtractMentions(const std::string &content) {
        std::vector<std::string> mentions;
        std::regex mention_pattern("@(\\w+)");

        auto words_begin = std::sregex_iterator(content.begin(), content.end(), mention_pattern);
        auto words_end = std::sregex_iterator();

        for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
            std::smatch match = *i;
            mentions.push_back(match[1].str());
        }
        return mentions;
    }

    void WebSocketHandler::SendAuthRequiredMessage(std::shared_ptr<WebSocketClient> client) {
        nlohmann::json response = {
            {"type", nuansa::messages::MessageType::AuthRequired},
            {"message", "Authentication required"}
        };
        SendMessage(client, response.dump());
    }

    void WebSocketHandler::SendSystemMessage(std::shared_ptr<WebSocketClient> client,
                                             const std::string &message) {
        nlohmann::json systemMsg = {
            {"type", "system"},
            {"content", message}
        };
        SendMessage(client, systemMsg.dump());
    }

    void WebSocketHandler::SendAckMessage(std::shared_ptr<WebSocketClient> client,
                                          const std::string &messageId,
                                          bool success,
                                          const std::string &details) {
        nlohmann::json ackMsg = {
            {"type", "ack"},
            {"messageId", messageId},
            {"success", success}
        };

        if (!details.empty()) {
            ackMsg["details"] = details;
        }

        SendMessage(client, ackMsg.dump());
    }

    void WebSocketHandler::ValidateMessageFormat(const nlohmann::json &msgData) {
        // Check required fields based on message type
        auto type = msgData["type"].get<nuansa::messages::MessageType>();

        switch (type) {
            case nuansa::messages::MessageType::New:
                if (!msgData.contains("content")) {
                    throw std::invalid_argument("New message must contain 'content' field");
                }
                break;

            case nuansa::messages::MessageType::Edit:
                if (!msgData.contains("id") || !msgData.contains("content")) {
                    throw std::invalid_argument("Edit message must contain 'id' and 'content' fields");
                }
                break;

            case nuansa::messages::MessageType::Delete:
                if (!msgData.contains("id")) {
                    throw std::invalid_argument("Delete message must contain 'id' field");
                }
                break;

            case nuansa::messages::MessageType::DirectMessage:
                if (!msgData.contains("recipient") || !msgData.contains("content")) {
                    throw std::invalid_argument("Direct message must contain 'recipient' and 'content' fields");
                }
                break;

            default:
                break;
        }
    }

    bool WebSocketHandler::IsUserOnline(const std::string &username) {
        return websocketServer->clients.find(username) != websocketServer->clients.end();
    }

    std::size_t WebSocketHandler::GetOnlineUserCount() {
        return websocketServer->clients.size();
    }

    std::vector<std::string> WebSocketHandler::GetOnlineUsers() {
        std::vector<std::string> onlineUsers;
        onlineUsers.reserve(websocketServer->clients.size());

        for (const auto &[username, _]: websocketServer->clients) {
            onlineUsers.push_back(username);
        }

        return onlineUsers;
    }

    void WebSocketHandler::SendOnlineUsersList(std::shared_ptr<WebSocketClient> client) {
        nlohmann::json userListMsg = {
            {"type", "userList"},
            {"users", GetOnlineUsers()},
            {"count", GetOnlineUserCount()}
        };

        SendMessage(client, userListMsg.dump());
    }

    void WebSocketHandler::SendTypingNotification(const std::string &username,
                                                  bool isTyping,
                                                  const std::string &recipient) {
        nlohmann::json typingMsg = {
            {"type", "typing"},
            {"username", username},
            {"isTyping", isTyping}
        };

        if (recipient.empty()) {
            // Broadcast to all users
            BroadcastMessage("system", typingMsg.dump());
        } else {
            // Send to specific recipient
            if (auto it = websocketServer->clients.find(recipient);
                it != websocketServer->clients.end()) {
                SendMessage(std::make_shared<WebSocketClient>(it->second), typingMsg.dump());
            }
        }
    }
}
