#ifndef MONGOOSE_CPP_HTTP_RESPONSE_H
#define MONGOOSE_CPP_HTTP_RESPONSE_H

#include <mongoose.h>

#include <memory>
#include <string>

#include "common.h"
#include "headers.h"
#include "remote_context.h"

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
  headers m_headers;

 public:
  explicit response(mg_connection* conn);

  response(const response&) = delete;
  response& operator=(const response&) = delete;

  response(response&&) noexcept = default;
  response& operator=(response&&) = default;

  [[nodiscard]] headers& get_headers();
  [[nodiscard]] const headers& get_headers() const;

  void send(status_code code);
  void send(status_code code, const std::string& body);

  void send(int code);
  void send(int code, const std::string& body);

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

    m_headers.remove("Content-Length");
    m_headers.set("Transfer-Encoding", std::move(encoding));
    const auto preamble =
        std::format("HTTP/1.1 {} {}\r\n{}\r\n", code, format_status_code(code),
                    m_headers.format());
    mg_send(m_conn, preamble.c_str(), preamble.size());

    auto* context = static_cast<remote_context*>(m_conn->fn_data);
    context->set_stream(std::move(producer));

    m_conn = nullptr;
  }
};
}  // namespace mg::http

#endif  // MONGOOSE_CPP_HTTP_RESPONSE_H
