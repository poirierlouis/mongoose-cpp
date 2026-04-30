#ifndef MONGOOSE_CPP_RESPONSE_H
#define MONGOOSE_CPP_RESPONSE_H

#include <mongoose.h>

#include <string>
#include <unordered_map>

#include "status_code.h"

namespace mg::http {
namespace not_found {
constexpr int k_code = 404;
constexpr auto k_header = "Content-Type: text/plain\r\n";
constexpr auto k_body = "Not Found\n";
}  // namespace not_found

namespace internal_server_error {
constexpr int k_code = 500;
constexpr auto k_header = "Content-Type: text/plain\r\n";
constexpr auto k_body = "Internal Server Error\n";
}  // namespace internal_server_error

class response {
  mg_connection* m_conn;
  std::unordered_map<std::string, std::string> m_headers;

 protected:
  [[nodiscard]] std::string format_headers() const;

 public:
  explicit response(mg_connection* conn);
  virtual ~response() = default;

  response(const response&) = delete;
  response& operator=(const response&) = delete;

  response(response&&) noexcept = default;
  response& operator=(response&&) = default;

  void set_header(std::string name, std::string value);

  virtual void send(status_code code);
  virtual void send(status_code code, const std::string& body);
  virtual void send_json(status_code code, const std::string& body);

  virtual void send(int code);
  virtual void send(int code, const std::string& body);
  virtual void send_json(int code, const std::string& body);
};
}  // namespace mg::http

#endif  // MONGOOSE_CPP_RESPONSE_H
