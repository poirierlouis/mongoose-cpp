#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <mongoose-cpp.hpp>
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

int main(int, char**) {
  std::signal(SIGINT, &signal_handler);

  mg::server server(
      [](const std::string_view message) {
        std::cout << "[mg::server] " << message;
      },
      MG_LL_DEBUG);
  if (!server.listen("http://127.0.0.1:4200")) {
    std::cerr << "[mg::server] Failed to listen on http://127.0.0.1:4200"
              << '\n';
    return 1;
  }

  example ex;
  server.register_http("/c", &simple_handler);
  server.register_http("/class", &ex, &example::simple);
  uint64_t counter{0};
  server.register_http(
      "/lambda",
      [&counter](const request&, const std::shared_ptr<response>& res) {
        res->send(status_code::ok,
                  std::format("I'm a lambda callback ({} calls).", ++counter));
      });
  server.register_http(
      "/api/#", [](const request& req, const std::shared_ptr<response>& res) {
        res->send(status_code::ok,
                  std::format("I'm capturing one group for API endpoints: {}",
                              req.get_param(0)));
      });

  std::atomic_uint64_t async_counter{0};
  server.register_async_http(
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