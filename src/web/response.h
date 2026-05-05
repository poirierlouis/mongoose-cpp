#ifndef MONGOOSE_CPP_RESPONSE_H
#define MONGOOSE_CPP_RESPONSE_H

#include <mongoose.h>

#include <memory>
#include <string>
#include <unordered_map>

#include "remote_context.h"
#include "status_code.h"
#include "web_common.h"

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

  template <typename F>
  void stream(status_code code, F&& callback) {
    stream(static_cast<int>(code), "chunked", std::forward<F>(callback));
  }

  template <typename F>
  void stream(status_code code, std::string encoding, F&& callback) {
    stream(static_cast<int>(code), std::move(encoding),
           std::forward<F>(callback));
  }

  template <typename F>
  void stream(int code, F&& callback) {
    stream(code, "chunked", std::forward<F>(callback));
  }

  template <typename F>
  void stream(int code, std::string encoding, F&& callback) {
    if (!m_conn) {
      return;
    }

    auto producer =
        std::make_unique<lambda_stream_producer<F>>(std::forward<F>(callback));

    m_headers.erase("Content-Length");
    set_header("Transfer-Encoding", std::move(encoding));
    const auto preamble =
        std::format("HTTP/1.1 {} {}\r\n{}\r\n", code, format_status_code(code),
                    format_headers());
    mg_send(m_conn, preamble.c_str(), preamble.size());

    auto* context = static_cast<remote_context*>(m_conn->fn_data);
    context->set_stream(std::move(producer));

    m_conn = nullptr;
  }
};
}  // namespace mg::http

#endif  // MONGOOSE_CPP_RESPONSE_H
