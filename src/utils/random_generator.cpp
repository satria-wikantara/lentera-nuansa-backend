#include "nuansa/utils/random_generator.h"

namespace nuansa::utils {
    std::random_device RandomGenerator::rd_;
    std::mt19937 RandomGenerator::gen_(rd_());

    std::string RandomGenerator::GenerateUUID() {
        boost::uuids::random_generator gen;
        boost::uuids::uuid uuid = gen();
        return boost::uuids::to_string(uuid);
    }

    std::string RandomGenerator::GenerateString(size_t length, const std::string& charset) {
        std::uniform_int_distribution<size_t> dis(0, charset.length() - 1);
        std::string str(length, 0);
        std::generate_n(str.begin(), length, [&]() { return charset[dis(gen_)]; });
        return str;
    }

    std::string RandomGenerator::GenerateHexString(size_t length) {
        return GenerateString(length, "0123456789abcdef");
    }
} 