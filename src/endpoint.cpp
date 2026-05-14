#include "mgxx/endpoint.hpp"

namespace mgxx {
endpoint::endpoint(std::weak_ptr<mg_mgr> mgr, std::string host)
    : m_host(std::move(host)), m_conn(nullptr), m_mgr(std::move(mgr)) {}

std::string_view endpoint::get_host() const { return m_host; }

unsigned long endpoint::get_conn_id() const { return m_conn->id; }

void endpoint::setup(mg_connection* conn) {
  m_conn = conn;
  m_conn->data[0] = 'E';
}
}  // namespace mgxx