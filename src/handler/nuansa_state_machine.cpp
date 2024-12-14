//
// Created by I Gede Panca Sutresna on 05/12/24.
//
#include <utility>

#include "nuansa/utils/pch.h"

#include "nuansa/handler/websocket_client.h"
#include "nuansa/handler/websocket_state_machine.h"
#include "nuansa/services/auth/auth_service.h"
#include "nuansa/services/auth/register_message.h"
#include "nuansa/messages/message_types.h"

using namespace nuansa::messages;
using namespace nuansa::utils::common;

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
            auto type = msgData[MESSAGE_HEADER][MESSAGE_HEADER_MESSAGE_TYPE].get<std::string>();

            LOG_DEBUG << "Message Type: " << type;

            // Handle authentication separately
            if (type == MessageTypeToString(MessageType::Login) ||
                type == MessageTypeToString(MessageType::Register)) {
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
        if (const auto type = msgData[MESSAGE_HEADER][MESSAGE_HEADER_MESSAGE_TYPE].get<nuansa::messages::MessageType>();
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
            auto messageHeader = nuansa::messages::MessageHeader::FromJson(msgData[MESSAGE_HEADER]);
            LOG_DEBUG << "Done Getting message header";

            // Get auth provider from message data
            auto authProvider = msgData[MESSAGE_BODY].value("authProvider", 
                static_cast<int>(nuansa::services::auth::AuthProvider::Custom));
            auto provider = static_cast<nuansa::services::auth::AuthProvider>(authProvider);

            nuansa::services::auth::RegisterRequest registrationRequest;

            if (provider == nuansa::services::auth::AuthProvider::Custom) {
                // Handle custom registration
                auto username = msgData[MESSAGE_BODY]["username"].get<std::string>();
                auto email = msgData[MESSAGE_BODY]["email"].get<std::string>();
                auto password = msgData[MESSAGE_BODY]["password"].get<std::string>();

                LOG_DEBUG << "Processing custom registration request for user: " << username;

                registrationRequest = nuansa::services::auth::RegisterRequest(
                    messageHeader,
                    username,
                    email,
                    password,
                    provider
                );
            } else {
                // Handle OAuth registration (Google or GitHub)
                const auto& oauthData = msgData["body"]["oauthCredentials"];
                
                nuansa::services::auth::OAuthCredentials credentials{
                    oauthData.value("accessToken", ""),
                    oauthData.value("refreshToken", ""),
                    oauthData.value("scope", ""),
                    oauthData.value("expiresIn", 0)
                };

                // Add OAuth flow fields
                if (oauthData.contains("code")) {
                    credentials.code = oauthData["code"].get<std::string>();
                }
                if (oauthData.contains("redirectUri")) {
                    credentials.redirectUri = oauthData["redirectUri"].get<std::string>();
                }

                // Add optional OAuth fields
                if (oauthData.contains("idToken")) {
                    credentials.idToken = oauthData["idToken"].get<std::string>();
                }
                if (oauthData.contains("tokenType")) {
                    credentials.tokenType = oauthData["tokenType"].get<std::string>();
                }
                if (oauthData.contains("expiresAt")) {
                    credentials.expiresAt = oauthData["expiresAt"].get<int64_t>();
                }

                LOG_DEBUG << "Processing OAuth registration request with provider: " 
                         << static_cast<int>(provider);

                registrationRequest = nuansa::services::auth::RegisterRequest(
                    messageHeader,
                    provider,
                    credentials
                );
            }

            // Get auth service instance and register
            auto& authService = nuansa::services::auth::AuthService::GetInstance();
            auto response = authService.Register(registrationRequest);

            // Prepare response JSON
            nlohmann::json responseJson = {
                {
                    MESSAGE_HEADER, {
                        {MESSAGE_HEADER_VERSION, "1.0"},
                        {MESSAGE_HEADER_MESSAGE_TYPE, "register"},
                        {MESSAGE_HEADER_MESSAGE_ID, messageHeader.messageId},
                        {MESSAGE_HEADER_CORRELATION_ID, messageHeader.correlationId},
                        {MESSAGE_HEADER_TIMESTAMP, std::time(nullptr)}
                    }
                },
                {
                    MESSAGE_BODY, {
                        {"success", response.IsSuccess()},
                        {"message", response.GetMessage()}
                    }
                }
            };

            if (response.IsSuccess()) {
                LOG_INFO << "User registered successfully";
            } else {
                LOG_WARNING << "Registration failed: " << response.GetMessage();
            }

            SendMessage(responseJson.dump());
        } catch (const std::exception& e) {
            LOG_ERROR << "Error during registration: " << e.what();

            nlohmann::json errorResponse = {
                {
                    MESSAGE_HEADER, {
                        {MESSAGE_HEADER_VERSION, "1.0"},
                        {MESSAGE_HEADER_MESSAGE_TYPE, "register"},
                        {MESSAGE_HEADER_TIMESTAMP, std::time(nullptr)}
                    }
                },
                {
                    MESSAGE_BODY, {
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
            nuansa::services::auth::AuthRequest authRequest{
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
                client->authStatus = nuansa::services::auth::AuthStatus::Authenticated;

                // Add token to response
                response["token"] = token;

                // Send success response
                SendMessage(response.dump());
                return true;
            } else {
                LOG_WARNING << "Authentication failed for user: " << username;

                // Update client state
                client->authStatus = nuansa::services::auth::AuthStatus::NotAuthenticated;
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

            LOG_DEBUG << "Message sent: " << msgData;
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
