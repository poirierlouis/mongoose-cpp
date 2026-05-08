#ifndef MGXX_HTTP_REQUEST_HPP
#define MGXX_HTTP_REQUEST_HPP

#include <mongoose.h>

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "mgxx/http/async_request.hpp"
#include "mgxx/internal/remote_context.hpp"

namespace mgxx::http {
class request {
  mg_http_message* m_msg;
  std::vector<mg_str> m_groups;
  std::string_view m_ip;
  std::weak_ptr<tls_cert_info> m_tls_cert_info;

 public:
  explicit request(mg_http_message* msg, const remote_context& context);
  explicit request(mg_http_message* msg, const remote_context& context,
                   std::vector<mg_str> groups);

  request(const request&) = delete;
  request& operator=(const request&) = delete;

  request(request&&) noexcept = default;
  request& operator=(request&&) = default;

  [[nodiscard]] std::string_view get_remote_ip() const;
  [[nodiscard]] std::weak_ptr<tls_cert_info> get_tls_cert_info() const;

  [[nodiscard]] std::string_view method() const;
  [[nodiscard]] std::string_view uri() const;
  [[nodiscard]] std::optional<std::string_view> get_param(size_t index) const;
  [[nodiscard]] std::string_view query() const;
  [[nodiscard]] std::string_view version() const;
  [[nodiscard]] std::optional<std::string_view> get_header(
      const std::string& name) const;
  [[nodiscard]] std::string_view body() const;

  [[nodiscard]] std::unique_ptr<async_request> to_async() const;
};
}  // namespace mgxx::http

#endif  // MGXX_HTTP_REQUEST_HPP