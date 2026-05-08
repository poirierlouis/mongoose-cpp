#ifndef MGXX_HTTP_ASYNC_STREAM_HPP
#define MGXX_HTTP_ASYNC_STREAM_HPP

#include <mongoose.h>

#include <atomic>
#include <memory>
#include <string>

namespace mgxx::http {
class endpoint;
class remote_context;

class async_stream {
 public:
  enum class state { start, has_data, flush, empty, close, completed, failed };

 private:
  std::weak_ptr<mg_mgr> m_mgr;
  unsigned long m_conn;
  size_t m_id;
  std::string m_data;
  std::atomic<state> m_state{state::start};

  [[nodiscard]] size_t get_id() const;
  [[nodiscard]] std::string get_data();
  [[nodiscard]] state get_state() const;

  void mark_flush();
  void mark_empty();
  void mark_completed();
  void mark_failed();

  friend class mgxx::http::endpoint;
  friend class mgxx::http::remote_context;

 public:
  async_stream(std::weak_ptr<mg_mgr> mgr, unsigned long conn, size_t id,
               std::string preamble);

  [[nodiscard]] bool wait() const;
  [[nodiscard]] bool failed() const;
  void send(std::string body);
  void close();
};
}  // namespace mgxx::http

#endif  // MGXX_HTTP_ASYNC_STREAM_HPP
