#!/bin/bash
set -e

# ======================================
# 🔹 AUTO-DETECÇÃO DO YAML E DEPLOYMENT
# ======================================
YAML_FILE=$(ls *.yaml 2>/dev/null | head -n 1)
if [[ -z "$YAML_FILE" ]]; then
  echo "❌ Nenhum arquivo YAML encontrado."
  exit 1
fi

DEPLOYMENT_NAME=$(grep -m1 'name:' "$YAML_FILE" | awk '{print $2}')
if [[ -z "$DEPLOYMENT_NAME" ]]; then
  echo "❌ Não foi possível extrair o nome do deployment de $YAML_FILE"
  exit 1
fi

# ======================================
# 🔹 FUNÇÕES DE COR E FORMATAÇÃO
# ======================================
cyan()   { echo -e "\033[1;36m$1\033[0m"; }
green()  { echo -e "\033[1;32m$1\033[0m"; }
yellow() { echo -e "\033[1;33m$1\033[0m"; }
red()    { echo -e "\033[1;31m$1\033[0m"; }
line()   { echo "──────────────────────────────────────────────────────────"; }

clear
cyan "╔═══════════════════════════════════════════════════════╗"
cyan "║ 🔍 STATUS DETALHADO DO DEPLOYMENT: ${DEPLOYMENT_NAME^^}"
cyan "╚═══════════════════════════════════════════════════════╝"
echo ""

# ======================================
# 1️⃣ DEPLOYMENT INFO
# ======================================
green "📦 DEPLOYMENT:"
kubectl get deployment "$DEPLOYMENT_NAME" -o custom-columns=\
"NAME:.metadata.name,\
NAMESPACE:.metadata.namespace,\
READY:.status.readyReplicas,\
UP-TO-DATE:.status.updatedReplicas,\
AVAILABLE:.status.availableReplicas,\
AGE:.metadata.creationTimestamp" --no-headers | column -t
line

# ======================================
# 2️⃣ PODS DETALHADOS
# ======================================
green "🐳 PODS:"
kubectl get pods -l app="$DEPLOYMENT_NAME" \
  -o custom-columns="NAME:.metadata.name,STATUS:.status.phase,RESTARTS:.status.containerStatuses[*].restartCount,AGE:.metadata.creationTimestamp,NODE:.spec.nodeName,IP:.status.podIP" \
  --no-headers | column -t
line

# ======================================
# 3️⃣ SERVICES
# ======================================
green "🌐 SERVICES:"
kubectl get svc -l app="$DEPLOYMENT_NAME" \
  -o custom-columns="NAME:.metadata.name,TYPE:.spec.type,CLUSTER-IP:.spec.clusterIP,PORTS:.spec.ports[*].port,NAMESPACE:.metadata.namespace" \
  --no-headers | column -t || yellow "⚠️ Nenhum service encontrado."
line

# ======================================
# 4️⃣ ROLLOUT STATUS
# ======================================
green "🔄 ROLLOUT STATUS:"
kubectl rollout status deployment/"$DEPLOYMENT_NAME" --timeout=15s || yellow "⚠️ Rollout ainda em progresso"
line

# ======================================
# 5️⃣ IMAGENS EM USO
# ======================================
green "🖼️ IMAGEM ATUAL:"
kubectl get pods -l app="$DEPLOYMENT_NAME" -o jsonpath='{range .items[*]}{.metadata.name}{" -> "}{.spec.containers[*].image}{"\n"}{end}' | column -t
line

# ======================================
# 6️⃣ RECURSOS DO POD
# ======================================
green "🧠 RECURSOS (CPU/MEM):"
kubectl top pod -l app="$DEPLOYMENT_NAME" 2>/dev/null || yellow "⚠️ Métricas indisponíveis (metrics-server ausente)"
line

# ======================================
# 7️⃣ EVENTOS RECENTES
# ======================================
green "📋 EVENTOS RECENTES:"
kubectl get events --sort-by=.metadata.creationTimestamp | grep "$DEPLOYMENT_NAME" | tail -n 10 || yellow "⚠️ Nenhum evento recente encontrado."
line

# ======================================
# 8️⃣ LOGS DO POD MAIS RECENTE
# ======================================
LATEST_POD=$(kubectl get pods -l app="$DEPLOYMENT_NAME" -o jsonpath='{.items[-1].metadata.name}' 2>/dev/null || true)
if [[ -n "$LATEST_POD" ]]; then
  green "🧾 LOGS DO POD MAIS RECENTE: $LATEST_POD"
  kubectl logs "$LATEST_POD" --tail=15 | sed 's/^/   /'
else
  yellow "⚠️ Nenhum pod encontrado para logs."
fi
line

# ======================================
# 9️⃣ STATUS RESUMIDO FINAL
# ======================================
cyan "📊 RESUMO FINAL:"
echo ""
printf "%-15s %-12s %-12s %-12s %-15s\n" "COMPONENTE" "STATUS" "RÉPLICAS" "IMAGEM" "NODE"
echo "------------------------------------------------------------------"
kubectl get deployment "$DEPLOYMENT_NAME" -o jsonpath='{.metadata.name}{"\t"}{.status.availableReplicas}{"\t"}{.status.replicas}{"\t"}{.spec.template.spec.containers[0].image}{"\t"}{.spec.template.spec.nodeName}{"\n"}' | column -t

echo ""
green "✅ Relatório completo de: $DEPLOYMENT_NAME"
echo ""
