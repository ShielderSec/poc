#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
CONTAINER_NAME="poc-fsmount"

echo ""
echo "Make sure ig is running in another terminal:"
echo "  sudo ig run trace_mount -c ${CONTAINER_NAME}"
echo ""
echo "Press Enter to continue..."
read -r

sudo docker run --rm \
    --name "${CONTAINER_NAME}" \
    --cap-add=SYS_ADMIN \
    -v "${SCRIPT_DIR}/fsmount.c:/src/fsmount.c:ro" \
    gcc:latest \
    bash -c "
        gcc -o /tmp/fsmount_poc /src/fsmount.c -Wall -Wextra && \
        /tmp/fsmount_poc
    "
