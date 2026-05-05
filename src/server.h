#ifndef MONGOOSE_CPP_SERVER_H
#define MONGOOSE_CPP_SERVER_H

#include <mongoose.h>

#include <memory>
#include <string>
#include <vector>

#include "endpoint.h"
#include "http/endpoint.h"
#include "logger_listener.h"

#ifdef poll
#undef poll
#endif

namespace mg {
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

  void register_logger(int level, size_t size);
  void setup();

 public:
  server();
  explicit server(int level);

  template <typename F>
  explicit server(F&& callback, const int level = MG_LL_DEBUG,
                  const size_t size = 8192) {
    m_logger.listener =
        std::make_unique<lambda_logger_listener<F>>(std::forward<F>(callback));
    register_logger(level, size);
    setup();
  }

  ~server() = default;

  server(const server&) = delete;
  server& operator=(const server&) = delete;

  server(server&&) noexcept = delete;
  server& operator=(server&&) = delete;

  [[nodiscard]] bool is_async() const;

  std::shared_ptr<http::endpoint> listen_http(const std::string& host);
  void poll(int ms = 0) const;
};
}  // namespace mg

#endif  // MONGOOSE_CPP_SERVER_H