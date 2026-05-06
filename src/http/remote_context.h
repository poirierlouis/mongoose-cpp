#ifndef MONGOOSE_CPP_HTTP_REMOTE_CONTEXT_H
#define MONGOOSE_CPP_HTTP_REMOTE_CONTEXT_H

#include <mongoose.h>

#include <chrono>
#include <string>

#include "../endpoint.h"
#include "common.h"

namespace mg::http {
class remote_context;

class tls_cert_info {
  std::string buffer;

  std::string_view subject_name;
  std::string_view issuer_name;
  std::string_view serial_number;

  std::string_view not_before;
  std::string_view not_after;

  std::string_view fingerprint;

  friend class remote_context;

 public:
  tls_cert_info() = default;
  ~tls_cert_info() = default;

  tls_cert_info(const tls_cert_info&) = delete;
  tls_cert_info& operator=(const tls_cert_info&) = delete;

  tls_cert_info(tls_cert_info&&) = delete;
  tls_cert_info& operator=(tls_cert_info&&) = delete;

  [[nodiscard]] std::string_view get_subject_name() const {
    return subject_name;
  }
  [[nodiscard]] std::string_view get_issuer_name() const { return issuer_name; }
  [[nodiscard]] std::string_view get_serial_number() const {
    return serial_number;
  }
  [[nodiscard]] std::string_view get_not_before() const { return not_before; }
  [[nodiscard]] std::string_view get_not_after() const { return not_after; }
  [[nodiscard]] std::string_view get_fingerprint() const { return fingerprint; }
};

class remote_context {
  mg::endpoint* m_endpoint;
  std::shared_ptr<tls_cert_info> m_tls_cert;
  std::unique_ptr<stream_producer> m_stream;

 public:
  explicit remote_context(mg::endpoint* endpoint, const mg_connection* conn);
  ~remote_context() = default;

  [[nodiscard]] std::weak_ptr<tls_cert_info> get_tls_cert_info() const;

  void setup(const mg_connection* conn);
  void handle(mg_connection* conn, int ev, void* ev_data) const;

  void set_stream(std::unique_ptr<stream_producer> producer);
  void pump_stream(mg_connection* conn);
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
}  // namespace mg::http

#endif  // MONGOOSE_CPP_HTTP_REMOTE_CONTEXT_H
