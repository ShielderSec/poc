#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
NETWORK_NAME="poc-ipv6-sni"
SERVER_NAME="poc-ipv6-server"
CLIENT_NAME="poc-ipv6-client"
SNI_HOST_V4="ipv4.example.com"
SNI_HOST_V6="ipv6.example.com"

echo "Make sure ig is running in another terminal:"
echo "  sudo ig run trace_sni -c ${CLIENT_NAME}"
echo ""
echo "Press Enter to continue..."
read -r

cleanup() {
    echo ""
    echo "[*] Cleaning up ..."
    sudo docker rm -f "${SERVER_NAME}" "${CLIENT_NAME}" 2>/dev/null || true
    sudo docker network rm "${NETWORK_NAME}" 2>/dev/null || true
}
trap cleanup EXIT

echo "[*] Creating dual-stack Docker network ..."
sudo docker network create \
    --ipv6 \
    --subnet 172.30.0.0/24 \
    --subnet fd00:dead:beef::/64 \
    "${NETWORK_NAME}" >/dev/null

echo "[*] Starting local HTTPS server ..."
sudo docker run -d --rm \
    --name "${SERVER_NAME}" \
    --network "${NETWORK_NAME}" \
    -v "${SCRIPT_DIR}/server.py:/server.py:ro" \
    python:3-slim \
    python3 /server.py >/dev/null

sleep 3

SERVER_IPV4=$(sudo docker inspect -f '{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}' "${SERVER_NAME}")
SERVER_IPV6=$(sudo docker inspect -f '{{range .NetworkSettings.Networks}}{{.GlobalIPv6Address}}{{end}}' "${SERVER_NAME}")
echo "    Server IPv4: ${SERVER_IPV4}"
echo "    Server IPv6: ${SERVER_IPV6}"

echo ""
echo "[PHASE 1] TLS over IPv4 to ${SNI_HOST_V4} — VISIBLE"
sudo docker run --rm \
    --name "${CLIENT_NAME}" \
    --network "${NETWORK_NAME}" \
    curlimages/curl \
    curl -4 -k -so /dev/null \
        --resolve "${SNI_HOST_V4}:443:${SERVER_IPV4}" \
        "https://${SNI_HOST_V4}/" && echo "  OK" || echo "  Failed"

sleep 2

echo ""
echo "[PHASE 2] TLS over IPv6 to ${SNI_HOST_V6} — INVISIBLE"
sudo docker run --rm \
    --name "${CLIENT_NAME}" \
    --network "${NETWORK_NAME}" \
    curlimages/curl \
    curl -6 -k -so /dev/null \
        --resolve "${SNI_HOST_V6}:443:[${SERVER_IPV6}]" \
        "https://${SNI_HOST_V6}/" && echo "  OK" || echo "  Failed"
