#include <atomic>
#include <chrono>
#include <csignal>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mongoose-cpp.hpp>
#include <thread>

using namespace mg::http;

void simple_handler(const request&, const std::shared_ptr<response>& res) {
  res->send(status_code::im_a_teapot,
            "What starts with T, ends with T, but only has T in?");
}

class example {
  int trick = 67;

 public:
  void simple(const request&, const std::shared_ptr<response>& res) {
    res->send(status_code::ok,
              std::format("I'm a class member callback: {}", trick));
  }
};

std::atomic_bool is_running{true};

void signal_handler(int) {
  std::cout << "[mg::server] Shutting down..." << '\n';
  is_running = false;
}

std::optional<std::string> read_file(const std::filesystem::path& path) {
  std::ifstream file(path);
  if (!file.is_open()) {
    return std::nullopt;
  }

  return std::string(std::istreambuf_iterator(file),
                     std::istreambuf_iterator<char>());
}

std::optional<int> to_int(const std::string_view str) {
  int value;
  const auto [ptr, ec] =
      std::from_chars(str.data(), str.data() + str.size(), value);
  if (ec == std::errc{}) {
    return value;
  }
  return std::nullopt;
}

int main(int, char**) {
  std::signal(SIGINT, &signal_handler);

  mg::server server(
      [](const std::string_view message) {
        std::cout << "[mg::server] " << message;
      },
      MG_LL_DEBUG);

#if MG_TLS == MG_TLS_OPENSSL || MG_TLS == MG_TLS_MBED
  const auto ca = read_file(std::filesystem::absolute("../../../root.pem"));
  if (!ca) {
    std::cerr << "[mg::server] Failed to load server's root key" << '\n';
    return 1;
  }
#endif

#if MG_TLS == MG_TLS_BUILTIN || MG_TLS == MG_TLS_OPENSSL || \
    MG_TLS == MG_TLS_MBED
  const auto cert = read_file(std::filesystem::absolute("../../../server.crt"));
  if (!cert) {
    std::cerr << "[mg::server] Failed to load server's certificate" << '\n';
    return 1;
  }

  const auto key = read_file(std::filesystem::absolute("../../../server.key"));
  if (!key) {
    std::cerr << "[mg::server] Failed to load server's key" << '\n';
    return 1;
  }

  constexpr auto host = "https://127.0.0.1:4200";
#else
  constexpr auto host = "http://127.0.0.1:4200";
#endif

  const auto http = server.listen_http(host);
  if (!http) {
    std::cerr << "[mg::server] Failed to listen on " << host << '\n';
    return 1;
  }

#if MG_TLS == MG_TLS_BUILTIN
  http->use_tls(cert.value(), key.value());
#elif MG_TLS == MG_TLS_OPENSSL || MG_TLS == MG_TLS_MBED
  http->use_tls(ca.value(), cert.value(), key.value());
#endif

  example ex;
  uint64_t counter{0};
  http->on_request("/joke", &simple_handler)
      .on_request(
          "/class",
          [&ex](const request& req, const std::shared_ptr<response>& res) {
            ex.simple(req, res);
          })
      .on_request(
          "/lambda",
          [&counter](const request&, const std::shared_ptr<response>& res) {
            res->send(
                status_code::ok,
                std::format("I'm a lambda callback ({} calls).", ++counter));
          })
      .on_request(
          "/headers",
          [](const request&, const std::shared_ptr<response>& res) {
            auto& headers = res->get_headers();
            headers.set("Authorization", "Bearer [token]");

            std::string body;

            const auto auth = headers.get_view("authorization");
            body += std::format("auth is {}\n", auth.value_or("<not found>"));

            const auto fake = headers.get_view("fake");
            body += std::format("fake is {}\n", fake.value_or("<not found>"));

            headers.set("X-Fake", "warning");
            // previous get_view results are invalidated due to write operation.

            body +=
                std::format("X-Fake is {}",
                            headers.get_view("x-fake").value_or("<not found>"));

            res->send(status_code::ok, body);
          })
      .on_request(
          "/file",
          [](const request&, const std::shared_ptr<response>& res) {
            auto file =
                std::make_unique<std::ifstream>("../../../../example/tls.cmd");
            if (!file->is_open()) {
              res->send(status_code::internal_server_error,
                        "Failed to open file");
              return;
            }

            res->get_headers().set("Content-Type", "text/plain");
            res->stream(
                status_code::ok,
                // "chunked" by default. You can set "chunked" and compression
                // algorithm after the status code argument.
                [file = std::move(file)]() -> std::optional<std::string> {
                  if (file->eof()) {
                    // terminating chunk
                    return std::nullopt;
                  }

                  std::string chunk(32, '\0');
                  file->read(chunk.data(),
                             static_cast<std::streamsize>(chunk.size()));
                  chunk.resize(file->gcount());
                  return chunk;
                });
            res->send(status_code::bad_request);  // no-op
          })
      .on_request(
          "/api/#",
          [](const request& req, const std::shared_ptr<response>& res) {
            const auto param = req.get_param(0).value_or("");
            res->send(
                status_code::ok,
                std::format("I'm capturing one group for API endpoints: {}",
                            param));
          })
      .on_request(
          "/cert",
          [](const request& req, const std::shared_ptr<response>& res) {
            if (const auto cert = req.get_tls_cert_info().lock()) {
              res->send(
                  status_code::ok,
                  std::format("--- Client certificate ---\n"
                              "Subject:{}\n"
                              "Issuer:{}\n"
                              "Serial Number:{}\n"
                              "Not Before:{}\n"
                              "Not After:{}\n"
                              "Fingerprint:{}",
                              cert->get_subject_name(), cert->get_issuer_name(),
                              cert->get_serial_number(), cert->get_not_before(),
                              cert->get_not_after(), cert->get_fingerprint()));
            } else {
              res->send(status_code::bad_request,
                        "No client certificate provided");
            }
          })
      .on_fallback([](const request&, const std::shared_ptr<response>& res) {
        res->send(status_code::not_found, "404 Not Found");
      });

  std::atomic_uint64_t async_counter{1};
  http->on_async_request(
          "/async/cert",
          [](const request& req, const std::shared_ptr<async_response>& res) {
            std::thread thread([async_req = req.to_async(), res] {
              std::this_thread::sleep_for(std::chrono::seconds(2));

              if (const auto cert = async_req->get_tls_cert_info().lock()) {
                res->send(status_code::ok,
                          std::format(
                              "--- Client certificate ---\n"
                              "Subject:{}\n"
                              "Issuer:{}\n"
                              "Serial Number:{}\n"
                              "Not Before:{}\n"
                              "Not After:{}\n"
                              "Fingerprint:{}",
                              cert->get_subject_name(), cert->get_issuer_name(),
                              cert->get_serial_number(), cert->get_not_before(),
                              cert->get_not_after(), cert->get_fingerprint()));
              } else {
                res->send(status_code::bad_request,
                          "No client certificate provided");
              }
            });

            thread.detach();
          })
      .on_async_request(
          "/async/?",
          [&async_counter](const request& req,
                           const std::shared_ptr<async_response>& res) {
            std::thread thread(
                [&async_counter, async_req = req.to_async(), res] {
                  const auto param = async_req->get_param(0).value_or("5");
                  const int sleep = to_int(param).value_or(5);
                  std::this_thread::sleep_for(std::chrono::seconds(sleep));

                  auto count = async_counter.fetch_add(1);
                  res->send(status_code::ok,
                            std::format("I'm an async {} callback ({} calls).",
                                        async_req->method(), count));
                });

            thread.detach();
          });

  while (is_running) {
    server.poll(100);
  }

  return 0;
}