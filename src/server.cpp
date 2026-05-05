#include "server.h"

#include <functional>
#include <memory>
#include <ranges>

#include "http/endpoint.h"
#include "http/remote_context.h"

namespace mg {
void event_manager_handler(mg_connection* conn, const int ev, void* ev_data) {
  const auto endpoint = static_cast<mg::endpoint*>(conn->fn_data);
  endpoint->handle(conn, ev, ev_data);
}

void event_manager_context_handler(mg_connection* conn, const int ev,
                                   void* ev_data) {
  // TODO: make `remote_context` an interface
  const auto context = static_cast<http::remote_context*>(conn->fn_data);
  context->handle(conn, ev, ev_data);
}

void event_manager_logger(const char ch, void* param) {
  auto* server = static_cast<mg::server*>(param);
  auto& [buffer, offset, listener] = server->m_logger;
  buffer[offset++] = ch;
  if (ch == '\n' || offset >= buffer.size()) {
    listener->invoke(std::string_view(buffer.data(), offset));
    offset = 0;
  }
}

server::server() { setup(); }

server::server(const int level) {
  mg_log_set(level);
  setup();
}

bool server::is_async() const { return m_wakeup; }

std::shared_ptr<http::endpoint> server::listen_http(const std::string& host) {
  auto endpoint = std::make_shared<http::endpoint>(m_mgr, host);
  const auto conn = mg_http_listen(m_mgr.get(), host.c_str(),
                                   &event_manager_handler, endpoint.get());
  if (!conn) {
    return nullptr;
  }

  endpoint->setup(conn);
  m_endpoints.push_back(endpoint);
  return endpoint;
}

void server::poll(const int ms) const { mg_mgr_poll(m_mgr.get(), ms); }

void server::register_logger(const int level, const size_t size) {
  if (!m_logger.listener) {
    return;
  }

  m_logger.buffer.resize(size);
  mg_log_set_fn(&event_manager_logger, this);
  mg_log_set(level);
}

void server::setup() {
  m_mgr = std::shared_ptr<mg_mgr>(
      [] {
        auto* mgr = new mg_mgr;
        mg_mgr_init(mgr);
        return mgr;
      }(),
      [](mg_mgr* mgr) {
        mg_mgr_free(mgr);
        delete mgr;

#if defined(MBEDTLS_VERSION_NUMBER) && MBEDTLS_VERSION_NUMBER >= 0x03000000 && \
    defined(MBEDTLS_PSA_CRYPTO_C)
        mbedtls_psa_crypto_free();
#endif
      });
  m_wakeup = mg_wakeup_init(m_mgr.get());
  if (!m_wakeup) {
    MG_ERROR(("errmsg=\"Failed to initialize asynchronous mode\""));
  }
}
}  // namespace mg
