#!/usr/bin/env python3
import socket
import time

MARKER_HIDDEN = b"SECRET_EXFIL_DATA"

def main():
    host = "0.0.0.0"
    port = 9999

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind((host, port))
    print(f"[receiver] Listening on {host}:{port} (UDP) ...")

    data, addr = sock.recvfrom(65536)
    print(f"[receiver] Received {len(data)} bytes from {addr}")

    idx = data.find(MARKER_HIDDEN)
    if idx >= 0:
        print(f"[receiver] Found hidden marker at payload offset {idx}")
        print(f"[receiver] Hidden content: {data[idx:idx+len(MARKER_HIDDEN)].decode()}")
    else:
        print("[receiver] WARNING: hidden marker not found in payload")

    sock.close()
    print("[receiver] Done.")

    time.sleep(30)


if __name__ == "__main__":
    main()
