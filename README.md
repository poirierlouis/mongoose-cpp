# mongoose-cpp

![version: work in progress](https://img.shields.io/badge/version-work%20in%20progress-orange)

A modern C++20 wrapper around the [Mongoose](https://github.com/cesanta/mongoose) embedded web server library.

mongoose-cpp provides a clean, expressive API for building HTTP servers in C++, hiding the C-level details of Mongoose behind thin, zero-overhead abstractions — while preserving full compatibility with embedded and resource-constrained targets.

## Features

- Register HTTP handlers as lambdas, free functions, or member functions
- URI pattern matching with capture groups (`#`, `*`, `?`)
- Fallback (404) handler support
- `std::string_view`-based request accessors (zero-copy)
- No exceptions, no RTTI — suitable for embedded use
- Mongoose vendored as a git submodule — no external dependencies

## Compatibility

- **C++ standard:** C++20
- **Build system:** CMake 4.2+
- **Compilers:** MSVC, GCC, Clang
- **Platforms:** Linux, Windows, and any platform supported by Mongoose (ESP32, STM32, ARM, RISC-V, etc.)
- **Mongoose version:** 7.21

## License

mongoose-cpp is licensed under the **GNU General Public License v2.0** (GPL-2.0).

The bundled [Mongoose](https://github.com/cesanta/mongoose) library is dual-licensed under **GPL-2.0** or a commercial license by Cesanta Software Limited. If you intend to use this project in a proprietary/commercial product, you must obtain a commercial license for Mongoose directly from [Cesanta](https://mongoose.ws/licensing/).

## Building

```bash
git clone --recurse-submodules https://github.com/poirierlouis/mongoose-cpp
cd mongoose-cpp
cmake -B build
cmake --build build
```

## Usage

```cpp
#include <mongoose-cpp.hpp>

using namespace mg::http;

int main() {
    mg::server server;
    if (!server.listen("http://127.0.0.1:8080")) {
        return 1;
    }

    // Lambda handler
    server.register_http("/", [](const request& req,
                                 const std::shared_ptr<response>& res) {
        res->send(status_code::ok, "Hello, world!");
    });

    // Parameterised route — captures the path segment after /user/
    server.register_http("/user/#", [](const request& req,
                                       const std::shared_ptr<response>& res) {
        std::string body{req.get_param(0)};
        res->send(status_code::ok, "User: " + body);
    });

    // Fallback / 404 handler
    server.register_http_fallback([](const request&,
                                     const std::shared_ptr<response>& res) {
        res->send(status_code::not_found, "Not found");
    });

    while (true) {
        server.poll(100);
    }

    return 0;
}
```

See the [`example/`](example/) folder for a complete runnable example demonstrating lambdas, free functions, member function handlers, and parameterised routes.

## Contributing

Pull requests and issues are welcome. Feel free to open an issue to report a bug, request a feature, or ask a question.
