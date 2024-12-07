//
// Created by I Gede Panca Sutresna on 07/12/24.
//

#ifndef NUANSA_UTILS_EXCEPTION_MESSAGE_EXCEPTION_H
#define NUANSA_UTILS_EXCEPTION_MESSAGE_EXCEPTION_H

#include "nuansa/utils/exception/exception.h"

namespace nuansa::utils::exception {
	class MessageException : public nuansa::utils::exception::Exception {
	public:
		explicit MessageException(const std::string &message)
			: nuansa::utils::exception::Exception(message) {
		}
	};

	class MessageParsingException final : public MessageException {
	public:
		explicit MessageParsingException(const std::string &message)
			: MessageException("Failed to parse message : " + message) {
		}
	};
}

#endif //NUANSA_UTILS_EXCEPTION_MESSAGE_EXCEPTION_H
