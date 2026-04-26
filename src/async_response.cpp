#include "async_response.h"

namespace mg::http {
async_response::async_response(const unsigned long conn, mg_mgr* mgr)
    : response(nullptr), m_conn(conn), m_mgr(mgr) {}

void async_response::send(const status_code code) const {
  async_response::send(static_cast<int>(code));
}

void async_response::send(const status_code code, const std::string& body) const {
  async_response::send(static_cast<int>(code), body);
}

void async_response::send(const int code) const {
  async_response::send(code, "");
}

void async_response::send(const int code, const std::string& body) const {
  auto* data = new (std::nothrow) payload;
  if (!data) {
    mg_wakeup(m_mgr, m_conn, nullptr, 0);
    return;
  }

  data->code = code;
  data->headers = format_headers();
  data->body = body;

  // FIXME: memory leak when remote close connection
  if (!mg_wakeup(m_mgr, m_conn, &data, sizeof(payload*))) {
    delete data;
  }
}
}  // namespace mg::http