#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
CONTAINER_NAME="poc-escape-inject"

echo "Make sure ig is running in another terminal:"
echo "  sudo ig run trace_open -c ${CONTAINER_NAME}"
echo ""
echo "Press Enter to continue..."
read -r

sudo docker run --rm \
    --name "${CONTAINER_NAME}" \
    -v "${SCRIPT_DIR}/escape_inject.c:/src/escape_inject.c:ro" \
    gcc:latest \
    bash -c "
        gcc -o /tmp/escape_inject /src/escape_inject.c -Wall && \
        /tmp/escape_inject
    "
