#ifndef MGXX_TLS_HPP
#define MGXX_TLS_HPP

#include <string>
#include <string_view>

namespace mgxx {

namespace http {
class remote_context;
}

class tls_cert_info {
  std::string buffer;

  std::string_view subject_name;
  std::string_view issuer_name;
  std::string_view serial_number;

  std::string_view not_before;
  std::string_view not_after;

  std::string_view fingerprint;

  friend class mgxx::http::remote_context;

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
}  // namespace mgxx

#endif  // MGXX_TLS_HPP
