#include "mgxx/http/async_response.hpp"

#include <format>

namespace mgxx::http {
async_response::async_response(const unsigned long endpoint_conn,
                               const unsigned long conn,
                               internal::queue_response* queue,
                               internal::queue_stream* stream,
                               std::weak_ptr<mg_mgr> mgr)
    : m_endpoint_conn(endpoint_conn),
      m_conn(conn),
      m_queue_response(queue),
      m_queue_stream(stream),
      m_mgr(std::move(mgr)) {}

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
  const auto mgr = m_mgr.lock();
  if (!mgr) {
    MG_ERROR(("errmsg=\"Failed to send response on %u, endpoint is lost.\"",
              m_conn));
    return;
  }

  internal::payload data;
  data.conn = m_conn;
  data.code = code;
  data.headers = m_headers.format();
  data.body = body;

  if (!m_queue_response->push(std::move(data))) {
    MG_ERROR(
        ("errmsg=\"Failed to send response on %u, queue is full.\"", m_conn));
    m_mgr.reset();
    return;
  }

  if (!mg_wakeup(mgr.get(), m_endpoint_conn, nullptr, 0)) {
    MG_ERROR(("errmsg=\"Failed to wakeup endpoint %u.\"", m_endpoint_conn));
  }
  m_mgr.reset();
}

std::unique_ptr<async_stream> async_response::stream(status_code code,
                                                     std::string encoding) {
  return stream(static_cast<int>(code), std::move(encoding));
}

std::unique_ptr<async_stream> async_response::stream(const int code,
                                                     std::string encoding) {
  const auto mgr = m_mgr.lock();
  if (!mgr) {
    MG_ERROR(("errmsg=\"Failed to stream response on %u, endpoint is lost.\"",
              m_conn));
    return nullptr;
  }

  m_headers.remove("Content-Length");
  m_headers.set("Transfer-Encoding", std::move(encoding));
  auto preamble = std::format("HTTP/1.1 {} {}\r\n{}\r\n", code,
                              format_status_code(code), m_headers.format());

  auto stream = std::make_unique<async_stream>(m_endpoint_conn, m_conn, m_mgr,
                                               m_queue_stream);
  const auto stream_ptr = stream.get();
  if (!mg_wakeup(mgr.get(), m_conn, &stream_ptr, sizeof(async_stream*))) {
    MG_ERROR(("errmsg=\"Failed to stream response on %u, wakeup is full.\"",
              m_conn));
    return nullptr;
  }

  internal::payload_stream payload;
  payload.conn = m_conn;
  payload.state = internal::payload_stream::state::preamble;
  payload.data = std::move(preamble);
  if (!m_queue_stream->push(std::move(payload))) {
    MG_ERROR(
        ("errmsg=\"Failed to stream response on %u, queue is full.\"", m_conn));
    return nullptr;
  }

  if (!mg_wakeup(mgr.get(), m_endpoint_conn, nullptr, 0)) {
    MG_ERROR(("errmsg=\"Failed to stream response on %u, wakeup is full.\"",
              m_conn));
    return nullptr;
  }

  return stream;
}
}  // namespace mgxx::http