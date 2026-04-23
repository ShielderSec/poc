import http.server
import socket
import ssl
import subprocess

subprocess.run(
    [
        "openssl",
        "req",
        "-x509",
        "-newkey",
        "rsa:2048",
        "-keyout",
        "/tmp/key.pem",
        "-out",
        "/tmp/cert.pem",
        "-days",
        "1",
        "-nodes",
        "-subj",
        "/CN=secret.example.com",
    ],
    capture_output=True,
)
ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
ctx.load_cert_chain("/tmp/cert.pem", "/tmp/key.pem")


class DualStackHTTPServer(http.server.HTTPServer):
    address_family = socket.AF_INET6

    def server_bind(self):
        self.socket.setsockopt(socket.IPPROTO_IPV6, socket.IPV6_V6ONLY, 0)
        super().server_bind()


srv = DualStackHTTPServer(("::", 443), http.server.BaseHTTPRequestHandler)
srv.socket = ctx.wrap_socket(srv.socket, server_side=True)
srv.serve_forever()
