#ifndef MONGOOSE_CPP_HTTP_REQUEST_H
#define MONGOOSE_CPP_HTTP_REQUEST_H

#include <mongoose.h>

#include <string>
#include <string_view>
#include <vector>

namespace mg::http {
class request {
  mg_http_message* m_msg;
  std::vector<mg_str> m_groups;

 public:
  explicit request(mg_http_message* msg);
  explicit request(mg_http_message* msg, std::vector<mg_str> groups);

  request(const request&) = delete;

  request& operator=(const request&) = delete;

  request(request&&) noexcept = default;

  request& operator=(request&&) = default;

  [[nodiscard]] std::string_view method() const;

  [[nodiscard]] std::string_view uri() const;

  [[nodiscard]] std::string_view get_param(size_t index) const;

  [[nodiscard]] std::string_view query() const;

  [[nodiscard]] std::string_view version() const;

  [[nodiscard]] std::string_view get_header(const std::string& name) const;

  [[nodiscard]] std::string_view body() const;
};
}  // namespace mg::http

#endif  // MONGOOSE_CPP_HTTP_REQUEST_H
