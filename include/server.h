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

  std::shared_ptr<mg_mgr> m_mgr{};
  bool m_wakeup{false};
  size_t m_counter{0};

  std::unordered_map<std::string, std::unique_ptr<http::event_listener>>
      m_http_listeners{};
  std::unique_ptr<http::event_listener> m_http_fallback{};

  std::unordered_map<
      unsigned long,
      std::unordered_map<size_t, std::shared_ptr<http::async_response>>>
      m_http_pending{};

  void handle(mg_connection* conn, int ev, void* ev_data);
  void handle_http_poll(mg_connection* conn);
  void handle_http(mg_connection* conn, mg_http_message* msg);
  void handle_http_wakeup(mg_connection* conn, const mg_str* data);
  void handle_http_close(const mg_connection* conn);

  [[nodiscard]] static size_t count_groups(std::string_view path);

 public:
  server();
  ~server() = default;

  server(const server&) = delete;
  server& operator=(const server&) = delete;

  server(server&&) noexcept = delete;
  server& operator=(server&&) = delete;

  bool listen(const std::string& host);
  bool listen(const std::string& protocol, const std::string& interface,
              uint16_t port);

  [[nodiscard]] bool is_async() const;

  template <typename F>
  void register_http(const std::string& path, F&& callback) {
    m_http_listeners.insert_or_assign(
        path, std::make_unique<http::lambda_event_listener<F, http::response>>(
                  std::forward<F>(callback), count_groups(path)));
  }

  template <typename T>
  void register_http(
      const std::string& path, T* target,
      void (T::*callback)(const http::request&,
                          const std::shared_ptr<http::response>&)) {
    m_http_listeners.insert_or_assign(
        path, std::make_unique<http::klass_event_listener<T, http::response>>(
                  target, callback, count_groups(path)));
  }

  void register_http(const std::string& path,
                     void (*callback)(const http::request&,
                                      const std::shared_ptr<http::response>&)) {
    m_http_listeners.insert_or_assign(
        path, std::make_unique<http::fn_event_listener<http::response>>(
                  callback, count_groups(path)));
  }

  template <typename F>
  void register_http_fallback(F&& callback) {
    m_http_fallback =
        std::make_unique<http::lambda_event_listener<F, http::response>>(
            std::forward<F>(callback), 0);
  }

  template <typename T>
  void register_http_fallback(
      T* target, void (T::*callback)(const http::request&,
                                     const std::shared_ptr<http::response>&)) {
    m_http_fallback =
        std::make_unique<http::klass_event_listener<T, http::response>>(
            target, callback, 0);
  }

  void register_http_fallback(void (*callback)(
      const http::request&, const std::shared_ptr<http::response>&)) {
    m_http_fallback =
        std::make_unique<http::fn_event_listener<http::response>>(callback, 0);
  }

  template <typename F>
  void register_async_http(const std::string& path, F&& callback) {
    if (!m_wakeup) {
      // TODO: log error
      return;
    }

    m_http_listeners.insert_or_assign(
        path,
        std::make_unique<http::lambda_event_listener<F, http::async_response>>(
            std::forward<F>(callback), count_groups(path)));
  }

  template <typename T>
  void register_async_http(
      const std::string& path, T* target,
      void (T::*callback)(const http::request&,
                          const std::shared_ptr<http::async_response>&)) {
    if (!m_wakeup) {
      // TODO: log error
      return;
    }

    m_http_listeners.insert_or_assign(
        path,
        std::make_unique<http::klass_event_listener<T, http::async_response>>(
            target, callback, count_groups(path)));
  }

  void register_async_http(
      const std::string& path,
      void (*callback)(const http::request&,
                       const std::shared_ptr<http::async_response>&)) {
    if (!m_wakeup) {
      // TODO: log error
      return;
    }

    m_http_listeners.insert_or_assign(
        path, std::make_unique<http::fn_event_listener<http::async_response>>(
                  callback, count_groups(path)));
  }

  void poll(int ms = 0) const;
};
}  // namespace mg

#endif  // MONGOOSE_CPP_SERVER_H