#!/bin/bash
set -e

# ==============================================
# 🔹 DETECTA NOME AUTOMATICAMENTE PELA PASTA
# ==============================================
PROJECT_DIR=$(basename "$(pwd)")
IMAGE_NAME="${PROJECT_DIR}:latest"

# procura YAML do projeto
YAML_FILE=$(ls *.yaml 2>/dev/null | head -n 1)
if [[ -z "$YAML_FILE" ]]; then
  echo "❌ Nenhum arquivo YAML encontrado na pasta atual."
  exit 1
fi

# tenta extrair nome do deployment
DEPLOYMENT_NAME=$(grep -m1 '^  name:' "$YAML_FILE" | awk '{print $2}')
if [[ -z "$DEPLOYMENT_NAME" ]]; then
  DEPLOYMENT_NAME="$PROJECT_DIR"  # fallback
fi

# ==============================================
# 🔹 FUNÇÕES COLORIDAS
# ==============================================
cyan()  { echo -e "\033[1;36m$1\033[0m"; }
green() { echo -e "\033[1;32m$1\033[0m"; }
red()   { echo -e "\033[1;31m$1\033[0m"; }

# ==============================================
# 🔹 INFORMAÇÕES INICIAIS
# ==============================================
clear
cyan "=============================================="
cyan "🚀 DEPLOY AUTOMÁTICO - K3S LOCAL"
cyan "📁 Projeto: $PROJECT_DIR"
cyan "📦 Imagem:  $IMAGE_NAME"
cyan "🧾 YAML:    $YAML_FILE"
cyan "🔧 Deploy:  $DEPLOYMENT_NAME"
cyan "=============================================="
echo ""

# ==============================================
# 🔹 BUILD DO DOCKER
# ==============================================
green "🐳 Construindo imagem Docker..."
if ! docker build -t "$IMAGE_NAME" .; then
  red "❌ Falha ao construir imagem."
  exit 1
fi

# ==============================================
# 🔹 IMPORTA IMAGEM PARA O K3S (containerd)
# ==============================================
green "📦 Importando imagem para o K3s..."
if ! docker save "$IMAGE_NAME" | sudo k3s ctr images import -; then
  red "❌ Falha ao importar imagem no K3s!"
  exit 1
fi

# ==============================================
# 🔹 ESCOLHER AÇÃO
# ==============================================
echo ""
cyan "-----------------------------------------"
cyan "💬 Escolha a ação:"
cyan "-----------------------------------------"
echo "1️⃣  Novo deploy (delete + apply)"
echo "2️⃣  Atualização (rollout restart)"
read -rp "Escolha [1 ou 2]: " CHOICE
echo ""

case "$CHOICE" in
  1)
    green "🆕 Novo deploy detectado..."
    # 🔸 Verifica se o deployment já existe
    if kubectl get deployment "$DEPLOYMENT_NAME" >/dev/null 2>&1; then
      red "⚠️  Deployment existente detectado. Deletando..."
      kubectl delete deployment "$DEPLOYMENT_NAME" --ignore-not-found
      sleep 2
    fi

    green "📦 Aplicando novo deployment com ${YAML_FILE}..."
    if ! kubectl apply -f "$YAML_FILE"; then
      red "❌ Falha ao aplicar o YAML!"
      exit 1
    fi
    ;;
  2)
    green "🔄 Reiniciando rollout do deployment existente..."
    if ! kubectl rollout restart deployment "$DEPLOYMENT_NAME"; then
      red "❌ Falha ao reiniciar rollout!"
      exit 1
    fi
    ;;
  *)
    red "❌ Opção inválida."
    exit 1
    ;;
esac

# ==============================================
# 🔹 MONITORAMENTO DO DEPLOYMENT
# ==============================================
green "⏳ Verificando rollout (timeout 60s)..."
kubectl rollout status deployment/"$DEPLOYMENT_NAME" --timeout=60s || true
echo ""

# ==============================================
# 🔹 RESUMO FINAL
# ==============================================
green "✅ DEPLOY CONCLUÍDO!"
echo ""
kubectl get deployment "$DEPLOYMENT_NAME" -o wide
echo ""
kubectl get pods -l app="$DEPLOYMENT_NAME" -o wide
echo ""
green "💡 Dica: use ./info.sh para visualizar detalhes e logs."
