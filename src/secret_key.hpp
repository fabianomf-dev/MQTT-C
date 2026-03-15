#pragma once

#include <openssl/rand.h>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <stdexcept>

class SecretKeyGenerator {
public:

    // ───────────── gerar chave em HEX ─────────────
    static std::string generate_hex(size_t bytes = 32) {

        std::vector<unsigned char> buffer(bytes);

        if (RAND_bytes(buffer.data(), buffer.size()) != 1) {
            throw std::runtime_error("Erro ao gerar random seguro");
        }

        std::stringstream ss;

        for (auto b : buffer) {
            ss << std::hex
               << std::setw(2)
               << std::setfill('0')
               << static_cast<int>(b);
        }

        return ss.str();
    }

    // ───────────── gerar chave raw bytes ─────────────
    static std::vector<unsigned char> generate_bytes(size_t bytes = 32) {

        std::vector<unsigned char> buffer(bytes);

        if (RAND_bytes(buffer.data(), buffer.size()) != 1) {
            throw std::runtime_error("Erro ao gerar random seguro");
        }

        return buffer;
    }

};