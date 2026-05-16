#ifndef MGXX_HTTP_REMOTE_CONTEXT_HPP
#define MGXX_HTTP_REMOTE_CONTEXT_HPP

#include <mongoose.h>

#include <chrono>
#include <memory>
#include <string>

#include "mgxx/endpoint.hpp"
#include "mgxx/http/internal/common.hpp"
#include "mgxx/tls.hpp"

namespace mgxx::http {
class remote_context;
class async_stream;

class remote_context {
  mgxx::endpoint* m_endpoint;
  std::string m_ip;
  std::shared_ptr<tls_cert_info> m_tls_cert;
  std::unique_ptr<stream_producer> m_stream;
  std::weak_ptr<async_stream> m_async_stream;

 public:
  explicit remote_context(mgxx::endpoint* endpoint, const mg_connection* conn);
  ~remote_context() = default;

  [[nodiscard]] std::string_view get_remote_ip() const;
  [[nodiscard]] std::weak_ptr<tls_cert_info> get_tls_cert_info() const;

  void setup(const mg_connection* conn);
  void handle(mg_connection* conn, int ev, void* ev_data) const;
  void close();

  void set_stream(std::unique_ptr<stream_producer> stream);
  void pump_stream(mg_connection* conn);

  void set_async_stream(std::weak_ptr<async_stream> async_stream);
  void pump_async_stream(const mg_connection* conn) const;
  void close_async_stream();
};

#if MG_TLS == MG_TLS_OPENSSL || MG_TLS == MG_TLS_WOLFSSL
#define MGPP_OPENSSL

// NOTE: copied from mongoose/src/tls_openssl.h
#include <openssl/err.h>
#include <openssl/ssl.h>

struct mg_tls {
  BIO_METHOD* bm;
  SSL_CTX* ctx;
  SSL* ssl;
};
#elif MG_TLS == MG_TLS_MBED
#define MGPP_MBED

// NOTE: copied from mongoose/src/tls_mbed.h
#include <mbedtls/debug.h>
#include <mbedtls/error.h>
#include <mbedtls/net_sockets.h>
#include <mbedtls/ssl.h>
#include <mbedtls/ssl_ticket.h>

struct mg_tls_ctx {
  int dummy;
#ifdef MBEDTLS_SSL_SESSION_TICKETS
  mbedtls_ssl_ticket_context tickets;
#endif
};

struct mg_tls {
  mbedtls_x509_crt ca;      // Parsed CA certificate
  mbedtls_x509_crt cert;    // Parsed certificate
  mbedtls_pk_context pk;    // Private key context
  mbedtls_ssl_context ssl;  // SSL/TLS context
  mbedtls_ssl_config conf;  // SSL-TLS config
#ifdef MBEDTLS_SSL_SESSION_TICKETS
  mbedtls_ssl_ticket_context ticket;  // Session tickets context
#endif
  // https://github.com/Mbed-TLS/mbedtls/blob/3b3c652d/include/mbedtls/ssl.h#L5071C18-L5076C29
  unsigned char* throttled_buf;  // see #3074
  size_t throttled_len;
};
#endif
}  // namespace mgxx::http

#endif  // MGXX_HTTP_REMOTE_CONTEXT_H
