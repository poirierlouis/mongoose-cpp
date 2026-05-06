#include "response.h"

namespace mg::http {
response::response(mg_connection* conn) : m_conn(conn) {}

headers& response::get_headers() { return m_headers; }

const headers& response::get_headers() const { return m_headers; }

void response::send(const status_code code) { send(static_cast<int>(code)); }

void response::send(const status_code code, const std::string& body) {
  send(static_cast<int>(code), body);
}

void response::send(const int code) { send(code, ""); }

void response::send(const int code, const std::string& body) {
  if (!m_conn) {
    return;
  }

  const auto headers = m_headers.format();
  mg_http_reply(m_conn, code, headers.c_str(), "%s\n", body.c_str());
  m_conn = nullptr;
}
}  // namespace mg::http
