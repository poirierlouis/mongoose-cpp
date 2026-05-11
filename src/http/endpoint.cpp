#include "mgxx/http/endpoint.hpp"

#include <ranges>
#include <utility>

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

void endpoint::handle_poll(mg_connection* conn) {
  const auto it_conn = m_pending_responses.find(conn->id);
  if (it_conn == m_pending_responses.end()) {
    return;
  }

  auto& responses = it_conn->second;
  for (auto it = responses.begin(); it != responses.end();) {
    if (it->second->get_state() == async_response::state::failed) {
      mg_http_reply(conn, internal_server_error::k_code,
                    internal_server_error::k_header,
                    internal_server_error::k_body);
      it = responses.erase(it);
    } else {
      ++it;
    }
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
        const auto id = m_request_counter++;
        const auto res = std::make_shared<async_response>(conn->id, m_mgr, id);
        m_pending_responses[conn->id][id] = res;
        listener->invoke(request, res);
      } else {
        const auto res = std::make_shared<response>(conn);
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
      const auto id = m_request_counter++;
      const auto res = std::make_shared<async_response>(conn->id, m_mgr, id);
      m_pending_responses[conn->id][id] = res;
      m_fallback->invoke(request, res);
    } else {
      const auto res = std::make_shared<response>(conn);
      m_fallback->invoke(request, res);
    }
    return;
  }

  mg_http_reply(conn, not_found::k_code, not_found::k_header,
                not_found::k_body);
}

void endpoint::handle_wakeup(mg_connection* conn, const mg_str* data) {
  if (handle_stream(conn, data)) {
    return;
  }

  if (handle_response(conn, data)) {
    return;
  }

  MG_ERROR(("errmsg=\"HTTP connection %d not found\"", conn->id));
  mg_http_reply(conn, internal_server_error::k_code,
                internal_server_error::k_header, internal_server_error::k_body);
}

void endpoint::handle_write(mg_connection* conn) {
  auto* context = static_cast<remote_context*>(conn->fn_data);
  context->pump_stream(conn);

  for (const auto& streams : m_pending_streams | std::views::values) {
    for (const auto& stream : streams | std::views::values) {
      context->pump_async_stream(conn, stream);
    }
  }
}

void endpoint::handle_close(const mg_connection* conn) {
  const auto dispose = [conn] {
    const auto* context = static_cast<remote_context*>(conn->fn_data);
    delete context;
  };

  if (const auto& it = m_pending_responses.find(conn->id);
      it != m_pending_responses.end()) {
    for (const auto& response : it->second | std::views::values) {
      response->mark_failed();
    }
    m_pending_responses.erase(it);
  }

  if (const auto& it = m_pending_streams.find(conn->id);
      it != m_pending_streams.end()) {
    for (const auto& stream : it->second | std::views::values) {
      stream->mark_failed();
    }
    m_pending_streams.erase(it);
  }

  dispose();
}

bool endpoint::handle_response(mg_connection* conn, const mg_str* data) {
  const auto it_conn = m_pending_responses.find(conn->id);
  if (it_conn == m_pending_responses.end()) {
    MG_ERROR(("errmsg=\"HTTP connection %d not found\"", conn->id));
    mg_http_reply(conn, internal_server_error::k_code,
                  internal_server_error::k_header,
                  internal_server_error::k_body);
    return false;
  }

  const auto id = *reinterpret_cast<size_t*>(data->buf);
  auto& responses = it_conn->second;
  const auto it = responses.find(id);
  if (it == responses.end()) {
    MG_ERROR(("errmsg=\"HTTP response %d not found for connection %d\"", id,
              conn->id));
    mg_http_reply(conn, internal_server_error::k_code,
                  internal_server_error::k_header,
                  internal_server_error::k_body);
    return false;
  }

  const auto& response = it->second;
  if (response->get_state() == async_response::state::streaming) {
    m_pending_streams[conn->id][id] = response->get_stream();
    responses.erase(it);
    return handle_stream(conn, data);
  }

  const auto& [code, headers, body] = response->get_payload();
  mg_http_reply(conn, code, headers.c_str(), "%.*s", body.size(), body.data());
  response->mark_completed();

  responses.erase(it);
  return true;
}

bool endpoint::handle_stream(mg_connection* conn, const mg_str* data) {
  const auto it_conn = m_pending_streams.find(conn->id);
  if (it_conn == m_pending_streams.end()) {
    return false;
  }

  const auto id = *reinterpret_cast<size_t*>(data->buf);
  auto& streams = it_conn->second;
  const auto it = streams.find(id);
  if (it == streams.end()) {
    return false;
  }

  const auto& stream = it->second;
  switch (stream->get_state()) {
    case async_stream::state::start: {
      const auto preamble = stream->get_data();
      mg_send(conn, preamble.data(), preamble.size());
      stream->mark_flush();
      return true;
    }
    case async_stream::state::has_data: {
      const auto chunk = stream->get_data();
      mg_http_write_chunk(conn, chunk.data(), chunk.size());
      stream->mark_flush();
      return true;
    }
    case async_stream::state::close: {
      mg_http_write_chunk(conn, "", 0);
      stream->mark_completed();
      streams.erase(it);
      if (streams.empty()) {
        m_pending_streams.erase(it_conn);
      }
      return true;
    }
    default:
      // NOTE: wait until mongoose flushes the internal IO buffer to the
      //       network. This provides backpressure, preventing the producer from
      //       overfilling RAM.
      return true;
  }
}

void endpoint::promote_context(mg_connection* conn) {
  auto* context = new remote_context(this, conn);
  conn->fn = &event_manager_context_handler;
  conn->fn_data = context;
}

void endpoint::setup_context(const mg_connection* conn) {
  auto* context = static_cast<remote_context*>(conn->fn_data);
  context->setup(conn);
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