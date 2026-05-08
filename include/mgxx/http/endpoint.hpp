#ifndef MGXX_HTTP_ENDPOINT_HPP
#define MGXX_HTTP_ENDPOINT_HPP

#include <mongoose.h>

#include <memory>
#include <string>
#include <unordered_map>

#include "mgxx/endpoint.hpp"
#include "mgxx/http/async_response.hpp"
#include "mgxx/http/listener.hpp"
#include "mgxx/http/response.hpp"

namespace mgxx::http {
class endpoint : public mgxx::endpoint {
  using responses = std::unordered_map<size_t, std::shared_ptr<async_response>>;
  using streams = std::unordered_map<size_t, std::shared_ptr<async_stream>>;

  mg_str m_tls_ca{};
  mg_str m_tls_cert{};
  mg_str m_tls_key{};
  bool m_tls_alloc{false};

  std::unordered_map<std::string, std::unique_ptr<listener>> m_listeners{};
  std::unique_ptr<listener> m_fallback{};

  size_t m_request_counter{0};
  std::unordered_map<unsigned long, responses> m_pending_responses{};
  std::unordered_map<unsigned long, streams> m_pending_streams{};

  void handle_poll(mg_connection* conn);
  void handle_secure(mg_connection* conn) const;
  void handle_http_message(mg_connection* conn, mg_http_message* msg);
  void handle_wakeup(mg_connection* conn, const mg_str* data);
  void handle_write(mg_connection* conn);
  void handle_close(const mg_connection* conn);

  [[nodiscard]] bool handle_response(mg_connection* conn, const mg_str* data);
  [[nodiscard]] bool handle_stream(mg_connection* conn, const mg_str* data);

  void promote_context(mg_connection* conn);
  void setup_context(const mg_connection* conn);

  [[nodiscard]] static size_t count_groups(std::string_view path);

 protected:
  void handle(mg_connection* conn, int ev, void* ev_data) override;

 public:
  explicit endpoint(std::weak_ptr<mg_mgr> mgr, std::string host);
  ~endpoint() override;

  [[nodiscard]] bool is_mtls() const;

  void use_tls(const std::string& cert, const std::string& key);
  void use_tls(const std::string& ca, const std::string& cert,
               const std::string& key);
  void use_tls(std::string_view cert, std::string_view key);
  void use_tls(std::string_view ca, std::string_view cert,
               std::string_view key);

  template <typename F>
  endpoint& on_request(const std::string& path, F&& callback) {
    m_listeners.insert_or_assign(
        path, std::make_unique<lambda_http_listener<F, response>>(
                  std::forward<F>(callback), count_groups(path)));
    return *this;
  }

  template <typename F>
  endpoint& on_async_request(const std::string& path, F&& callback) {
    m_listeners.insert_or_assign(
        path, std::make_unique<lambda_http_listener<F, async_response>>(
                  std::forward<F>(callback), count_groups(path)));
    return *this;
  }

  template <typename F>
  endpoint& on_fallback(F&& callback) {
    m_fallback = std::make_unique<lambda_http_listener<F, response>>(
        std::forward<F>(callback), 0);
    return *this;
  }

  template <typename F>
  endpoint& on_async_fallback(F&& callback) {
    m_fallback = std::make_unique<lambda_http_listener<F, async_response>>(
        std::forward<F>(callback), 0);
    return *this;
  }
};
}  // namespace mgxx::http

#endif  // MGXX_HTTP_ENDPOINT_HPP
