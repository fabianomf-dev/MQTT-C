#include <iostream>
#include <string>
#include <vector>
#include <fstream>

#include "config.hpp"
#include "mqtt_dispatcher.hpp"
#include "httplib.h"
#include "jwt_auth.hpp"

// ─── Helpers ──────────────────────────────────────────────────────────────────

static std::string extract_bearer(const Request& req) {
    auto it = req.headers.find("Authorization");
    if (it == req.headers.end()) return {};

    const std::string& v = it->second;
    if (v.rfind("Bearer ", 0) != 0) return {};

    std::string token = v.substr(7);
    token.erase(0, token.find_first_not_of(" \t\r\n"));
    token.erase(token.find_last_not_of(" \t\r\n") + 1);
    return token;
}

static std::string missing_field(const std::string& body) {
    for (const auto& field : { "job_id", "data", "sig" })
        if (jget_vps(body, field).empty()) return field;
    return {};
}

static void json_error(Response& res, int status, const std::string& msg) {
    res.status = status;
    res.body   = "{\"error\":\"" + msg + "\"}";
}

// ─── Main ─────────────────────────────────────────────────────────────────────

int main(int argc, char* argv[]) {
    const std::string env_path = (argc > 1) ? argv[1] : "/home/fabiano/Downloads/files (6)/vps-broker-hmac2/.env";

    // Configuração
    Config cfg;
    try {
        cfg.load(env_path);
    } catch (const std::exception& e) {
        std::cerr << "[fatal] Falha ao carregar configuração: " << e.what() << "\n";
        return 1;
    }

    // MQTT
    MQTTDispatcher mqtt(cfg);
    mqtt.start();

    // HTTP
    HttpServer svr(cfg.api_port);

    // ── GET /health ───────────────────────────────────────────────────────────
    svr.Get("/health", [](const Request&, Response& res) {
        res.body = "{\"ok\":true}";
    });

    // ── POST /sign ────────────────────────────────────────────────────────────
    svr.Post("/sign", [&](const Request& req, Response& res) {

        // 1. Autenticação
        const std::string token = extract_bearer(req);
        if (token.empty()) {
            json_error(res, 401, "token de autenticação ausente");
            return;
        }

        // 2. Validação JWT
        JwtAuth jwt(cfg.jwt_secret, "vps-broker");
        if (!jwt.verify_token(token)) {
            json_error(res, 401, "token de autenticação inválido");
            return;
        }

        // 3. Body
        if (req.body.empty()) {
            json_error(res, 400, "body vazio");
            return;
        }

        // 4. Campos obrigatórios
        const std::string missing = missing_field(req.body);
        if (!missing.empty()) {
            json_error(res, 400, "campo obrigatório ausente: " + missing);
            return;
        }

        // 5. Despacha para o Pi via MQTT
        try {
            res.body = mqtt.dispatch(req.body, cfg.pi_timeout);
        } catch (const std::exception& e) {
            json_error(res, 504, e.what());
        }
    });

    std::cout << "[api] ouvindo em http://0.0.0.0:" << cfg.api_port << "\n";
    svr.listen();

    mqtt.stop();
    std::cout << "[encerrado]\n";
    return 0;
}