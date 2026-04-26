#include "server.h"

#include <functional>
#include <memory>

#include "event_listener.h"
#include "request.h"

namespace mg {
void event_manager_handler(mg_connection* conn, const int ev, void* ev_data) {
  const auto evt_mgr = static_cast<server*>(conn->fn_data);
  evt_mgr->handle(conn, ev, ev_data);
}

server::server() {
  mg_mgr_init(&m_mgr);

  // TODO: only call when at least one async handler is registered?
  mg_wakeup_init(&m_mgr);
}

server::~server() { mg_mgr_free(&m_mgr); }

bool server::listen(const std::string& host) {
  return mg_http_listen(&m_mgr, host.c_str(), &event_manager_handler, this) !=
         nullptr;
}

bool server::listen(const std::string& protocol, const std::string& interface,
                    const uint16_t port) {
  const auto host = protocol + "://" + interface + ":" + std::to_string(port);
  return listen(host);
}

void server::poll(const int ms) { mg_mgr_poll(&m_mgr, ms); }

void server::handle(mg_connection* conn, const int ev, void* ev_data) {
  if (ev == MG_EV_HTTP_MSG) {
    handle_http(conn, static_cast<mg_http_message*>(ev_data));
  } else if (ev == MG_EV_WAKEUP) {
    handle_http_wakeup(conn, static_cast<mg_str*>(ev_data));
  }
}

void server::handle_http(mg_connection* conn, mg_http_message* msg) {
  for (const auto& [path, listener] : m_http_listeners) {
    const mg_str str = mg_str_n(path.c_str(), path.size());
    std::vector<mg_str> groups{listener->get_groups()};

    if (mg_match(msg->uri, str, groups.data())) {
      const http::request request(msg, std::move(groups));
      if (listener->is_async()) {
        const auto res =
            std::make_shared<http::async_response>(conn->id, &m_mgr);
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
      const auto res = std::make_shared<http::async_response>(conn->id, &m_mgr);
      m_http_fallback->invoke(request, res);
    } else {
      const auto res = std::make_shared<http::response>(conn);
      m_http_fallback->invoke(request, res);
    }
    return;
  }

  const auto res = std::make_shared<http::response>(conn);
  res->send(404, "Not Found");
}

void server::handle_http_wakeup(mg_connection* conn, const mg_str* data) {
  if (!data || !data->buf || data->len != sizeof(http::async_response::payload*)) {
    mg_http_reply(conn, 500, nullptr, "%s\n", "Internal Server Error");
    return;
  }

  // FIXME: memory leak when remote close connection
  const auto payload = *reinterpret_cast<http::async_response::payload**>(data->buf);
  mg_http_reply(conn, payload->code, payload->headers.c_str(), "%s\n", payload->body.c_str());
  delete payload;
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
