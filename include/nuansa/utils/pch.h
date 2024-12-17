//
// Created by I Gede Panca Sutresna on 01/12/24.
//

#ifndef NUANSA_UTILS_PCH_H
#define NUANSA_UTILS_PCH_H

#include "nuansa/utils/framework.h"

// C++ Standard Library
#include <string>
#include <sstream>
#include <vector>
#include <memory>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <functional>
#include <iostream>
#include <chrono>
#include <random>
#include <thread>
#include <filesystem>
#include <mutex>
#include <optional>
#include <iomanip>
#include <queue>
#include <regex>
#include <time.h>
#include <fstream>
#include <stdexcept>
#include <ctime>

// Boost Library
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/core/detail/base64.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/program_options.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/bind_executor.hpp>
#include <yaml-cpp/yaml.h>
#include <nlohmann/json.hpp>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/rand.h>
#include <openssl/hmac.h>
#include <pqxx/pqxx>

// Project Headers
#include "nuansa/utils/common.h"
#include "nuansa/utils/log/log.h"


#endif //NUANSA_UTILS_PCH_H
