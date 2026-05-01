#ifndef MONGOOSE_CPP_REMOTE_CONTEXT_H
#define MONGOOSE_CPP_REMOTE_CONTEXT_H

#include <mongoose.h>

#include <chrono>
#include <string>

#include "endpoint.h"

namespace mg {
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
  endpoint* m_endpoint;
  std::shared_ptr<tls_cert_info> m_tls_cert;

 public:
  explicit remote_context(endpoint* endpoint, const mg_connection* conn);
  ~remote_context() = default;

  [[nodiscard]] std::weak_ptr<tls_cert_info> get_tls_cert_info() const;

  void setup(const mg_connection* conn);
  void handle(mg_connection* conn, int ev, void* ev_data) const;
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
#endif
}  // namespace mg

#endif  // MONGOOSE_CPP_REMOTE_CONTEXT_H
