#ifndef MONGOOSE_CPP_ASYNC_RESPONSE_H
#define MONGOOSE_CPP_ASYNC_RESPONSE_H

#include <mongoose.h>

#include <memory>

#include "response.h"

namespace mg {
class server;
}

namespace mg::http {

class async_response : public response {
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

  payload m_payload{};
  std::atomic<state> m_state{state::pending};

  void mark_completed();
  void mark_failed();

  friend class mg::server;

 public:
  explicit async_response(unsigned long conn, std::weak_ptr<mg_mgr> mgr,
                          size_t id);
  ~async_response() override = default;

  [[nodiscard]] size_t get_id() const;
  [[nodiscard]] const payload& get_payload() const;
  [[nodiscard]] state get_state() const;

  void send(status_code code) override;
  void send(status_code code, const std::string& body) override;
  void send_json(status_code code, const std::string& body) override;

  void send(int code) override;
  void send(int code, const std::string& body) override;
  void send_json(int code, const std::string& body) override;
};
}  // namespace mg::http

#endif  // MONGOOSE_CPP_ASYNC_RESPONSE_H
