#include "web_endpoint.h"

#include <ranges>

#include "remote_context.h"
#include "server.h"

namespace mg {
web_endpoint::web_endpoint(std::weak_ptr<mg_mgr> mgr, std::string host)
    : endpoint(std::move(mgr), std::move(host)) {}

web_endpoint::~web_endpoint() {
  if (m_tls_alloc) {
    mg_free(m_tls_ca.buf);
    mg_free(m_tls_cert.buf);
    mg_free(m_tls_key.buf);
    m_tls_alloc = false;
  }
}

bool web_endpoint::is_mtls() const { return m_tls_ca.len > 0; }

void web_endpoint::use_tls(const std::string& cert, const std::string& key) {
  const mg_str tls_cert = mg_str_n(cert.c_str(), cert.size());
  const mg_str tls_key = mg_str_n(key.c_str(), key.size());
  m_tls_cert = mg_strdup(tls_cert);
  m_tls_key = mg_strdup(tls_key);
  m_tls_alloc = true;
}

void web_endpoint::use_tls(const std::string& ca, const std::string& cert,
                           const std::string& key) {
  const mg_str tls_ca = mg_str_n(ca.c_str(), ca.size());
  const mg_str tls_cert = mg_str_n(cert.c_str(), cert.size());
  const mg_str tls_key = mg_str_n(key.c_str(), key.size());
  m_tls_ca = mg_strdup(tls_ca);
  m_tls_cert = mg_strdup(tls_cert);
  m_tls_key = mg_strdup(tls_key);
  m_tls_alloc = true;
}

void web_endpoint::use_tls(const std::string_view cert,
                           const std::string_view key) {
  m_tls_cert = mg_str_n(cert.data(), cert.size());
  m_tls_key = mg_str_n(key.data(), key.size());
}

void web_endpoint::use_tls(const std::string_view ca,
                           const std::string_view cert,
                           const std::string_view key) {
  m_tls_ca = mg_str_n(ca.data(), ca.size());
  m_tls_cert = mg_str_n(cert.data(), cert.size());
  m_tls_key = mg_str_n(key.data(), key.size());
}

void web_endpoint::handle(mg_connection* conn, const int ev, void* ev_data) {
  if (ev == MG_EV_ACCEPT) {
    handle_secure(conn);
    promote_context(conn);
    return;
  }

  switch (ev) {
    case MG_EV_POLL:
      handle_poll(conn);
      break;
    case MG_EV_TLS_HS:
      setup_context(conn);
      break;
    case MG_EV_HTTP_MSG:
      handle_http_message(conn, static_cast<mg_http_message*>(ev_data));
      break;
    case MG_EV_WAKEUP:
      handle_wakeup(conn, static_cast<mg_str*>(ev_data));
      break;
    case MG_EV_CLOSE:
      handle_close(conn);
      break;
    default:
      break;
  }
}

void web_endpoint::handle_poll(mg_connection* conn) {
  const auto it_conn = m_pending.find(conn->id);
  if (it_conn == m_pending.end()) {
    return;
  }

  auto& responses = it_conn->second;
  for (auto it = responses.begin(); it != responses.end();) {
    if (it->second->get_state() == http::async_response::state::failed) {
      mg_http_reply(conn, http::internal_server_error::k_code,
                    http::internal_server_error::k_header,
                    http::internal_server_error::k_body);
      it = responses.erase(it);
    } else {
      ++it;
    }
  }
}

void web_endpoint::handle_secure(mg_connection* conn) const {
  if (!conn->is_tls || m_tls_cert.len == 0 || m_tls_key.len == 0) {
    return;
  }

  const mg_tls_opts opts{.ca = m_tls_ca, .cert = m_tls_cert, .key = m_tls_key};
  mg_tls_init(conn, &opts);
}

void web_endpoint::handle_http_message(mg_connection* conn,
                                       mg_http_message* msg) {
  const auto* context = static_cast<remote_context*>(conn->fn_data);

  for (const auto& [path, listener] : m_listeners) {
    const mg_str str = mg_str_n(path.c_str(), path.size());
    std::vector<mg_str> groups{listener->get_groups() + 1};

    if (mg_match(msg->uri, str, groups.data())) {
      const http::request request(msg, *context, std::move(groups));
      if (listener->is_async()) {
        const auto id = m_counter++;
        const auto res =
            std::make_shared<http::async_response>(conn->id, m_mgr, id);
        m_pending[conn->id][id] = res;
        listener->invoke(request, res);
      } else {
        const auto res = std::make_shared<http::response>(conn);
        listener->invoke(request, res);
      }
      return;
    }
  }

  if (m_fallback) {
    const http::request request(msg, *context);
    if (m_fallback->is_async()) {
      const auto id = m_counter++;
      const auto res =
          std::make_shared<http::async_response>(conn->id, m_mgr, id);
      m_pending[conn->id][id] = res;
      m_fallback->invoke(request, res);
    } else {
      const auto res = std::make_shared<http::response>(conn);
      m_fallback->invoke(request, res);
    }
    return;
  }

  mg_http_reply(conn, http::not_found::k_code, http::not_found::k_header,
                http::not_found::k_body);
}

void web_endpoint::handle_wakeup(mg_connection* conn, const mg_str* data) {
  const auto it_conn = m_pending.find(conn->id);
  if (it_conn == m_pending.end()) {
    MG_ERROR(("HTTP connection %d not found", conn->id));
    mg_http_reply(conn, http::internal_server_error::k_code,
                  http::internal_server_error::k_header,
                  http::internal_server_error::k_body);
    return;
  }

  const auto id = *reinterpret_cast<size_t*>(data->buf);
  auto& responses = it_conn->second;
  const auto it = responses.find(id);
  if (it == responses.end()) {
    MG_ERROR(("HTTP response %zu not found for connection %d", id, conn->id));
    mg_http_reply(conn, http::internal_server_error::k_code,
                  http::internal_server_error::k_header,
                  http::internal_server_error::k_body);
    return;
  }

  const auto& response = it->second;
  const auto& [code, headers, body] = response->get_payload();
  mg_http_reply(conn, code, headers.c_str(), "%s\n", body.c_str());
  response->mark_completed();

  responses.erase(it);
}

void web_endpoint::handle_close(const mg_connection* conn) {
  const auto dispose = [conn] {
    const auto* context = static_cast<remote_context*>(conn->fn_data);
    delete context;
  };

  const auto& it = m_pending.find(conn->id);
  if (it == m_pending.end()) {
    dispose();
    return;
  }

  for (const auto& response : it->second | std::views::values) {
    response->mark_failed();
  }
  m_pending.erase(it);

  dispose();
}

void web_endpoint::promote_context(mg_connection* conn) {
  auto* context = new remote_context(this, conn);
  conn->fn = &event_manager_context_handler;
  conn->fn_data = context;
}

void web_endpoint::setup_context(const mg_connection* conn) {
  auto* context = static_cast<remote_context*>(conn->fn_data);
  context->setup(conn);
}

size_t web_endpoint::count_groups(const std::string_view path) {
  size_t groups = 0;
  for (const auto& c : path) {
    if (c == '?' || c == '*' || c == '#') {
      groups++;
    }
  }
  return groups;
}
}  // namespace mg