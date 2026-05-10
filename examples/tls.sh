#!/bin/bash
openssl=$1
cert_path=$2

if [ -f "$cert_path/root.key" ]; then
    echo "[TLS] Certificates already generated."
    exit 0
fi

# Root
"$openssl" ecparam -name prime256v1 -genkey -noout -out "$cert_path/root.key"
"$openssl" req -x509 \
            -new \
            -nodes \
            -key "$cert_path/root.key" \
            -out "$cert_path/root.pem" \
            -days 730 \
            -sha512 \
            -subj "/CN=mgxx-ca"

# Client
"$openssl" ecparam -name prime256v1 -genkey -noout -out "$cert_path/client.key"
"$openssl" req -new \
            -key "$cert_path/client.key" \
            -out "$cert_path/client.csr" \
            -subj "/CN=mgxx-client"
"$openssl" x509 -req \
            -in "$cert_path/client.csr" \
            -CA "$cert_path/root.pem" \
            -CAkey "$cert_path/root.key" \
            -CAcreateserial \
            -out "$cert_path/client.crt" \
            -days 365 \
            -sha512

# Browser
"$openssl" pkcs12 -export \
            -out "$cert_path/client.p12" \
            -inkey "$cert_path/client.key" \
            -in "$cert_path/client.crt" \
            -certfile "$cert_path/root.pem" \
            -passout pass:password

# Server
"$openssl" ecparam -name prime256v1 -genkey -noout -out "$cert_path/server.key"
"$openssl" req -new \
            -key "$cert_path/server.key" \
            -out "$cert_path/server.csr" \
            -subj "/CN=127.0.0.1"
"$openssl" x509 -req \
            -in "$cert_path/server.csr" \
            -CA "$cert_path/root.pem" \
            -CAkey "$cert_path/root.key" \
            -CAcreateserial \
            -out "$cert_path/server.crt" \
            -days 365 \
            -sha512
