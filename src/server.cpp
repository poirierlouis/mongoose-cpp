#include <functional>
#include <memory>

#include "event_listener.h"
#include "request.h"
#include "server.h"

namespace mg {
void event_manager_handler(mg_connection* conn, const int ev, void* ev_data) {
  const auto evt_mgr = static_cast<server*>(conn->fn_data);
  evt_mgr->handle(conn, ev, ev_data);
}

server::server() { mg_mgr_init(&m_mgr); }

server::~server() { mg_mgr_free(&m_mgr); }

bool server::listen(const std::string& host) {
  return mg_http_listen(&m_mgr, host.c_str(), &event_manager_handler, this) != nullptr;
}

bool server::listen(const std::string& protocol,
                           const std::string& interface, const uint16_t port) {
  const auto host = protocol + "://" + interface + ":" + std::to_string(port);
  return listen(host);
}

void server::poll(const int ms) { mg_mgr_poll(&m_mgr, ms); }

void server::handle(mg_connection* conn, const int ev, void* ev_data) {
  if (ev == MG_EV_HTTP_MSG) {
    handle_http(conn, static_cast<mg_http_message*>(ev_data));
  }
}

void server::handle_http(mg_connection* conn, mg_http_message* msg) {
  for (const auto& [path, listener] : m_http_listeners) {
    const mg_str str = mg_str_n(path.c_str(), path.size());
    std::vector<mg_str> groups{listener->get_groups()};

    if (mg_match(msg->uri, str, groups.data())) {
      const http::request request(msg, std::move(groups));
      listener->invoke(request, std::make_shared<http::response>(conn));
      return;
    }
  }

  if (m_http_fallback) {
    const http::request request(msg, std::vector<mg_str>{});
    m_http_fallback->invoke(request, std::make_shared<http::response>(conn));
    return;
  }

  const http::response response(conn);
  response.send(404, "Not Found");
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
