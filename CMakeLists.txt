cmake_minimum_required(VERSION 3.16)
project(vps_broker CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)

# ───────────── executável principal ─────────────
add_executable(broker-pi
    src/main.cpp
    vendor/MQTT-C/src/mqtt.c
    vendor/MQTT-C/src/mqtt_pal.c
)

target_include_directories(broker-pi PRIVATE
    src
    vendor
    vendor/MQTT-C/include
    vendor/MQTT-C/examples/templates
    vendor/jwt-cpp/include
)

target_link_libraries(broker-pi
    Threads::Threads
    OpenSSL::SSL
    OpenSSL::Crypto
)

target_compile_options(broker-pi PRIVATE
    -O2
    -fstack-protector-strong
)

target_link_options(broker-pi PRIVATE
    -Wl,-z,relro
    -Wl,-z,now
)

# ───────────── executável de testes/utils ─────────────
add_executable(utils
    src/utils.cpp
)

target_include_directories(utils PRIVATE
    src
    vendor/jwt-cpp/include
)

target_link_libraries(utils
    OpenSSL::SSL
    OpenSSL::Crypto
)