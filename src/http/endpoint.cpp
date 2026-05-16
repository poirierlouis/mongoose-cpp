#include "mgxx/http/endpoint.hpp"

#include <ranges>
#include <utility>

#include "mgxx/http/async_stream.hpp"
#include "mgxx/internal/remote_context.hpp"
#include "mgxx/server.hpp"

namespace mgxx::http {
endpoint::endpoint(std::weak_ptr<mg_mgr> mgr, std::string host)
    : mgxx::endpoint(std::move(mgr), std::move(host)) {}

endpoint::~endpoint() {
  if (m_tls_alloc) {
    if (is_mtls()) {
      mg_free(m_tls_ca.buf);
    }
    mg_free(m_tls_cert.buf);
    mg_free(m_tls_key.buf);
    m_tls_alloc = false;
  }
}

bool endpoint::is_mtls() const { return m_tls_ca.len > 0; }

void endpoint::use_tls(const std::string& cert, const std::string& key) {
  const mg_str tls_cert = mg_str_n(cert.c_str(), cert.size());
  const mg_str tls_key = mg_str_n(key.c_str(), key.size());
  m_tls_cert = mg_strdup(tls_cert);
  m_tls_key = mg_strdup(tls_key);
  m_tls_alloc = true;
}

void endpoint::use_tls(const std::string& ca, const std::string& cert,
                       const std::string& key) {
  const mg_str tls_ca = mg_str_n(ca.c_str(), ca.size());
  const mg_str tls_cert = mg_str_n(cert.c_str(), cert.size());
  const mg_str tls_key = mg_str_n(key.c_str(), key.size());
  m_tls_ca = mg_strdup(tls_ca);
  m_tls_cert = mg_strdup(tls_cert);
  m_tls_key = mg_strdup(tls_key);
  m_tls_alloc = true;
}

void endpoint::use_tls(const std::string_view cert,
                       const std::string_view key) {
  m_tls_cert = mg_str_n(cert.data(), cert.size());
  m_tls_key = mg_str_n(key.data(), key.size());
}

void endpoint::use_tls(const std::string_view ca, const std::string_view cert,
                       const std::string_view key) {
  m_tls_ca = mg_str_n(ca.data(), ca.size());
  m_tls_cert = mg_str_n(cert.data(), cert.size());
  m_tls_key = mg_str_n(key.data(), key.size());
}

endpoint& endpoint::on_asset(const std::string_view path, std::string asset) {
  return on_asset(path, std::move(asset), {}, headers{});
}

endpoint& endpoint::on_asset(const std::string_view path, std::string asset,
                             std::string mime_type) {
  return on_asset(path, std::move(asset), std::move(mime_type), headers{});
}

endpoint& endpoint::on_asset(const std::string_view path, std::string asset,
                             std::string mime_type, const headers& headers) {
  auto& resource = m_assets[path];
  resource.path = std::move(asset);
  resource.mime_type = std::move(mime_type);
  resource.headers = headers.format(true);
  return *this;
}

endpoint& endpoint::on_assets() { return *this; }

void endpoint::handle(mg_connection* conn, const int ev, void* ev_data) {
  if (ev == MG_EV_ACCEPT) {
    handle_secure(conn);
    promote_context(conn);
    return;
  }

  switch (ev) {
    case MG_EV_TLS_HS:
      setup_context(conn);
      break;
    case MG_EV_HTTP_MSG:
      handle_http_message(conn, static_cast<mg_http_message*>(ev_data));
      break;
    case MG_EV_WAKEUP:
      handle_wakeup(conn);
      break;
    case MG_EV_WRITE:
      handle_write(conn);
      break;
    case MG_EV_CLOSE:
      handle_close(conn);
      break;
    default:
      break;
  }
}

void endpoint::handle_secure(mg_connection* conn) const {
  if (!conn->is_tls || m_tls_cert.len == 0 || m_tls_key.len == 0) {
    return;
  }

  mg_tls_opts opts{};
  if (is_mtls()) {
    opts.ca = m_tls_ca;
  }
  opts.cert = m_tls_cert;
  opts.key = m_tls_key;
  mg_tls_init(conn, &opts);
}

void endpoint::handle_http_message(mg_connection* conn, mg_http_message* msg) {
  const auto* context = static_cast<remote_context*>(conn->fn_data);

  for (const auto& [path, listener] : m_listeners) {
    const mg_str str = mg_str_n(path.c_str(), path.size());
    std::vector<mg_str> groups{listener->get_groups() + 1};

    if (mg_match(msg->uri, str, groups.data())) {
      const request request(msg, *context, std::move(groups));
      if (listener->is_async()) {
        const auto res = std::make_shared<async_response>(
            get_conn_id(), conn->id, &m_responses, &m_streams, m_mgr);
        listener->invoke(request, res);
      } else {
        response res(conn);
        listener->invoke(request, res);
      }
      return;
    }
  }

  for (const auto& [path, asset] : m_assets) {
    const mg_str str = mg_str_n(path.data(), path.size());
    if (mg_match(msg->uri, str, nullptr)) {
      mg_http_serve_opts opts = {
          .root_dir = nullptr,
          .extra_headers =
              asset.headers.empty() ? nullptr : asset.headers.c_str(),
          .mime_types =
              asset.mime_type.empty() ? nullptr : asset.mime_type.data(),
          .fs = nullptr};
      if (asset.is_dir) {
        const auto root_dir =
            std::format("{}={}", path.substr(0, path.size() - 2), asset.path);
        opts.root_dir = root_dir.c_str();
        mg_http_serve_dir(conn, msg, &opts);
      } else {
        mg_http_serve_file(conn, msg, asset.path.data(), &opts);
      }
      return;
    }
  }

  if (m_fallback) {
    const request request(msg, *context);
    if (m_fallback->is_async()) {
      const auto res = std::make_shared<async_response>(
          get_conn_id(), conn->id, &m_responses, &m_streams, m_mgr);
      m_fallback->invoke(request, res);
    } else {
      response res(conn);
      m_fallback->invoke(request, res);
    }
    return;
  }

  mg_http_reply(conn, not_found::k_code, not_found::k_header,
                not_found::k_body);
}

void endpoint::handle_wakeup(const mg_connection* conn) {
  if (conn->is_listening) {
    handle_responses();
    handle_streams();
  }
}

void endpoint::handle_write(mg_connection* conn) {
  auto* context = static_cast<remote_context*>(conn->fn_data);
  context->pump_stream(conn);
  context->pump_async_stream(conn);
}

void endpoint::handle_close(mg_connection* conn) {
  if (conn->data[0] == 'R') {
    auto* context = static_cast<remote_context*>(conn->fn_data);
    context->close();
    delete context;

    conn->fn_data = nullptr;
    conn->data[0] = '\0';
  }
}

void endpoint::handle_responses() {
  std::optional<internal::payload> payload;

  while ((payload = m_responses.pop()).has_value()) {
    mg_connection* conn = find_conn_by_id(payload->conn);
    if (conn == nullptr) {
      continue;
    }

    mg_http_reply(conn, payload->code, payload->headers.c_str(), "%.*s",
                  payload->body.size(), payload->body.data());
  }
}

void endpoint::handle_streams() {
  std::optional<internal::payload_stream> payload;

  while ((payload = m_streams.pop()).has_value()) {
    mg_connection* conn = find_conn_by_id(payload->conn);
    if (conn == nullptr) {
      continue;
    }

    auto* context = static_cast<remote_context*>(conn->fn_data);
    const auto& chunk = payload->data;
    if (payload->state == internal::payload_stream::state::preamble) {
      const auto stream = static_cast<async_stream*>(payload->stream);
      context->set_async_stream(stream->weak_from_this());
      mg_send(conn, chunk.data(), chunk.size());
      continue;
    }

    mg_http_write_chunk(conn, chunk.data(), chunk.size());
    context->pump_async_stream(conn);
    if (chunk.empty()) {
      context->close_async_stream();
    }
  }
}

void endpoint::promote_context(mg_connection* conn) {
  auto* context = new remote_context(this, conn);
  conn->fn = &event_manager_context_handler;
  conn->fn_data = context;
  conn->data[0] = 'R';
}

void endpoint::setup_context(const mg_connection* conn) {
  auto* context = static_cast<remote_context*>(conn->fn_data);
  context->setup(conn);
}

mg_connection* endpoint::find_conn_by_id(const unsigned long id) const {
  const auto mgr = m_mgr.lock();
  if (!mgr) {
    return nullptr;
  }

  for (auto conn = mgr->conns; conn != nullptr; conn = conn->next) {
    if (conn->id == id) {
      return conn;
    }
  }
  return nullptr;
}

size_t endpoint::count_groups(const std::string_view path) {
  size_t groups = 0;
  for (const auto& c : path) {
    if (c == '?' || c == '*' || c == '#') {
      groups++;
    }
  }
  return groups;
}
}  // namespace mgxx::http