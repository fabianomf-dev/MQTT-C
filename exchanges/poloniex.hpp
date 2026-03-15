#pragma once

#include "iexchange.hpp"
#include <nlohmann/json.hpp>
#include <string>

using json = nlohmann::json;
class FirebaseWriter;

class RestPoloniex : public IExchange {
private:
  std::string apikey;
  std::string secretKey;
  FirebaseWriter* m_writer;

  static size_t curl_write_callback(void *contents, size_t size, size_t nmemb,
                                    std::string *output);
  static size_t WriteCallback(void *contents, size_t size, size_t nmemb,
                              void *userp);
  std::string base64_encode(const unsigned char *buffer, size_t length);

  std::string hmac_sha256(const std::string &data);

public:
  explicit RestPoloniex(FirebaseWriter* writer);
  json create_signature(const std::string &method, const std::string &endpoint,
                        const json& payload) override;
};
