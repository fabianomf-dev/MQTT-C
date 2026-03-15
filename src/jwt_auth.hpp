#pragma once

#include <jwt-cpp/jwt.h>
#include <string>
#include <chrono>
#include <optional>

class JwtAuth {
public:

    JwtAuth(const std::string& secret,
            const std::string& issuer,
            int expiration_minutes = 1296000)// 15 dias
        : secret_(secret),
          issuer_(issuer),
          expiration_minutes_(expiration_minutes) {}

    // ───────────── Criar token ─────────────
    std::string create_token(const std::string& subject) const {

        auto now = std::chrono::system_clock::now();

        return jwt::create()
            .set_issuer(issuer_)
            .set_subject(subject)
            .set_issued_at(now)
            .set_expires_at(now + std::chrono::minutes(expiration_minutes_))
            .sign(jwt::algorithm::hs256{secret_});
    }

    // ───────────── Verificar token ─────────────
    bool verify_token(const std::string& token) const {

        try {

            auto decoded = jwt::decode(token);

            auto verifier = jwt::verify()
                .allow_algorithm(jwt::algorithm::hs256{secret_})
                .with_issuer(issuer_);

            verifier.verify(decoded);

            return true;

        } catch (...) {
            return false;
        }
    }

    // ───────────── Extrair subject ─────────────
    std::optional<std::string> get_subject(const std::string& token) const {

        try {

            auto decoded = jwt::decode(token);
            return decoded.get_subject();

        } catch (...) {
            return std::nullopt;
        }
    }

private:

    std::string secret_;
    std::string issuer_;
    int expiration_minutes_;
};