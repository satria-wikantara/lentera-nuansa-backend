#ifndef NUANSA_UTILS_RANDOM_GENERATOR_H
#define NUANSA_UTILS_RANDOM_GENERATOR_H

#include "nuansa/utils/pch.h"

namespace nuansa::utils {
    class RandomGenerator {
    public:
        // Generate UUID string
        static std::string GenerateUUID();
        
        // Generate random string with custom length and charset
        static std::string GenerateString(
            size_t length,
            const std::string& charset = 
                "0123456789"
                "abcdefghijklmnopqrstuvwxyz"
                "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
        
        // Generate random hex string
        static std::string GenerateHexString(size_t length);

    private:
        static std::random_device rd_;
        static std::mt19937 gen_;
    };
}

#endif // NUANSA_UTILS_RANDOM_GENERATOR_H 