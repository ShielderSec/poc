#!/bin/bash
# requires kernel >= 5.6 for openat2 support.

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
CONTAINER_NAME="poc-openat2"

echo "Make sure ig is running in another terminal:"
echo "  sudo ig run trace_open -c ${CONTAINER_NAME} | grep OPENAT"
echo ""
echo "Press Enter to continue..."
read -r

sudo docker run --rm \
    --name "${CONTAINER_NAME}" \
    -v "${SCRIPT_DIR}/openat2.c:/src/openat2.c:ro" \
    gcc:latest \
    bash -c "
        gcc -o /tmp/openat2_poc /src/openat2.c -Wall -Wextra && \
        /tmp/openat2_poc
    "

echo ""
echo "============================================"
echo "Check ig output. 'VISIBLE_OPENAT' should appear."
echo "'INVISIBLE_OPENAT2' should NOT appear."
echo "============================================"
