#include "server.h"

#include <functional>
#include <memory>
#include <ranges>

#include "event_listener.h"
#include "request.h"

namespace mg {
void event_manager_handler(mg_connection* conn, const int ev, void* ev_data) {
  const auto evt_mgr = static_cast<server*>(conn->fn_data);
  evt_mgr->handle(conn, ev, ev_data);
}

server::server()
    : m_mgr(
          [] {
            auto* mgr = new mg_mgr;
            mg_mgr_init(mgr);
            return mgr;
          }(),
          [](mg_mgr* mgr) {
            mg_mgr_free(mgr);
            delete mgr;
          }) {
  m_wakeup = mg_wakeup_init(m_mgr.get());
  if (!m_wakeup) {
    MG_ERROR(("Failed to initialize asynchronous mode"));
  }
}

bool server::listen(const std::string& host) {
  return mg_http_listen(m_mgr.get(), host.c_str(), &event_manager_handler,
                        this) != nullptr;
}

bool server::listen(const std::string& protocol, const std::string& interface,
                    const uint16_t port) {
  const auto host = protocol + "://" + interface + ":" + std::to_string(port);
  return listen(host);
}

bool server::is_async() const { return m_wakeup; }

void server::poll(const int ms) const { mg_mgr_poll(m_mgr.get(), ms); }

void server::handle(mg_connection* conn, const int ev, void* ev_data) {
  if (ev == MG_EV_POLL) {
    handle_http_poll(conn);
  } else if (ev == MG_EV_HTTP_MSG) {
    handle_http(conn, static_cast<mg_http_message*>(ev_data));
  } else if (ev == MG_EV_WAKEUP) {
    handle_http_wakeup(conn, static_cast<mg_str*>(ev_data));
  } else if (ev == MG_EV_CLOSE) {
    handle_http_close(conn);
  }
}

void server::handle_http_poll(mg_connection* conn) {
  const auto it_conn = m_http_pending.find(conn->id);
  if (it_conn == m_http_pending.end()) {
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

void server::handle_http(mg_connection* conn, mg_http_message* msg) {
  for (const auto& [path, listener] : m_http_listeners) {
    const mg_str str = mg_str_n(path.c_str(), path.size());
    std::vector<mg_str> groups{listener->get_groups() + 1};

    if (mg_match(msg->uri, str, groups.data())) {
      const http::request request(msg, std::move(groups));
      if (listener->is_async()) {
        const auto id = m_counter++;
        const auto res =
            std::make_shared<http::async_response>(conn->id, m_mgr, id);
        m_http_pending[conn->id][id] = res;
        listener->invoke(request, res);
      } else {
        const auto res = std::make_shared<http::response>(conn);
        listener->invoke(request, res);
      }
      return;
    }
  }

  if (m_http_fallback) {
    const http::request request(msg);
    if (m_http_fallback->is_async()) {
      const auto id = m_counter++;
      const auto res =
          std::make_shared<http::async_response>(conn->id, m_mgr, id);
      m_http_pending[conn->id][id] = res;
      m_http_fallback->invoke(request, res);
    } else {
      const auto res = std::make_shared<http::response>(conn);
      m_http_fallback->invoke(request, res);
    }
    return;
  }

  mg_http_reply(conn, http::not_found::k_code, http::not_found::k_header,
                http::not_found::k_body);
}

void server::handle_http_wakeup(mg_connection* conn, const mg_str* data) {
  const auto it_conn = m_http_pending.find(conn->id);
  if (it_conn == m_http_pending.end()) {
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

void server::handle_http_close(const mg_connection* conn) {
  const auto& it = m_http_pending.find(conn->id);
  if (it == m_http_pending.end()) {
    return;
  }

  for (const auto& response : it->second | std::views::values) {
    response->mark_failed();
  }
  m_http_pending.erase(it);
}

size_t server::count_groups(const std::string_view path) {
  size_t groups = 0;
  for (const auto& c : path) {
    if (c == '?' || c == '*' || c == '#') {
      groups++;
    }
  }
  return groups;
}
}  // namespace mg
