#include "request.h"

namespace mg::http {
request::request(mg_http_message* msg) : m_msg(msg) {}

request::request(mg_http_message* msg, std::vector<mg_str> groups)
    : m_msg(msg), m_groups(std::move(groups)) {}

std::string_view request::method() const {
  return {m_msg->method.buf, m_msg->method.len};
}

std::string_view request::uri() const {
  return {m_msg->uri.buf, m_msg->uri.len};
}

std::string_view request::get_param(const size_t index) const {
  if (index >= m_groups.size()) {
    // TODO: use std::optional
    return {};
  }
  return {m_groups[index].buf, m_groups[index].len};
}

std::string_view request::query() const {
  return {m_msg->query.buf, m_msg->query.len};
}

std::string_view request::version() const {
  return {m_msg->proto.buf, m_msg->proto.len};
}

std::string_view request::get_header(const std::string& name) const {
  const auto header = mg_http_get_header(m_msg, name.c_str());
  if (!header) {
    // TODO: use std::optional
    return {};
  }
  return {header->buf, header->len};
}

std::string_view request::body() const {
  return {m_msg->body.buf, m_msg->body.len};
}

std::unique_ptr<async_request> request::to_async() const {
  return std::make_unique<async_request>(m_msg, m_groups);
}
}  // namespace mg::http
