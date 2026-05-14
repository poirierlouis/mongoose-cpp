#ifndef MGXX_HTTP_ASYNC_STREAM_HPP
#define MGXX_HTTP_ASYNC_STREAM_HPP

#include <mongoose.h>

#include <atomic>
#include <memory>
#include <string>

#include "mgxx/http/internal/payload.hpp"

#ifndef MGXX_HTTP_ASYNC_STREAM_MAX_TICKETS
#define MGXX_HTTP_ASYNC_STREAM_MAX_TICKETS 4
#endif

namespace mgxx::http {
class endpoint;
class remote_context;

class async_stream {
  unsigned long m_endpoint_conn;
  unsigned long m_conn;
  std::weak_ptr<mg_mgr> m_mgr;
  internal::queue_stream* m_queue;
  std::atomic_signed_lock_free m_signal;

  bool m_closed;

  friend class mgxx::http::endpoint;
  friend class mgxx::http::remote_context;

  void mark_empty();
  void mark_closed();

 public:
  explicit async_stream(unsigned long endpoint_conn, unsigned long conn,
                        std::weak_ptr<mg_mgr> mgr,
                        internal::queue_stream* queue);

  async_stream(const async_stream&) = delete;
  async_stream& operator=(const async_stream&) = delete;

  async_stream(async_stream&&) noexcept = delete;
  async_stream& operator=(async_stream&&) = delete;

  [[nodiscard]] bool is_closed() const;

  [[nodiscard]] bool send(std::string body);
  [[nodiscard]] bool wait() const;
  void close();
};
}  // namespace mgxx::http

#endif  // MGXX_HTTP_ASYNC_STREAM_HPP
