#ifndef MGXX_SERVER_HPP
#define MGXX_SERVER_HPP

#include <mongoose.h>

#include <memory>
#include <string>
#include <vector>

#include "mgxx/endpoint.hpp"
#include "mgxx/http/endpoint.hpp"
#include "mgxx/internal/logger_listener.hpp"

#ifdef poll
#undef poll
#endif

namespace mgxx {
void event_manager_handler(mg_connection* conn, int ev, void* ev_data);
void event_manager_context_handler(mg_connection* conn, int ev, void* ev_data);
void event_manager_logger(char ch, void* param);

class server {
  friend void event_manager_handler(mg_connection*, int, void*);
  friend void event_manager_logger(char, void*);

  struct logger {
    std::string buffer;
    size_t offset{0};
    std::unique_ptr<logger_listener> listener;
  };

  std::shared_ptr<mg_mgr> m_mgr{};
  bool m_wakeup{false};

  logger m_logger{};

  std::vector<std::shared_ptr<endpoint>> m_endpoints{};

  void start();
  void stop();
  void register_logger(int level, size_t size);

 public:
  server();
  explicit server(int level);
  template <typename F>
  explicit server(F&& callback, const int level = MG_LL_DEBUG,
                  const size_t size = 8192) {
    m_logger.listener =
        std::make_unique<lambda_logger_listener<F>>(std::forward<F>(callback));
    register_logger(level, size);
    start();
  }
  ~server();

  server(const server&) = delete;
  server& operator=(const server&) = delete;

  server(server&&) noexcept = delete;
  server& operator=(server&&) = delete;

  [[nodiscard]] bool is_async() const;

  std::shared_ptr<http::endpoint> listen_http(const std::string& host);
  void poll(int ms = 0) const;
};
}  // namespace mgxx

#endif  // MGXX_SERVER_HPP