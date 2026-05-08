#include "mgxx/http/async_stream.hpp"

namespace mgxx::http {
async_stream::async_stream(std::weak_ptr<mg_mgr> mgr, const unsigned long conn,
                           const size_t id, std::string preamble)
    : m_mgr(std::move(mgr)),
      m_conn(conn),
      m_id(id),
      m_data(std::move(preamble)) {}

bool async_stream::wait() const {
  auto current = m_state.load(std::memory_order_acquire);
  while (current != state::empty) {
    if (current == state::failed || current == state::completed) {
      return false;
    }

    m_state.wait(current);
    current = m_state.load(std::memory_order_acquire);
  }

  return true;
}

bool async_stream::failed() const { return m_state == state::failed; }

void async_stream::send(std::string body) {
  m_data = std::move(body);
  m_state.store(state::has_data, std::memory_order_release);
  if (const auto mgr = m_mgr.lock()) {
    if (!mg_wakeup(mgr.get(), m_conn, &m_id, sizeof(m_id))) {
      m_state = state::failed;
    }
  }
}

void async_stream::close() {
  m_data.clear();
  m_state.store(state::close, std::memory_order_release);
  if (const auto mgr = m_mgr.lock()) {
    if (!mg_wakeup(mgr.get(), m_conn, &m_id, sizeof(m_id))) {
      m_state = state::failed;
    }
  }
}

size_t async_stream::get_id() const { return m_id; }

std::string async_stream::get_data() { return std::move(m_data); }

async_stream::state async_stream::get_state() const {
  return m_state.load(std::memory_order_acquire);
}

void async_stream::mark_flush() {
  m_state.store(state::flush, std::memory_order_release);
  m_state.notify_one();
}

void async_stream::mark_empty() {
  m_state.store(state::empty, std::memory_order_release);
  m_state.notify_one();
}

void async_stream::mark_completed() {
  m_state.store(state::completed, std::memory_order_release);
  m_state.notify_one();
}

void async_stream::mark_failed() {
  m_state.store(state::failed, std::memory_order_release);
  m_state.notify_all();
}
}  // namespace mgxx::http