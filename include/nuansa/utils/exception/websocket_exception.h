//
// Created by I Gede Panca Sutresna on 01/12/24.
//

#ifndef NUANSA_UTILS_EXCEPTION_WEBSOCKET_EXCEPTION_H
#define NUANSA_UTILS_EXCEPTION_WEBSOCKET_EXCEPTION_H

#include <stdexcept>

#include "nuansa/utils/pch.h"
#include "nuansa/utils/exception/exception.h"

namespace nuansa::utils::exception {
	class WebSocketException : public nuansa::utils::exception::Exception {
	public:
		explicit WebSocketException(const std::string &message)
			: Exception(message) {
		}
	};

	class WebSocketAcceptException final : public WebSocketException {
	public:
		explicit WebSocketAcceptException(const std::string &message)
			: WebSocketException("WebSocket accept error:" + message) {
		}
	};

	class WebSocketHandshakeException final : public WebSocketException {
	public:
		explicit WebSocketHandshakeException(const std::string &message)
			: WebSocketException("WebSocket handshake error:" + message) {
		}
	};

	class WebSocketReadException final : public WebSocketException {
	public:
		explicit WebSocketReadException(const std::string &message)
			: WebSocketException("WebSocket read error:" + message) {
		}
	};

	class WebSocketWriteException final : public WebSocketException {
	public:
		explicit WebSocketWriteException(const std::string &message)
			: WebSocketException("WebSocket write error:" + message) {
		}
	};

	class WebSocketCloseException final : public WebSocketException {
	public:
		explicit WebSocketCloseException(const std::string &message)
			: WebSocketException("WebSocket close error:" + message) {
		}
	};

	class WebSocketFrameException final : public WebSocketException {
	public:
		explicit WebSocketFrameException(const std::string &message)
			: WebSocketException("WebSocket frame error:" + message) {
		}
	};

	class WebSocketHandshakeTimeoutException final : public WebSocketException {
	public:
		explicit WebSocketHandshakeTimeoutException(const std::string &message)
			: WebSocketException("WebSocket handshake timeout:" + message) {
		}
	};

	class WebSocketConnectionException final : public WebSocketException {
	public:
		explicit WebSocketConnectionException(const std::string &message)
			: WebSocketException("WebSocket connection error:" + message) {
		}
	};
}

#endif //NUANSA_UTILS_EXCEPTION_WEBSOCKET_EXCEPTION_H
