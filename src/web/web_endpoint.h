#ifndef MONGOOSE_CPP_WEB_ENDPOINT_H
#define MONGOOSE_CPP_WEB_ENDPOINT_H

#include <mongoose.h>

#include <memory>
#include <string>
#include <unordered_map>

#include "async_response.h"
#include "endpoint.h"
#include "web_listener.h"

namespace mg {
class web_endpoint : public endpoint {
  using responses =
      std::unordered_map<size_t, std::shared_ptr<http::async_response>>;

  mg_str m_tls_cert{};
  mg_str m_tls_key{};
  bool m_tls_alloc{false};

  std::unordered_map<std::string, std::unique_ptr<web_listener>> m_listeners{};
  std::unique_ptr<web_listener> m_fallback{};

  size_t m_counter{0};
  std::unordered_map<unsigned long, responses> m_pending{};

  void handle_poll(mg_connection* conn);
  void handle_secure(mg_connection* conn) const;
  void handle_http_message(mg_connection* conn, mg_http_message* msg);
  void handle_wakeup(mg_connection* conn, const mg_str* data);
  void handle_close(const mg_connection* conn);

  [[nodiscard]] static size_t count_groups(std::string_view path);

 protected:
  void handle(mg_connection* conn, int ev, void* ev_data) override;

 public:
  explicit web_endpoint(std::weak_ptr<mg_mgr> mgr, std::string host);
  ~web_endpoint() override;

  void use_tls(const std::string& cert, const std::string& key);
  void use_tls(std::string_view cert, std::string_view key);

  template <typename F>
  web_endpoint& on_request(const std::string& path, F&& callback) {
    m_listeners.insert_or_assign(
        path, std::make_unique<lambda_web_listener<F, http::response>>(
                  std::forward<F>(callback), count_groups(path)));
    return *this;
  }

  template <typename F>
  web_endpoint& on_async_request(const std::string& path, F&& callback) {
    m_listeners.insert_or_assign(
        path, std::make_unique<lambda_web_listener<F, http::async_response>>(
                  std::forward<F>(callback), count_groups(path)));
    return *this;
  }

  template <typename F>
  web_endpoint& on_fallback(F&& callback) {
    m_fallback = std::make_unique<lambda_web_listener<F, http::response>>(
        std::forward<F>(callback), 0);
    return *this;
  }

  template <typename F>
  web_endpoint& on_async_fallback(F&& callback) {
    m_fallback = std::make_unique<lambda_web_listener<F, http::async_response>>(
        std::forward<F>(callback), 0);
    return *this;
  }
};
}  // namespace mg

#endif  // MONGOOSE_CPP_WEB_ENDPOINT_H
