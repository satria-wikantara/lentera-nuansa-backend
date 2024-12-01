//
// Created by I Gede Panca Sutresna on 01/12/24.
//

#ifndef NUANSA_UTILS_ERRORS_WEBSOCKET_ERROR_H
#define NUANSA_UTILS_ERRORS_WEBSOCKET_ERROR_H

#include <stdexcept>

#include "nuansa/pch/pch.h"

namespace nuansa::utils::error {
	class WebSocketError : public std::runtime_error {
		using std::runtime_error::runtime_error;
	};

	class WebSocketAcceptError : public WebSocketError {
	public:
		explicit WebSocketAcceptError(const std::string &message)
			: WebSocketError("WebSocket accept error:" + message) {
		}
	};

	class WebSocketHandshakeError : public WebSocketError {
	public:
		explicit WebSocketHandshakeError(const std::string &message)
			: WebSocketError("WebSocket handshake error:" + message) {
		}
	};

	class WebSocketReadError : public WebSocketError {
	public:
		explicit WebSocketReadError(const std::string &message)
			: WebSocketError("WebSocket read error:" + message) {
		}
	};

	class WebSocketWriteError : public WebSocketError {
	public:
		explicit WebSocketWriteError(const std::string &message)
			: WebSocketError("WebSocket write error:" + message) {
		}
	};

	class WebSocketCloseError : public WebSocketError {
	public:
		explicit WebSocketCloseError(const std::string &message)
			: WebSocketError("WebSocket close error:" + message) {
		}
	};

	class WebSocketFrameError : public WebSocketError {
	public:
		explicit WebSocketFrameError(const std::string &message)
			: WebSocketError("WebSocket frame error:" + message) {
		}
	};

	class WebSocketHandshakeTimeoutError : public WebSocketError {
	public:
		explicit WebSocketHandshakeTimeoutError(const std::string &message)
			: WebSocketError("WebSocket handshake timeout:" + message) {
		}
	};

	class WebSocketConnectionError : public WebSocketError {
	public:
		explicit WebSocketConnectionError(const std::string &message)
			: WebSocketError("WebSocket connection error:" + message) {
		}
	};
}

#endif //NUANSA_UTILS_ERRORS_WEBSOCKET_ERROR_H
