echo off
set openssl=%1
set path=%2

set openssl=%openssl:"=%
set path=%path:"=%

if exist "%path%\root.key" (
    echo [TLS] Certificates already generated.
    exit /b 0
)

: Root
"%openssl%" genrsa -out "%path%\root.key" 4096
"%openssl%" req -x509 ^
            -new ^
            -nodes ^
            -key "%path%\root.key" ^
            -out "%path%\root.pem" ^
            -days 730 ^
            -sha512 ^
            -subj "/CN=mongoose-cpp-ca"

: Client
"%openssl%" genrsa -out "%path%\client.key" 2048
"%openssl%" req -new ^
            -key "%path%\client.key" ^
            -out "%path%\client.csr" ^
            -subj "/CN=mongoose-cpp-client"
"%openssl%" x509 -req ^
            -in "%path%\client.csr" ^
            -CA "%path%\root.pem" ^
            -CAkey "%path%\root.key" ^
            -CAcreateserial ^
            -out "%path%\client.crt" ^
            -days 365 ^
            -sha512

: Browser
"%openssl%" pkcs12 -export ^
            -out "%path%\client.p12" ^
            -inkey "%path%\client.key" ^
            -in "%path%\client.crt" ^
            -certfile "%path%\root.pem" ^
            -passout pass:password

: Server
"%openssl%" genrsa -out "%path%\server.key" 2048
"%openssl%" req -new ^
            -key "%path%\server.key" ^
            -out "%path%\server.csr" ^
            -subj "/CN=127.0.0.1"
"%openssl%" x509 -req ^
            -in "%path%\server.csr" ^
            -CA "%path%\root.pem" ^
            -CAkey "%path%\root.key" ^
            -CAcreateserial ^
            -out "%path%\server.crt" ^
            -days 365 ^
            -sha512
