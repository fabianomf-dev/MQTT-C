#pragma once
#include <string>
#include <unordered_map>
#include <mutex>
#include <condition_variable>
#include <stdexcept>
#include <atomic>
#include <thread>
#include <chrono>
#include <iostream>
#include <random>
#include <sstream>
#include <iomanip>
#include <unistd.h>

#include "mqtt.h"
#include "posix_sockets.h"
#include "config.hpp"

// extrai string de JSON simples
static std::string jget_vps(const std::string& j, const std::string& key) {
    auto needle = "\"" + key + "\":\"";
    auto pos = j.find(needle);
    if (pos == std::string::npos) return "";
    pos += needle.size();
    auto end = j.find('"', pos);
    return end == std::string::npos ? "" : j.substr(pos, end - pos);
}

static std::string make_uuid_vps() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint32_t> dis(0, 0xFFFFFFFF);
    std::ostringstream ss;
    ss << std::hex << std::setfill('0')
       << std::setw(8) << dis(gen) << std::setw(8) << dis(gen);
    return ss.str();
}

struct PendingJob {
    std::string             result;
    bool                    done = false;
    std::mutex              mu;
    std::condition_variable cv;
};

class MQTTDispatcher {
    static constexpr size_t BUF = 16384;

    uint8_t     sendbuf_[BUF];
    uint8_t     recvbuf_[BUF];
    mqtt_client client_;
    int         sockfd_ = -1;

    std::mutex   jobs_mu_;
    std::unordered_map<std::string, PendingJob*> pending_;

    std::atomic<bool> running_{false};
    std::thread       loop_thread_;

    Config* cfg_;

public:
    explicit MQTTDispatcher(Config& cfg) : cfg_(&cfg) {}

    void start() {
        running_ = true;
        loop_thread_ = std::thread([this]{ loop(); });
    }

    void stop() {
        running_ = false;
        if (loop_thread_.joinable()) loop_thread_.join();
        if (sockfd_ >= 0) { close(sockfd_); sockfd_ = -1; }
    }

    // repassa msg intacta — job_id vem do App dentro do payload
    std::string dispatch(const std::string& body, int timeout_s) {
        std::string job_id = jget_vps(body, "job_id");

        PendingJob job;
        {
            std::lock_guard<std::mutex> lk(jobs_mu_);
            pending_[job_id] = &job;
        }
        std::cout << "[dispatch] job_id=" << job_id << " payload=" << body << "\n";
        mqtt_publish(&client_,
                     cfg_->topic_pub.c_str(),
                     body.data(), body.size(),
                     MQTT_PUBLISH_QOS_1);

        std::unique_lock<std::mutex> lk(job.mu);
        bool ok = job.cv.wait_for(lk,
            std::chrono::seconds(timeout_s),
            [&job]{ return job.done; });

        {
            std::lock_guard<std::mutex> jlk(jobs_mu_);
            pending_.erase(job_id);
        }

        if (!ok) throw std::runtime_error("Pi timeout — job_id=" + job_id);
        return job.result;
    }

private:
    void loop() {
        while (running_) {
            if (!do_connect()) {
                std::this_thread::sleep_for(std::chrono::seconds(5));
                continue;
            }
            while (running_) {
                client_.publish_response_callback_state = this;
                mqtt_sync(&client_);
                if (client_.error != MQTT_OK) {
                    std::cerr << "[mqtt] " << mqtt_error_str(client_.error)
                              << " — reconectando\n";
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
    }

    bool do_connect() {
        if (sockfd_ >= 0) { close(sockfd_); sockfd_ = -1; }
        sockfd_ = open_nb_socket(cfg_->mqtt_host.c_str(), cfg_->mqtt_port.c_str());
        if (sockfd_ < 0) {
            std::cerr << "[mqtt] falha TCP → " << cfg_->mqtt_host
                      << ":" << cfg_->mqtt_port << "\n";
            return false;
        }
        mqtt_init(&client_, sockfd_, sendbuf_, BUF, recvbuf_, BUF, on_message_cb);
        const char* u = cfg_->mqtt_user.empty() ? nullptr : cfg_->mqtt_user.c_str();
        const char* p = cfg_->mqtt_pass.empty() ? nullptr : cfg_->mqtt_pass.c_str();
        mqtt_connect(&client_, "vps-broker", nullptr, nullptr, 0,
                     u, p, MQTT_CONNECT_CLEAN_SESSION, 30);
        if (client_.error != MQTT_OK) {
            std::cerr << "[mqtt] connect erro: " << mqtt_error_str(client_.error) << "\n";
            return false;
        }
        mqtt_subscribe(&client_, cfg_->topic_sub.c_str(), 1);
        client_.publish_response_callback_state = this;
        std::cout << "[mqtt] conectado " << cfg_->mqtt_host
                  << " sub:" << cfg_->topic_sub
                  << " pub:" << cfg_->topic_pub << "\n";
        return true;
    }

    static void on_message_cb(void** state, struct mqtt_response_publish* pub) {
        auto* self = static_cast<MQTTDispatcher*>(*state);
        std::string payload(
            static_cast<const char*>(pub->application_message),
            pub->application_message_size);
        std::string job_id = jget_vps(payload, "job_id");
        if (job_id.empty()) {
            std::cerr << "[mqtt] resposta sem job_id\n"; return;
        }
        std::lock_guard<std::mutex> lk(self->jobs_mu_);
        auto it = self->pending_.find(job_id);
        if (it == self->pending_.end()) {
            std::cerr << "[mqtt] job_id desconhecido: " << job_id << "\n"; return;
        }
        PendingJob* job = it->second;
        {
            std::lock_guard<std::mutex> jlk(job->mu);
            job->result = payload;
            job->done   = true;
        }
        job->cv.notify_one();
    }
};
