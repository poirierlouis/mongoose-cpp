#include "async_response.h"

namespace mg::http {
async_response::async_response(const unsigned long conn,
                               std::weak_ptr<mg_mgr> mgr, const size_t id)
    : response(nullptr), m_conn(conn), m_mgr(std::move(mgr)), m_id(id) {}

size_t async_response::get_id() const { return m_id; }

const async_response::payload& async_response::get_payload() const {
  return m_payload;
}

async_response::state async_response::get_state() const {
  return m_state.load();
}

void async_response::send(const status_code code) {
  async_response::send(static_cast<int>(code));
}

void async_response::send(const status_code code, const std::string& body) {
  async_response::send(static_cast<int>(code), body);
}

void async_response::send_json(status_code code, const std::string& body) {
  async_response::send_json(static_cast<int>(code), body);
}

void async_response::send(const int code) { async_response::send(code, ""); }

void async_response::send(const int code, const std::string& body) {
  if (m_state != state::pending) {
    return;
  }

  if (const auto mgr = m_mgr.lock()) {
    m_payload.code = code;
    m_payload.headers = format_headers();
    m_payload.body = body;

    if (!mg_wakeup(mgr.get(), m_conn, &m_id, sizeof(m_id))) {
      m_state = state::failed;
    } else {
      m_state = state::sending;
    }
  } else {
    m_state = state::failed;
  }
}

void async_response::send_json(const int code, const std::string& body) {
  set_header("Content-Type", "application/json");
  async_response::send(code, body);
}

void async_response::mark_completed() { m_state = state::completed; }

void async_response::mark_failed() { m_state = state::failed; }
}  // namespace mg::http