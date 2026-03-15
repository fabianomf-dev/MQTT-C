#pragma once

#include "iexchange.hpp"
#include "poloniex.hpp"
#include "../firebase_writer.hpp"

#include <memory>
#include <stdexcept>
#include <string>

// ─────────────────────────────────────────────────────────────────────────────
// ExchangeFactory
//
// Cria a exchange correta com base no nome recebido no campo "exchange" do
// comando. Para adicionar uma nova exchange basta incluir o header e
// acrescentar mais um else-if abaixo.
//
// Exemplo de uso:
//   auto api = ExchangeFactory::create("poloniex", m_writer);
//   json result = api->create_signature(method, endpoint, payload);
// ─────────────────────────────────────────────────────────────────────────────

class ExchangeFactory {
public:
    static std::unique_ptr<IExchange> create(const std::string& name,
                                             FirebaseWriter* writer)
    {
        if (name == "poloniex")
            return std::make_unique<RestPoloniex>(writer);

        // ── adicione novas exchanges aqui ──────────────────────
        // if (name == "binance")
        //     return std::make_unique<RestBinance>(writer);
        // if (name == "kraken")
        //     return std::make_unique<RestKraken>(writer);

        throw std::runtime_error("Unknown exchange: '" + name + "'");
    }
};
