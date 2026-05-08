#include "mgxx/http/async_response.hpp"

#include <format>

namespace mgxx::http {
async_response::async_response(const unsigned long conn,
                               std::weak_ptr<mg_mgr> mgr, const size_t id)
    : m_conn(conn), m_mgr(std::move(mgr)), m_id(id) {}

headers& async_response::get_headers() { return m_headers; }

const headers& async_response::get_headers() const { return m_headers; }

void async_response::send(const status_code code) {
  send(static_cast<int>(code));
}

void async_response::send(const status_code code, const std::string& body) {
  send(static_cast<int>(code), body);
}

void async_response::send(const int code) { send(code, ""); }

void async_response::send(const int code, const std::string& body) {
  if (m_state != state::pending) {
    return;
  }

  if (const auto mgr = m_mgr.lock()) {
    m_payload.code = code;
    m_payload.headers = m_headers.format();
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

std::shared_ptr<async_stream> async_response::stream(status_code code,
                                                     std::string encoding) {
  return stream(static_cast<int>(code), std::move(encoding));
}

std::shared_ptr<async_stream> async_response::stream(const int code,
                                                     std::string encoding) {
  if (m_state != state::pending) {
    return nullptr;
  }

  if (const auto mgr = m_mgr.lock()) {
    m_headers.remove("Content-Length");
    m_headers.set("Transfer-Encoding", std::move(encoding));

    const auto preamble =
        std::format("HTTP/1.1 {} {}\r\n{}\r\n", code, format_status_code(code),
                    m_headers.format());
    m_stream = std::make_shared<async_stream>(m_mgr, m_conn, m_id, preamble);
    MG_DEBUG(("Waking-up %d on connection %d", m_id, m_conn));
    if (!mg_wakeup(mgr.get(), m_conn, &m_id, sizeof(m_id))) {
      m_state.store(state::failed, std::memory_order_release);
      return nullptr;
    }

    m_state.store(state::streaming, std::memory_order_release);
    return m_stream;
  }

  m_state = state::failed;
  return nullptr;
}

void async_response::mark_completed() { m_state = state::completed; }

void async_response::mark_failed() { m_state = state::failed; }

size_t async_response::get_id() const { return m_id; }

const async_response::payload& async_response::get_payload() const {
  return m_payload;
}

async_response::state async_response::get_state() const {
  return m_state.load(std::memory_order_acquire);
}

std::shared_ptr<async_stream> async_response::get_stream() {
  return std::move(m_stream);
}
}  // namespace mgxx::http