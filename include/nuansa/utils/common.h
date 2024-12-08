//
// Created by I Gede Panca Sutresna on 01/12/24.
//

#ifndef NUANSA_UTILS_COMMON_H
#define NUANSA_UTILS_COMMON_H

namespace nuansa::utils::common {
	inline constexpr auto VERSION = "0.0.1";
	inline constexpr auto CONFIG_FILE_OPTION = "config,c";
	inline constexpr auto DEFAULT_CONFIG_FILE = "config.yml";

	inline constexpr auto MESSAGE_HEADER = "head";
	inline constexpr auto MESSAGE_HEADER_VERSION = "ver";
	inline constexpr auto MESSAGE_HEADER_MESSAGE_ID = "msg_id";
	inline constexpr auto MESSAGE_HEADER_SENDER = "sender";
	inline constexpr auto MESSAGE_HEADER_TIMESTAMP = "timestamp";
	inline constexpr auto MESSAGE_HEADER_MESSAGE_TYPE = "msg_type";
	inline constexpr auto MESSAGE_HEADER_CORRELATION_ID = "corr_id";
	inline constexpr auto MESSAGE_HEADER_PRIORITY = "priority";
	inline constexpr auto MESSAGE_HEADER_CONTENT_TYPE = "content_type";
	inline constexpr auto MESSAGE_HEADER_ENCODING = "encoding";
	inline constexpr auto MESSAGE_HEADER_CONTENT_LENGTH = "content_length";
	inline constexpr auto MESSAGE_HEADER_HASH = "hash";
	inline constexpr auto MESSAGE_HEADER_CUSTOM_HEADERS = "custom_headers";


	inline constexpr auto MESSAGE_BODY = "body";
	inline constexpr auto MESSAGE_FOOTER = "foot";
} // namespace nuansa::utils

#endif  // NUANSA_UTILS_COMMON_H
