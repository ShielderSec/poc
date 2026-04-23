#!/bin/bash
# requires kernel >= 5.6, seccomp must allow io_uring_setup.

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
CONTAINER_NAME="poc-io-uring"

echo "Make sure ig is running in another terminal:"
echo "  sudo ig run trace_open -c ${CONTAINER_NAME} | grep OPENAT"
echo "Press Enter to continue..."
read -r

sudo docker run --rm \
    --name "${CONTAINER_NAME}" \
    --security-opt seccomp=unconfined \
    -v "${SCRIPT_DIR}/io_uring_open.c:/src/io_uring_open.c:ro" \
    gcc:latest \
    bash -c "
        gcc -o /tmp/io_uring_open /src/io_uring_open.c -Wall -Wextra && \
        /tmp/io_uring_open
    "
