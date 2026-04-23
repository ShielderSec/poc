#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
NETWORK_NAME="poc-jumbo-net"
SENDER_NAME="poc-jumbo-sender"
RECEIVER_NAME="poc-jumbo-receiver"
MTU=9000
PCAP_FILE="/tmp/poc-jumbo-gadget.pcap"

cleanup() {
    echo ""
    echo "[*] Cleaning up ..."
    # Stop the gadget
    if [ -n "${GADGET_PID}" ]; then
        sudo kill "${GADGET_PID}" 2>/dev/null || true
        wait "${GADGET_PID}" 2>/dev/null || true
    fi
    sudo docker rm -f "${SENDER_NAME}" "${RECEIVER_NAME}" 2>/dev/null || true
    sudo docker network rm "${NETWORK_NAME}" 2>/dev/null || true
}
trap cleanup EXIT

echo "[*] Creating Docker network with MTU ${MTU} ..."
sudo docker network create \
    --driver bridge \
    --opt com.docker.network.driver.mtu=${MTU} \
    "${NETWORK_NAME}" >/dev/null

echo "[*] Starting receiver container ..."
sudo docker run -d \
    --name "${RECEIVER_NAME}" \
    --network "${NETWORK_NAME}" \
    -v "${SCRIPT_DIR}/receiver.py:/receiver.py:ro" \
    python:3-slim \
    python3 -u /receiver.py >/dev/null

sleep 2

RECEIVER_IP=$(sudo docker inspect -f '{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}' "${RECEIVER_NAME}")
echo "[*] Receiver IP: ${RECEIVER_IP}"

echo "[*] Verifying MTU inside receiver container ..."
ACTUAL_MTU=$(sudo docker exec "${RECEIVER_NAME}" cat /sys/class/net/eth0/mtu)
echo "    Container eth0 MTU: ${ACTUAL_MTU}"

if [ "${ACTUAL_MTU}" -lt 9000 ]; then
    echo "    WARNING: MTU is less than 9000, jumbo frames may be fragmented"
fi

echo "[*] Starting tcpdump gadget (saving to ${PCAP_FILE}) ..."
rm -f "${PCAP_FILE}"
sudo ig run tcpdump -c "${RECEIVER_NAME}" -o pcap-ng > "${PCAP_FILE}" 2>/dev/null &
GADGET_PID=$!
sleep 3

echo "[*] Sending UDP datagram with hidden data beyond snaplen ..."
echo ""
sudo docker run --rm \
    --name "${SENDER_NAME}" \
    --network "${NETWORK_NAME}" \
    -v "${SCRIPT_DIR}/sender.py:/sender.py:ro" \
    python:3-slim \
    python3 -u /sender.py "${RECEIVER_IP}"

echo ""

sleep 3

echo "[*] Receiver output:"
sudo docker logs "${RECEIVER_NAME}" 2>&1
echo ""

echo "[*] Stopping gadget ..."
sudo kill "${GADGET_PID}" 2>/dev/null || true
wait "${GADGET_PID}" 2>/dev/null || true
unset GADGET_PID
sleep 1

echo "[*] All strings found in captured pcap:"
echo "---"
strings "${PCAP_FILE}"
echo "---"
echo ""

echo "[*] Searching for 'SECRET_EXFIL' in captured pcap ..."
if strings "${PCAP_FILE}" | grep -q "SECRET_EXFIL"; then
    echo "    FOUND — the hidden data IS visible in the capture (unexpected!)"
else
    echo "    NOT FOUND — the hidden data was truncated by snaplen=1500"
fi
