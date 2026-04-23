#!/usr/bin/env python3

import socket
import sys
import time
PADDING = b"A" * 2000

MARKER_HIDDEN = b"SECRET_EXFIL_DATA"

def main():
    host = sys.argv[1] if len(sys.argv) > 1 else "10.0.0.2"
    port = 9999

    payload = PADDING + MARKER_HIDDEN

    print(f"[sender] Secret marker at payload offset: {len(PADDING)}")
    print(f"[sender] Secret content: {MARKER_HIDDEN.decode()}")
    print()

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    print(f"[sender] Sending to {host}:{port} ...")
    time.sleep(2)

    sock.sendto(payload, (host, port))
    print(f"[sender] Sent {len(payload)} byte UDP datagram.")

    sock.close()
    print("[sender] Done.")


if __name__ == "__main__":
    main()
