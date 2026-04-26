#ifndef MONGOOSE_CPP_RESPONSE_H
#define MONGOOSE_CPP_RESPONSE_H

#include <string>
#include <unordered_map>

#include "mongoose.h"
#include "status_code.h"

namespace mg::http {
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

  virtual void send(status_code code) const;

  virtual void send(status_code code, const std::string& body) const;

  virtual void send(int code) const;

  virtual void send(int code, const std::string& body) const;
};
}  // namespace mg::http

#endif  // MONGOOSE_CPP_RESPONSE_H
