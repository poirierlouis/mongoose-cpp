# mongoose-cpp

![version: work in progress](https://img.shields.io/badge/version-work%20in%20progress-orange)

A modern C++20 wrapper around the
[Mongoose](https://github.com/cesanta/mongoose) embedded web server library.

mongoose-cpp provides a clean, expressive API for building HTTP servers in C++,
hiding the C-level details of Mongoose behind thin, zero-overhead abstractions —
while preserving full compatibility with embedded and resource-constrained
targets.

## Features

- Register HTTP handlers using lambdas (no `std::function`)
- Synchronous and asynchronous request handling (thread-safe)
- URI pattern matching with capture groups (`#`, `*`, `?`) using mongoose
- Fallback handler support (404)
- TLS/HTTPS support (built-in, OpenSSL, mbedTLS)
- Two-way TLS (mTLS) with client certificate extraction
- JSON response utility
- Custom logging with configurable levels and callbacks
- `std::string_view` based request accessors (zero-copy)
- No exceptions, no RTTI — suitable for embedded use
- Mongoose vendored as a git submodule — no external dependencies

## Compatibility

- **C++ standard:** C++20
- **Build system:** CMake 3.15+
- **Compilers:** MSVC, GCC, Clang
- **Platforms:** Windows (currently tested), Linux, and any platform supported 
  by Mongoose (ESP32, STM32, ARM, RISC-V, etc.)
- **Mongoose version:** 7.21

## License

mongoose-cpp is licensed under the **GNU General Public License v2.0**
(GPL-2.0).

The bundled [Mongoose](https://github.com/cesanta/mongoose) library is
dual-licensed under **GPL-2.0** or a commercial license by Cesanta Software
Limited. If you intend to use this project in a proprietary/commercial product,
you must get a commercial license for Mongoose directly from
[Cesanta](https://mongoose.ws/licensing/).

## Building

CMake presets are provided for building the static library with/without the 
example project. It provides options with no TLS, builtin TLS, OpenSSL, and 
mbedTLS support. It only includes support for Windows at the moment.

```bash
git clone --recurse-submodules https://github.com/poirierlouis/mongoose-cpp
cd mongoose-cpp
cmake -B build --preset example-win-openssl
cmake --build build --config Release
```

## Usage

### Simple Server

```cpp
// ...
#include <mongoose-cpp.hpp>

using namespace mg::http;

int main() {
  // Optional: setup a custom logger
  mg::server server([](std::string_view msg) {
    std::cout << "[log] " << msg;
  }, MG_LL_INFO);

  auto web = server.listen_web("http://0.0.0.0:80");
  if (!web) {
    return 1;
  }

  // Lambda handler
  web->on_request("/", [](const request&,
                          const std::shared_ptr<response>& res) {
    res->send(status_code::ok, "Hello, world!");
  });

  // Parameterized route with capture groups
  web->on_request("/user/?", [](const request& req,
                                const std::shared_ptr<response>& res) {
    const auto name = req.get_param(0).value_or("unknown");
    res->send(status_code::ok, std::format("Hello, {}!", name));
  });

  // JSON response
  web->on_request("/api/status", [](const request&,
                                    const std::shared_ptr<response>& res) {
    res->send_json(status_code::ok, R"({"status": "up"})");
  });

  while (true) {
    server.poll(100);
  }

  return 0;
}
```

### HTTPS and mTLS

Only builtin TLS (from mongoose), OpenSSL, and mbedTLS are supported for now.

```cpp
// listen on https://

// Enable TLS
const std::string cert = "data of server.crt";
const std::string key = "data of server.key";
web->use_tls(cert, key);

// Enable two-way TLS (mTLS) by providing a CA certificate
const std::string ca = "data of ca.pem";
web->use_tls(ca, cert, key);

// Access client's certificate info
web->on_request("/secure", [](const request& req,
                              const std::shared_ptr<response>& res) {
  if (const auto info = req.get_tls_cert_info().lock()) {
    res->send(status_code::ok, "Welcome, " + info->get_subject_name());
  }
});
```

### Asynchronous Responses

For long-running tasks or thread-safe responses:

```cpp
web->on_async_request("/long-task", [](const request& req,
                                       const std::shared_ptr<async_response>& res) {
  // req.to_async() captures request data safely for use in another thread

  // Zero-copy is a no-go in this case. One continuous memory-block is allocated
  // to prevent memory fragmentation like Mongoose does.

  std::thread([async_req = req.to_async(), res]() {
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // use can use async_req safely
    res->send(status_code::ok, "Task completed");
  }).detach();
});
```

### mTLS

Extraction of client's certificate info is supported for both OpenSSL and
mbedTLS. A single format is used to represent the data consistently across TLS
backends.

This is an example of the client's certificate info available in the request:
```
Subject: CN=mongoose-cpp-client
Issuer: CN=mongoose-cpp-ca
Serial Number: 38B175F7211897C00949D2CF8C62E7F39D744359
Not Before: 1777664143
Not After: 1809200143
Fingerprint: 76:3F:5A:29:8B:C6:1A:76:A3:E2:DA:1F:0D:75:6F:40:91:A3:71:EA:74:1F:9D:A7:9E:DF:FC:6C:3F:98:25:C5
```

## Examples

See the [`example/`](example/src/main.cpp) for a complete runnable example
demonstrating all these features, including different handler types,
parameterized routes, TLS, and asynchronous responses.

It includes a `tls.cmd` script to generate a self-signed certificate for 
testing. It will automatically run when you build the example project as long as
you have OpenSSL installed. You can manually add `client.p12` certificate in 
your browser to test mTLS.

## Contributing

Pull requests and issues are welcome. Feel free to open an issue to report a
bug, request a feature, or ask a question.

### Code formatting

This project uses `clang-format` to maintain a consistent code style. It is
derived from Google C++ style.