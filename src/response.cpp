#include "response.h"

namespace mg::http {
response::response(mg_connection* conn) : m_conn(conn) {}

void response::set_header(std::string name, std::string value) {
  m_headers[std::move(name)] = std::move(value);
}

void response::send(const status_code code) const {
  send(static_cast<int>(code));
}

void response::send(const status_code code, const std::string& body) const {
  send(static_cast<int>(code), body);
}

void response::send(const int code) const {
  const auto headers = format_headers();
  mg_http_reply(m_conn, code, headers.c_str(), "\n");
}

void response::send(const int code, const std::string& body) const {
  const auto headers = format_headers();
  mg_http_reply(m_conn, code, headers.c_str(), "%s\n", body.c_str());
}

std::string response::format_headers() const {
  std::string headers;
  for (const auto& [name, value] : m_headers) {
    headers.append(name);
    headers.append(": ");
    headers.append(value);
    headers.append("\r\n");
  }
  return headers;
}
}  // namespace mg::http
