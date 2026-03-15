#pragma once
#include <string>
#include <fstream>
#include <stdexcept>
#include <algorithm>

static inline std::string trim(std::string s) {
    auto bad = [](unsigned char c){ return c=='\r'||c=='\n'||c==' '||c=='\t'; };
    s.erase(s.begin(), std::find_if_not(s.begin(), s.end(), bad));
    s.erase(std::find_if_not(s.rbegin(), s.rend(), bad).base(), s.end());
    return s;
}
static inline std::string strip_comment(std::string s) {
    for (auto d : {" #", "\t#"}) {
        auto p = s.find(d);
        if (p != std::string::npos) s = s.substr(0, p);
    }
    return trim(s);
}

struct Config {
    std::string jwt_secret;
    std::string mqtt_host  = "localhost";
    std::string mqtt_port  = "1883";
    std::string mqtt_user;
    std::string mqtt_pass;
    std::string topic_pub  = "pi/jobs";
    std::string topic_sub  = "pi/results";
    int         api_port   = 8000;
    int         pi_timeout = 30;

    void load(const std::string& path) {
        std::ifstream f(path);
        if (!f) throw std::runtime_error("não foi possível abrir: " + path);
        std::string line;
        while (std::getline(f, line)) {
            line = trim(line);
            if (line.empty() || line[0]=='#') continue;
            auto eq = line.find('=');
            if (eq == std::string::npos) continue;
            std::string k = trim(line.substr(0, eq));
            std::string v = strip_comment(line.substr(eq+1));
            if (v.size()>=2 && v.front()=='"' && v.back()=='"')
                v = v.substr(1, v.size()-2);
            v = trim(v);
            if      (k=="MQTT_HOST")  mqtt_host  = v;
            else if (k=="MQTT_PORT")  mqtt_port  = v;
            else if (k=="MQTT_USER")  mqtt_user  = v;
            else if (k=="MQTT_PASS")  mqtt_pass  = v;
            else if (k=="TOPIC_PUB")  topic_pub  = v;
            else if (k=="TOPIC_SUB")  topic_sub  = v;
            else if (k=="JWT_SECRET") jwt_secret = v;
            else if (k=="API_PORT")   api_port   = std::stoi(v);
            else if (k=="PI_TIMEOUT") pi_timeout = std::stoi(v);
        }
        if (mqtt_host.empty())
            throw std::runtime_error("MQTT_HOST não definido");
    }
};
