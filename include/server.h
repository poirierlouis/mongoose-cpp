#ifndef MONGOOSE_CPP_SERVER_H
#define MONGOOSE_CPP_SERVER_H

#include <mongoose.h>

#include <memory>
#include <string>
#include <unordered_map>

#include "event_listener.h"

#ifdef poll
#undef poll
#endif

namespace mg {
void event_manager_handler(mg_connection* conn, int ev, void* ev_data);

class server {
  friend void event_manager_handler(mg_connection*, int, void*);

  mg_mgr m_mgr{};
  std::unordered_map<std::string, std::unique_ptr<http::event_listener>>
      m_http_listeners{};
  std::unique_ptr<http::event_listener> m_http_fallback{};

  void handle(mg_connection* conn, int ev, void* ev_data);

  void handle_http(mg_connection* conn, mg_http_message* msg);

  [[nodiscard]] static size_t count_groups(std::string_view path);

 public:
  server();

  ~server();

  server(const server&) = delete;

  server& operator=(const server&) = delete;

  server(server&&) noexcept = delete;

  server& operator=(server&&) = delete;

  bool listen(const std::string& host);

  bool listen(const std::string& protocol, const std::string& interface,
              uint16_t port);

  template <typename F>
  void register_http(const std::string& path, F&& callback) {
    m_http_listeners.insert_or_assign(
        path, std::make_unique<http::lambda_event_listener<F>>(
                  std::forward<F>(callback), count_groups(path)));
  }

  template <typename T>
  void register_http(
      const std::string& path, T* target,
      void (T::*callback)(const http::request&,
                          const std::shared_ptr<http::response>&)) {
    m_http_listeners.insert_or_assign(
        path, std::make_unique<http::klass_event_listener<T>>(
                  target, callback, count_groups(path)));
  }

  void register_http(const std::string& path,
                     void (*callback)(const http::request&,
                                      const std::shared_ptr<http::response>&)) {
    m_http_listeners.insert_or_assign(
        path, std::make_unique<http::fn_event_listener>(callback,
                                                        count_groups(path)));
  }

  template <typename F>
  void register_http_fallback(F&& callback) {
    m_http_fallback = std::make_unique<http::lambda_event_listener<F>>(
        std::forward<F>(callback, 0));
  }

  template <typename T>
  void register_http_fallback(
      T* target, void (T::*callback)(const http::request&,
                                     const std::shared_ptr<http::response>&)) {
    m_http_fallback =
        std::make_unique<http::klass_event_listener<T>>(target, callback, 0);
  }

  void register_http_fallback(void (*callback)(
      const http::request&, const std::shared_ptr<http::response>&)) {
    m_http_fallback = std::make_unique<http::fn_event_listener>(callback, 0);
  }

  void poll(int ms = 0);
};
}  // namespace mg

#endif  // MONGOOSE_CPP_SERVER_H