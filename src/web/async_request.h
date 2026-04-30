#ifndef MONGOOSE_CPP_ASYNC_REQUEST_H
#define MONGOOSE_CPP_ASYNC_REQUEST_H

#include <mongoose.h>

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace mg::http {
class async_request {
  mg_http_message m_msg{};
  std::vector<mg_str> m_groups;

 public:
  explicit async_request(const mg_http_message* msg);
  explicit async_request(const mg_http_message* msg,
                         std::vector<mg_str> groups);
  ~async_request();

  async_request(const async_request&) = delete;
  async_request& operator=(const async_request&) = delete;

  async_request(async_request&&) noexcept = delete;
  async_request& operator=(async_request&&) = delete;

  [[nodiscard]] std::string_view method() const;
  [[nodiscard]] std::string_view uri() const;
  [[nodiscard]] std::optional<std::string_view> get_param(size_t index) const;
  [[nodiscard]] std::string_view query() const;
  [[nodiscard]] std::string_view version() const;
  [[nodiscard]] std::optional<std::string_view> get_header(
      const std::string& name) const;
  [[nodiscard]] std::string_view body() const;
};
}  // namespace mg::http

#endif  // MONGOOSE_CPP_ASYNC_REQUEST_H
