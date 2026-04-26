#ifndef MONGOOSE_CPP_EXAMPLE_ASYNC_RESPONSE_H
#define MONGOOSE_CPP_EXAMPLE_ASYNC_RESPONSE_H

#include <mongoose.h>

#include "response.h"

namespace mg {
class server;
}

namespace mg::http {

class async_response : public response {
  struct payload {
    int code;
    std::string headers;
    std::string body;
  };

  const unsigned long m_conn;
  mg_mgr* m_mgr;

  friend class mg::server;

 public:
  explicit async_response(unsigned long conn, mg_mgr* mgr);
  ~async_response() override = default;

  void send(status_code code) const override;
  void send(status_code code, const std::string& body) const override;
  void send(int code) const override;
  void send(int code, const std::string& body) const override;
};
}  // namespace mg::http

#endif  // MONGOOSE_CPP_EXAMPLE_ASYNC_RESPONSE_H
