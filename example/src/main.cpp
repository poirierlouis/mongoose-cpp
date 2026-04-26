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

void signal_handler(int) { is_running = false; }

int main(int, char**) {
  std::signal(SIGINT, &signal_handler);

  example ex;

  mg::server server;
  if (!server.listen("http://127.0.0.1:4200")) {
    std::cerr << "Failed to listen on http://127.0.0.1:4200" << '\n';
    return 1;
  }
  server.register_http("/c", &simple_handler);
  server.register_http("/class", &ex, &example::simple);
  server.register_http(
      "/lambda", [](const request&, const std::shared_ptr<response>& res) {
        res->send(status_code::ok, "I'm a lambda callback.");
      });
  server.register_http(
      "/api/#", [](const request& req, const std::shared_ptr<response>& res) {
        res->send(status_code::ok,
                  std::format("I'm capturing one group for API endpoints: {}",
                              req.get_param(0)));
      });
  server.register_async_http("/async", [](const request&, const std::shared_ptr<async_response>& res) {
    std::thread thread([res] {
      std::this_thread::sleep_for(std::chrono::seconds(5));
      res->send(status_code::ok, "I'm an async callback with 5 seconds of delay.");
    });

    thread.detach();
  });
  while (is_running) {
    server.poll(100);
  }
  return 0;
}