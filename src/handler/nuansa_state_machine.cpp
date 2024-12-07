//
// Created by I Gede Panca Sutresna on 05/12/24.
//
#include <utility>

#include "nuansa/pch/pch.h"

#include "nuansa/handler/websocket_client.h"
#include "nuansa/handler/websocket_handler.h"
#include "nuansa/handler/websocket_state_machine.h"
#include "nuansa/services/auth/auth_service.h"
#include "nuansa/messages/message_types.h"

using namespace nuansa::messages;

namespace nuansa::handler {
    WebSocketStateMachine::WebSocketStateMachine(
        std::shared_ptr<WebSocketClient> client,
        std::shared_ptr<WebSocketServer> server)
        : client(std::move(client)),
          state(ClientState::Initial),
          websocketServer(std::move(server)) {
    }

    void WebSocketStateMachine::ProcessMessage(const nlohmann::json &msgData) {
        try {
            auto type = msgData["header"]["messageType"].get<std::string>();

            LOG_DEBUG << "Message Type: " << type;

            // Handle authentication separately
            if (type == ToString(MessageType::Login) ||
                type == ToString(MessageType::Register)) {
                HandleAuthMessage(msgData);
                return;
            }

            // Process message based on current state
            switch (client->GetState()) {
                case ClientState::Initial:
                    HandleInitialState(msgData);
                    break;

                case ClientState::AwaitingAuth:
                    HandleAwaitingAuthState(msgData);
                    break;

                case ClientState::Authenticated:
                    HandleAuthenticatedState(msgData);
                    break;

                default:
                    LOG_WARNING << "Invalid state";
                    break;
            }
        } catch (const std::exception &e) {
            LOG_ERROR << "Error processing message: " << e.what();
            SendErrorMessage("Error processing message");
        }
    }

    void WebSocketStateMachine::HandleAuthMessage(const nlohmann::json &msgData) {
        LOG_DEBUG << "WebSocketStateMachine::HandleAuthMessage";
        if (const auto type = msgData["header"]["messageType"].get<nuansa::messages::MessageType>();
            type == nuansa::messages::MessageType::Register) {
            HandleRegistration(msgData);
            TransitionTo(ClientState::AwaitingAuth);
        } else if (type == nuansa::messages::MessageType::Login) {
            if (HandleLogin(msgData)) {
                TransitionTo(ClientState::Authenticated);
                AddAuthenticatedClient();
            } else {
                TransitionTo(ClientState::AwaitingAuth);
            }
        }
    }

    void WebSocketStateMachine::HandleRegistration(const nlohmann::json &msgData) const {
        try {
            LOG_DEBUG << "Getting message header";
            // Extract header from the nested structure
            auto messageHeader = nuansa::messages::MessageHeader::FromJson(msgData["header"]);

            LOG_DEBUG << "Done Getting message header";

            // Extract data from the nested structure
            auto username = msgData["payload"]["username"].get<std::string>();
            auto email = msgData["payload"]["email"].get<std::string>();
            auto password = msgData["payload"]["password"].get<std::string>();

            LOG_DEBUG << "Processing registration request for user: " << username;

            // Create registration request
            nuansa::messages::RegisterRequest registrationRequest{
                messageHeader,
                username,
                email,
                password
            };

            // Get auth service instance and register
            auto &authService = nuansa::services::auth::AuthService::GetInstance();
            auto [success, token, message] = authService.Register(registrationRequest);

            // Prepare response JSON with similar structure
            nlohmann::json response = {
                {
                    "header", {
                        {"version", "1.0"},
                        {"messageType", "register"},
                        {"messageId", messageHeader.messageId},
                        {"correlationId", messageHeader.correlationId},
                        {"timestamp", std::time(nullptr)}
                    }
                },
                {
                    "payload", {
                        {"success", success},
                        {"message", message},
                        {"token", token} // Include token if registration successful
                    }
                }
            };

            if (success) {
                LOG_INFO << "User registered successfully: " << username;
            } else {
                LOG_WARNING << "Registration failed for user: " << username;
            }
            SendMessage(response.dump());
        } catch (const std::exception &e) {
            LOG_ERROR << "Error during registration: " << e.what();

            nlohmann::json errorResponse = {
                {
                    "header", {
                        {"version", "1.0"},
                        {"messageType", "register"},
                        {"timestamp", std::time(nullptr)}
                    }
                },
                {
                    "payload", {
                        {"success", false},
                        {"message", "Internal server error during registration"}
                    }
                }
            };
            SendMessage(errorResponse.dump());
        }
    }

    // TODO: create implementation
    void WebSocketStateMachine::HandleAuth(const std::string &message) {
    }

