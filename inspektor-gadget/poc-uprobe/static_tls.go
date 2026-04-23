package main

import (
	"crypto/tls"
	"fmt"
	"io"
	"net/http"
	"os"
)

func main() {
	host := "example.com"
	if len(os.Args) > 1 {
		host = os.Args[1]
	}

	fmt.Printf("[static/go] Making TLS connection to %s using Go crypto/tls...\n", host)

	tr := &http.Transport{
		TLSClientConfig: &tls.Config{
			ServerName: host,
		},
	}
	client := &http.Client{Transport: tr}

	resp, err := client.Get(fmt.Sprintf("https://%s/", host))
	if err != nil {
		fmt.Fprintf(os.Stderr, "[static/go] Request failed: %v\n", err)
		os.Exit(1)
	}
	defer resp.Body.Close()

	body, _ := io.ReadAll(io.LimitReader(resp.Body, 256))
	fmt.Printf("[static/go] Response: %s %s\n", resp.Proto, resp.Status)
	_ = body

	fmt.Printf("[static/go] Done. This should NOT appear in trace_ssl output.\n")
}
