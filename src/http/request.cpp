#include "mgxx/http/request.hpp"

namespace mgxx::http {
request::request(mg_http_message* msg, const remote_context& context)
    : request(msg, context, {}) {}

request::request(mg_http_message* msg, const remote_context& context,
                 std::vector<mg_str> groups)
    : m_msg(msg),
      m_groups(std::move(groups)),
      m_ip(context.get_remote_ip()),
      m_tls_cert_info(context.get_tls_cert_info()) {}

std::string_view request::get_remote_ip() const { return m_ip; }

std::weak_ptr<tls_cert_info> request::get_tls_cert_info() const {
  return m_tls_cert_info;
}

std::string_view request::method() const {
  return {m_msg->method.buf, m_msg->method.len};
}

std::string_view request::uri() const {
  return {m_msg->uri.buf, m_msg->uri.len};
}

std::optional<std::string_view> request::get_param(const size_t index) const {
  if (index >= m_groups.size()) {
    return std::nullopt;
  }
  return std::string_view{m_groups[index].buf, m_groups[index].len};
}

std::string_view request::query() const {
  return {m_msg->query.buf, m_msg->query.len};
}

std::string_view request::version() const {
  return {m_msg->proto.buf, m_msg->proto.len};
}

std::optional<std::string_view> request::get_header(
    const std::string& name) const {
  const auto header = mg_http_get_header(m_msg, name.c_str());
  if (!header) {
    return std::nullopt;
  }
  return std::string_view{header->buf, header->len};
}

std::string_view request::body() const {
  return {m_msg->body.buf, m_msg->body.len};
}

std::unique_ptr<async_request> request::to_async() const {
  return std::make_unique<async_request>(m_msg, m_groups, m_ip,
                                         m_tls_cert_info);
}
}  // namespace mgxx::http
