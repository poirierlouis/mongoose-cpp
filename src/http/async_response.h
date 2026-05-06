#ifndef MONGOOSE_CPP_HTTP_ASYNC_RESPONSE_H
#define MONGOOSE_CPP_HTTP_ASYNC_RESPONSE_H

#include <mongoose.h>

#include <atomic>
#include <memory>
#include <string>

#include "common.h"
#include "headers.h"

namespace mg::http {
class endpoint;

class async_response {
 public:
  enum class state { pending, sending, completed, failed };

 private:
  struct payload {
    int code{-1};
    std::string headers;
    std::string body;
  };

  const unsigned long m_conn;
  std::weak_ptr<mg_mgr> m_mgr;
  const size_t m_id;

  headers m_headers;
  payload m_payload{};
  std::atomic<state> m_state{state::pending};

  void mark_completed();
  void mark_failed();

  friend class endpoint;

 public:
  explicit async_response(unsigned long conn, std::weak_ptr<mg_mgr> mgr,
                          size_t id);

  [[nodiscard]] size_t get_id() const;
  [[nodiscard]] const payload& get_payload() const;
  [[nodiscard]] state get_state() const;

  [[nodiscard]] headers& get_headers();
  [[nodiscard]] const headers& get_headers() const;

  void send(status_code code);
  void send(status_code code, const std::string& body);

  void send(int code);
  void send(int code, const std::string& body);
};
}  // namespace mg::http

#endif  // MONGOOSE_CPP_HTTP_ASYNC_RESPONSE_H
