//
// Created by I Gede Panca Sutresna on 07/12/24.
//

#ifndef NUANSA_UTILS_EXCEPTION_EXCEPTION_H
#define NUANSA_UTILS_EXCEPTION_EXCEPTION_H
#include <string>

namespace nuansa::utils::exception {
	class Exception : public std::exception {
	private:
		std::string message_;

	public:
		explicit Exception(const std::string &message) : message_(message) {
		}

		// Override what to return our custom message
		const char *what() const noexcept override {
			return message_.c_str();
		}
	};
}

#endif //NUANSA_UTILS_EXCEPTION_EXCEPTION_H
