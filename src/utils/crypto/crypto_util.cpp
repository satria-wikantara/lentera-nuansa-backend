//
// Created by I Gede Panca Sutresna on 05/12/24.
//
#include "nuansa/utils/pch.h"

#include "nuansa/utils/crypto/crypto_util.h"
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>

namespace nuansa::utils::crypto {
    std::string CryptoUtil::GenerateRandomSalt() {
        return GenerateSHA256Hash(std::to_string(std::random_device()()));
    }

    std::string CryptoUtil::HashPassword(const std::string &password, const std::string &salt) {
        return GenerateSHA256Hash(password + salt);
    }

    std::string CryptoUtil::GenerateSHA256Hash(const std::string &content) {
        unsigned char hash[EVP_MAX_MD_SIZE];
        unsigned int hashLen;

        // Create a new EVP_MD_CTX
        const std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> ctx(
            EVP_MD_CTX_new(), EVP_MD_CTX_free);

        if (!ctx) {
            throw std::runtime_error("Failed to create EVP_MD_CTX");
        }

        // Initialize the digest
        if (EVP_DigestInit_ex(ctx.get(), EVP_sha256(), nullptr) != 1) {
            throw std::runtime_error("Failed to initialize SHA256 digest");
        }

        // Update with the input data
        if (EVP_DigestUpdate(ctx.get(), content.c_str(), content.length()) != 1) {
            throw std::runtime_error("Failed to update digest");
        }

        // Finalize the digest
        if (EVP_DigestFinal_ex(ctx.get(), hash, &hashLen) != 1) {
            throw std::runtime_error("Failed to finalize digest");
        }

        // Convert to hex string
        std::stringstream ss;
        for (unsigned int i = 0; i < hashLen; i++) {
            ss << std::hex << std::setw(2) << std::setfill('0')
                    << static_cast<int>(hash[i]);
        }

        return ss.str();
    }

    std::string CryptoUtil::Base64Encode(const std::string& input) {
        BIO *bio, *b64;
        BUF_MEM *bufferPtr;

        b64 = BIO_new(BIO_f_base64());
        bio = BIO_new(BIO_s_mem());
        bio = BIO_push(b64, bio);

        BIO_write(bio, input.c_str(), input.length());
        BIO_flush(bio);
        BIO_get_mem_ptr(bio, &bufferPtr);

        std::string result(bufferPtr->data, bufferPtr->length - 1);  // -1 to remove newline
        BIO_free_all(bio);

        return result;
    }
}
