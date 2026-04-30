#include <../../src/mongoose-cpp.hpp>
#include <atomic>
#include <chrono>
#include <csignal>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>

using namespace mg::http;

void simple_handler(const request&, const std::shared_ptr<response>& res) {
  res->send(status_code::ok, "I'm a raw function callback.");
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

int main(int, char**) {
  std::signal(SIGINT, &signal_handler);

  mg::server server(
      [](const std::string_view message) {
        std::cout << "[mg::server] " << message;
      },
      MG_LL_DEBUG);

#ifdef HAS_TLS
  const auto cert = read_file(std::filesystem::absolute("cert.pem"));
  if (!cert) {
    std::cerr << "[mg::server] Failed to load public certificate" << '\n';
    return 1;
  }

  const auto key = read_file(std::filesystem::absolute("key.pem"));
  if (!key) {
    std::cerr << "[mg::server] Failed to load private key" << '\n';
    return 1;
  }

  constexpr auto host = "https://127.0.0.1:4200";
#else
  constexpr auto host = "http://127.0.0.1:4200";
#endif

  const auto web = server.listen_web(host);
  if (!web) {
    std::cerr << "[mg::server] Failed to listen on " << host << '\n';
    return 1;
  }

#ifdef HAS_TLS
  web->use_tls(cert.value(), key.value());
#endif

  example ex;
  uint64_t counter{0};
  web->on_request("/c", &simple_handler)
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
          "/api/#",
          [](const request& req, const std::shared_ptr<response>& res) {
            res->send(
                status_code::ok,
                std::format("I'm capturing one group for API endpoints: {}",
                            req.get_param(0)));
          })
      .on_fallback([](const request&, const std::shared_ptr<response>& res) {
        res->send(status_code::not_found, "404 Not Found");
      });

  std::atomic_uint64_t async_counter{0};
  web->on_async_request(
      "/async", [&async_counter](const request&,
                                 const std::shared_ptr<async_response>& res) {
        std::thread thread([&async_counter, res] {
          std::this_thread::sleep_for(std::chrono::seconds(5));
          auto count = async_counter.fetch_add(1);
          res->send(status_code::ok,
                    std::format("I'm an async callback ({} calls).", count));
        });

        thread.detach();
      });

  while (is_running) {
    server.poll(100);
  }

  return 0;
}