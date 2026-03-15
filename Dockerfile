FROM gcc:13.4.0-bookworm

ENV DEBIAN_FRONTEND=noninteractive

# ============================================================
# 🔹 Dependências — apenas o que o projeto realmente usa:
#    - build-essential / cmake → compilação
#    - libssl-dev              → OpenSSL (JWT + TLS do MQTT)
#    - ca-certificates         → validação de certificados
# ============================================================
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake \
    libssl-dev \
    ca-certificates \
 && apt-get clean && rm -rf /var/lib/apt/lists/*

# ============================================================
# 🔹 Copia o projeto (vendor/MQTT-C e vendor/jwt-cpp já estão no repo)
# ============================================================
WORKDIR /app
COPY . .

# ============================================================
# 🔹 Compila
# ============================================================
RUN cmake -S . -B build \
      -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_TESTING=OFF \
      -DCATCH2_BUILD_TESTING=OFF \
 && cmake --build build --target broker-pi -- -j$(nproc)

# ============================================================
# 🔹 Porta (deve bater com API_PORT no ConfigMap)
# ============================================================
EXPOSE 8008

ENTRYPOINT ["./build/broker-pi"]