#pragma once

#include <nlohmann/json.hpp>
#include <string>

using json = nlohmann::json;

// ─────────────────────────────────────────────────────────────────────────────
// IExchange
//
// Interface base para todas as exchanges.
// Qualquer nova exchange deve herdar desta classe e implementar
// create_signature().
// ─────────────────────────────────────────────────────────────────────────────

class IExchange {
public:
    virtual ~IExchange() = default;

    virtual json create_signature(
        const std::string& method,
        const std::string& endpoint,
        const json& payload) = 0;
};
