// http_server.h — servidor HTTP mínimo header-only para a VPS API
// substitua por cpp-httplib em produção: https://github.com/yhirose/cpp-httplib
#pragma once
#include <string>
#include <functional>
#include <map>
#include <sstream>
#include <thread>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

struct Request {
    std::string method;
    std::string path;
    std::string body;
    std::map<std::string,std::string> headers;
};

struct Response {
    int status = 200;
    std::string body;
    std::string content_type = "application/json";
};

using Handler = std::function<void(const Request&, Response&)>;

class HttpServer {
    int port_;
    std::map<std::string, Handler> routes_; // "METHOD /path"
    std::atomic<bool> running_{false};
    int server_fd_ = -1;

public:
    explicit HttpServer(int port) : port_(port) {}

    void Post(const std::string& path, Handler h) {
        routes_["POST " + path] = h;
    }
    void Get(const std::string& path, Handler h) {
        routes_["GET " + path] = h;
    }

    void listen() {
        server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
        int opt = 1;
        setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port_);
        bind(server_fd_, (sockaddr*)&addr, sizeof(addr));
        ::listen(server_fd_, 32);
        running_ = true;
        std::cout << "[http] porta " << port_ << "\n";

        while (running_) {
            int client = accept(server_fd_, nullptr, nullptr);
            if (client < 0) continue;
            std::thread([this, client]{ handle(client); }).detach();
        }
    }

    void stop() {
        running_ = false;
        if (server_fd_ >= 0) close(server_fd_);
    }

private:
    void handle(int fd) {
        char buf[8192] = {};
        ssize_t n = recv(fd, buf, sizeof(buf)-1, 0);
        if (n <= 0) { close(fd); return; }

        std::string raw(buf, n);
        Request req;

        // parse primeira linha
        std::istringstream ss(raw);
        std::string line;
        std::getline(ss, line);
        if (!line.empty() && line.back()=='\r') line.pop_back();
        std::istringstream ls(line);
        std::string ver;
        ls >> req.method >> req.path >> ver;

        // headers
        int content_length = 0;
        while (std::getline(ss, line)) {
            if (!line.empty() && line.back()=='\r') line.pop_back();
            if (line.empty()) break;
            auto colon = line.find(':');
            if (colon != std::string::npos) {
                std::string k = line.substr(0, colon);
                std::string v = line.substr(colon+2);
                req.headers[k] = v;
                if (k == "Content-Length") content_length = std::stoi(v);
            }
        }

        // body
        auto pos = raw.find("\r\n\r\n");
        if (pos != std::string::npos && content_length > 0)
            req.body = raw.substr(pos + 4, content_length);

        // route
        std::string key = req.method + " " + req.path;
        Response res;
        auto it = routes_.find(key);
        if (it != routes_.end()) {
            try { it->second(req, res); }
            catch (const std::exception& e) {
                res.status = 500;
                res.body = std::string("{\"error\":\"") + e.what() + "\"}";
            }
        } else {
            res.status = 404;
            res.body = "{\"error\":\"not found\"}";
        }

        // resposta
        std::string resp =
            "HTTP/1.1 " + std::to_string(res.status) + " OK\r\n"
            "Content-Type: " + res.content_type + "\r\n"
            "Content-Length: " + std::to_string(res.body.size()) + "\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "\r\n" + res.body;
        send(fd, resp.data(), resp.size(), 0);
        close(fd);
    }
};
