#include "async_request.h"

namespace mg::http {
async_request::async_request(const mg_http_message* msg)
    : async_request(msg, {}) {}

async_request::async_request(const mg_http_message* msg,
                             std::vector<mg_str> groups)
    : m_groups(std::move(groups)) {
  m_msg.message = mg_strdup(msg->message);

  ptrdiff_t delta = m_msg.message.buf - msg->message.buf;
  const auto rebase = [delta](const mg_str& str) -> mg_str {
    if (!str.buf) {
      return {.buf = nullptr, .len = 0};
    }
    return {.buf = str.buf + delta, .len = str.len};
  };

  m_msg.method = rebase(msg->method);
  m_msg.uri = rebase(msg->uri);
  m_msg.query = rebase(msg->query);
  m_msg.proto = rebase(msg->proto);

  for (size_t i = 0; i < MG_MAX_HTTP_HEADERS; i++) {
    const auto& header = msg->headers[i];
    auto& m_header = m_msg.headers[i];
    m_header.name = rebase(header.name);
    m_header.value = rebase(header.value);

    if (m_header.name.buf == nullptr) {
      break;
    }
  }

  m_msg.body = rebase(msg->body);

  for (auto& group : m_groups) {
    group = rebase(group);
  }
}

async_request::~async_request() { mg_free(m_msg.message.buf); }

std::string_view async_request::method() const {
  return {m_msg.method.buf, m_msg.method.len};
}

std::string_view async_request::uri() const {
  return {m_msg.uri.buf, m_msg.uri.len};
}

std::string_view async_request::get_param(const size_t index) const {
  if (index >= m_groups.size()) {
    // TODO: use std::optional
    return {};
  }
  return {m_groups[index].buf, m_groups[index].len};
}

std::string_view async_request::query() const {
  return {m_msg.query.buf, m_msg.query.len};
}

std::string_view async_request::version() const {
  return {m_msg.proto.buf, m_msg.proto.len};
}

std::string_view async_request::get_header(const std::string& name) const {
  const auto header =
      mg_http_get_header(const_cast<mg_http_message*>(&m_msg), name.c_str());
  if (!header) {
    // TODO: use std::optional
    return {};
  }
  return {header->buf, header->len};
}

std::string_view async_request::body() const {
  return {m_msg.body.buf, m_msg.body.len};
}
}  // namespace mg::http