    bool WebSocketStateMachine::HandleLogin(const nlohmann::json &msgData) const {
        try {
            auto username = msgData["username"].get<std::string>();
            auto password = msgData["password"].get<std::string>();

            LOG_DEBUG << "Processing login request for user: " << username;

            // Create auth request
            nuansa::messages::AuthRequest authRequest{
                username,
                password
            };

            // Get auth service instance and authenticate
            auto &authService = nuansa::services::auth::AuthService::GetInstance();
            auto [success, token, message] = authService.Authenticate(authRequest);

            // Prepare response JSON
            nlohmann::json response = {
                {"type", nuansa::messages::MessageType::Login},
                {"success", success},
                {"message", message}
            };

            if (success) {
                LOG_INFO << "User authenticated successfully: " << username;

                // Update client state
                client->username = username;
                client->authToken = token;
                client->authStatus = nuansa::messages::AuthStatus::Authenticated;

                // Add token to response
                response["token"] = token;

                // Send success response
                SendMessage(response.dump());
                return true;
            } else {
                LOG_WARNING << "Authentication failed for user: " << username;

                // Update client state
                client->authStatus = nuansa::messages::AuthStatus::NotAuthenticated;
                client->authToken = std::nullopt;

                // Send failure response
                SendMessage(response.dump());
                return false;
            }
        } catch (const std::exception &e) {
            LOG_ERROR << "Error during login: " << e.what();

            // Send error response
            nlohmann::json errorResponse = {
                {"type", nuansa::messages::MessageType::Login},
                {"success", false},
                {"message", "Internal server error during login"}
            };
            SendMessage(errorResponse.dump());
            return false;
        }
    }

    // Helper method to send messages
    void WebSocketStateMachine::SendMessage(const std::string &msgData) const {
        if (!client || !client->GetWebSocket()) {
            LOG_ERROR << "Cannot send message - invalid client or websocket";
            return;
        }

        try {
            const auto ws = client->GetWebSocket();
            ws->text(true);
            ws->write(net::buffer(msgData));
        } catch (const std::exception &e) {
            LOG_ERROR << "Error sending message: " << e.what();
        }
    }

    void WebSocketStateMachine::HandleInitialState(const nlohmann::json &msgData) {
        SendAuthRequiredMessage();
        TransitionTo(ClientState::AwaitingAuth);
    }

    void WebSocketStateMachine::HandleAwaitingAuthState(const nlohmann::json &msgData) {
        SendAuthRequiredMessage();
    }

    void WebSocketStateMachine::HandleAuthenticatedState(const nlohmann::json &msgData) {
        switch (msgData["type"].get<nuansa::messages::MessageType>()) {
            case nuansa::messages::MessageType::Logout:
                HandleLogout();
                TransitionTo(ClientState::AwaitingAuth);
                break;

            case nuansa::messages::MessageType::New:
                HandleNewMessage(msgData);
                break;

            case nuansa::messages::MessageType::Edit:
                HandleEditMessage(msgData);
                break;

            case nuansa::messages::MessageType::Delete:
                HandleDeleteMessage(msgData);
                break;

            case nuansa::messages::MessageType::DirectMessage:
                HandleDirectMessage(msgData);
                break;

            default:
                LOG_WARNING << "Unhandled message type";
                break;
        }
    }

    void WebSocketStateMachine::TransitionTo(ClientState newState) {
        LOG_DEBUG << "State transition: "
                            << static_cast<int>(state)
                            << " -> "
                            << static_cast<int>(newState);
        state = newState;
        client->SetState(newState);
    }

    void WebSocketStateMachine::HandleLogout() {
        // Implement logout logic
    }

    void WebSocketStateMachine::HandleNewMessage(const nlohmann::json &msgData) {
        // Implement new message handling
    }

    void WebSocketStateMachine::SendErrorMessage(const std::string &msgData) {
        // Implement error message sending
    }

    void WebSocketStateMachine::HandleEditMessage(const nlohmann::json &msgData) {
        // Implement edit message handling
    }

    void WebSocketStateMachine::HandleDeleteMessage(const nlohmann::json &msgData) {
        // Implement delete message handling
    }

    void WebSocketStateMachine::HandleDirectMessage(const nlohmann::json &msgData) {
        // Implement direct message handling
    }

    // TODO: Implement this
    void WebSocketStateMachine::HandlePluginMessage(const nlohmann::json &msgData) {
    }

    void WebSocketStateMachine::AddAuthenticatedClient() {
        // Implement client authentication logic
    }

    void WebSocketStateMachine::SendAuthRequiredMessage() {
        // Implement auth required message sending
    }
} // namespace nuansa::handler
