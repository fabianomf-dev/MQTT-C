#include "poloniex.hpp"
#include "../firebase_writer.hpp"
#include <boost/interprocess/ipc/message_queue.hpp>
#include <chrono>
#include <cmath>
#include <ctime>
#include <curl/curl.h>
#include <iostream>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <string>

#include <cstdlib>
#include <openssl/bio.h>
#include <openssl/buffer.h>

using json = nlohmann::json;

RestPoloniex::RestPoloniex(FirebaseWriter* writer)
    : m_writer(writer)
{
    const char* key    = std::getenv("POLONIEX_API_KEY");
    const char* secret = std::getenv("POLONIEX_API_SECRET");
    apikey    = key    ? key    : "";
    secretKey = secret ? secret : "";
}

std::string RestPoloniex::hmac_sha256(const std::string &data) {
  unsigned char result[EVP_MAX_MD_SIZE];
  unsigned int result_len = 0;

  // Calcula HMAC usando a chave secreta e dados
  HMAC(EVP_sha256(), reinterpret_cast<const unsigned char *>(secretKey.c_str()),
       secretKey.length(),
       reinterpret_cast<const unsigned char *>(data.c_str()), data.length(),
       result, &result_len);

  // Agora base64-encode do resultado binário
  return base64_encode(reinterpret_cast<const unsigned char *>(result),
                       result_len);
}

std::string RestPoloniex::base64_encode(const unsigned char *buffer,
                                        size_t length) {
  BIO *bio, *b64;
  BUF_MEM *bufferPtr;

  b64 = BIO_new(BIO_f_base64());
  bio = BIO_new(BIO_s_mem());
  bio = BIO_push(b64, bio);

  BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL); // Não usar newline

  BIO_write(bio, buffer, length);
  BIO_flush(bio);

  BIO_get_mem_ptr(bio, &bufferPtr);
  std::string encoded_data(bufferPtr->data, bufferPtr->length);

  BIO_free_all(bio);

  return encoded_data;
}

json RestPoloniex::create_signature(
    const std::string& method,
    const std::string& endpoint,
    const json& payload)
{
    try {

        auto now = std::chrono::system_clock::now();
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(
                          now.time_since_epoch())
                          .count();

        std::string timestamp = std::to_string(millis);

        if (method == "POST") {

            json requestBody;

            if (payload.contains("coin"))       requestBody["coin"]       = payload["coin"];
            if (payload.contains("network"))    requestBody["network"]    = payload["network"];
            if (payload.contains("amount"))     requestBody["amount"]     = payload["amount"];
            if (payload.contains("address"))    requestBody["address"]    = payload["address"];
            if (payload.contains("addressTag")) requestBody["addressTag"] = payload["addressTag"];

            std::string requestBodyStr = requestBody.dump();

            std::cout << requestBodyStr << std::endl;

            std::string queryString =
                "requestBody=" + requestBodyStr +
                "&signTimestamp=" + timestamp;

            std::string signatureBase =
                method + "\n" +
                endpoint + "\n" +
                queryString;

            std::string signature = hmac_sha256(signatureBase);
            m_writer->writeLog(
                "INFO",
                "create_signature",
                "Signature created",
                {{"method", method}, {"endpoint", endpoint}}
            );
            return json{
                {"timestamp", timestamp},
                {"signature", signature}
            };
        }

        else if (method == "GET") {

            std::string queryString =
                "signTimestamp=" + timestamp;

            std::string signatureBase =
                method + "\n" +
                endpoint + "\n" +
                queryString;

            std::string signature = hmac_sha256(signatureBase);

            return json{
                {"timestamp", timestamp},
                {"signature", signature}
            };
        }

        else {
            throw std::invalid_argument("Unsupported HTTP method: " + method);
        }

    } catch (const std::exception& e) {

        std::cerr << e.what() << std::endl;
        return {};
    }
}