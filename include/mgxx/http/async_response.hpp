#ifndef MGXX_HTTP_ASYNC_RESPONSE_HPP
#define MGXX_HTTP_ASYNC_RESPONSE_HPP

#include <mongoose.h>

#include <memory>
#include <string>

#include "mgxx/http/async_stream.hpp"
#include "mgxx/http/headers.hpp"
#include "mgxx/http/internal/common.hpp"
#include "mgxx/http/internal/payload.hpp"

namespace mgxx::http {
class async_response {
 public:
  enum class state { pending, sending, streaming, completed, failed };

 private:
  const unsigned long m_endpoint_conn;
  const unsigned long m_conn;
  internal::queue_response* m_queue_response{nullptr};
  internal::queue_stream* m_queue_stream{nullptr};
  std::weak_ptr<mg_mgr> m_mgr;

  headers m_headers;

  friend class endpoint;

 public:
  explicit async_response(unsigned long endpoint_conn, unsigned long conn,
                          internal::queue_response* queue,
                          internal::queue_stream* stream,
                          std::weak_ptr<mg_mgr> mgr);

  [[nodiscard]] headers& get_headers();
  [[nodiscard]] const headers& get_headers() const;

  void send(status_code code);
  void send(status_code code, const std::string& body);

  void send(int code);
  void send(int code, const std::string& body);

  [[nodiscard]] std::unique_ptr<async_stream> stream(
      status_code code, std::string encoding = "chunked");
  [[nodiscard]] std::unique_ptr<async_stream> stream(
      int code, std::string encoding = "chunked");
};
}  // namespace mgxx::http

#endif  // MGXX_HTTP_ASYNC_RESPONSE_HPP
