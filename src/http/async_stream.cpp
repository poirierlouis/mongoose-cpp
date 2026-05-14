#include "mgxx/http/async_stream.hpp"

#include "mgxx/http/internal/payload.hpp"

namespace mgxx::http {
constexpr auto k_closed_sentinel = -1000000;
constexpr auto k_closed_threshold = k_closed_sentinel / 2;

async_stream::async_stream(const unsigned long endpoint_conn,
                           const unsigned long conn, std::weak_ptr<mg_mgr> mgr,
                           internal::queue_stream* queue)
    : m_endpoint_conn(endpoint_conn),
      m_conn(conn),
      m_mgr(std::move(mgr)),
      m_queue(queue),
      m_signal(MGXX_HTTP_ASYNC_STREAM_MAX_TICKETS),
      m_closed(false) {}

bool async_stream::is_closed() const {
  return m_signal.load(std::memory_order_acquire) <= k_closed_threshold;
}

bool async_stream::send(std::string body) {
  if (m_closed) {
    return false;
  }

  const auto mgr = m_mgr.lock();
  if (!mgr) {
    MG_ERROR(("errmsg=\"Failed to stream on %u, endpoint is lost.\"", m_conn));
    return false;
  }

  m_signal.fetch_sub(1, std::memory_order_acquire);
  if (m_signal.load(std::memory_order_relaxed) <= k_closed_threshold) {
    return false;
  }

  internal::payload_stream payload;
  payload.conn = m_conn;
  payload.state = internal::payload_stream::state::chunk;
  payload.data = std::move(body);

  if (!m_queue->push(std::move(payload))) {
    m_signal.fetch_add(1, std::memory_order_release);
    m_signal.notify_one();
    MG_ERROR(("errmsg=\"Failed to stream on %u, queue is full.\"", m_conn));
    return false;
  }

  return mg_wakeup(mgr.get(), m_endpoint_conn, nullptr, 0);
}

bool async_stream::wait() const {
  auto current = m_signal.load(std::memory_order_acquire);
  while (current <= 0) {
    if (current <= k_closed_threshold) {
      break;
    }

    m_signal.wait(current, std::memory_order_relaxed);
    current = m_signal.load(std::memory_order_acquire);
  }

  return !m_closed;
}

void async_stream::close() {
  (void)send("");
  m_closed = true;
}

void async_stream::mark_empty() {
  m_signal.fetch_add(1, std::memory_order_release);
  m_signal.notify_one();
}

void async_stream::mark_closed() {
  m_signal.store(k_closed_sentinel, std::memory_order_release);
  m_signal.notify_all();
}
}  // namespace mgxx::http