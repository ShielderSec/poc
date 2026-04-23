#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
CONTAINER_NAME="poc-uprobe-evasion"
TARGET_HOST="${1:-example.com}"

echo "Make sure ig is running in another terminal:"
echo "  sudo ig run trace_ssl -c ${CONTAINER_NAME}"
echo ""
echo "Press Enter to continue..."
read -r

cleanup() {
    echo ""
    echo "[*] Cleaning up ..."
    sudo docker rm -f "${CONTAINER_NAME}" 2>/dev/null || true
}
trap cleanup EXIT

echo "[*] Starting container and installing curl + Go ..."
sudo docker run -d \
    --name "${CONTAINER_NAME}" \
    -v "${SCRIPT_DIR}/static_tls.go:/src/static_tls.go:ro" \
    golang:latest \
    sleep 300 >/dev/null

sudo docker exec "${CONTAINER_NAME}" \
    bash -c "apt-get update -qq && apt-get install -y -qq curl >/dev/null 2>&1"

sleep 3

echo "[*] PHASE 1: curl with dynamic OpenSSL - VISIBLE"
sudo docker exec "${CONTAINER_NAME}" \
    curl -so /dev/null "https://${TARGET_HOST}/"
echo "  Done."

sleep 2

echo ""
echo "[*] PHASE 2: Go crypto/tls client - INVISIBLE"
sudo docker exec "${CONTAINER_NAME}" \
    bash -c "cd /src && go run static_tls.go ${TARGET_HOST}"
