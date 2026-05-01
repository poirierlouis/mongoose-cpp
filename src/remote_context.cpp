#include "remote_context.h"

namespace mg {
remote_context::remote_context(endpoint* endpoint, const mg_connection* conn)
    : m_endpoint(endpoint) {
  // TODO: setup remote IP address
}

std::weak_ptr<tls_cert_info> remote_context::get_tls_cert_info() const {
  return m_tls_cert;
}

void remote_context::setup(const mg_connection* conn) {
  if (!conn->tls) {
    return;
  }

#ifdef MGPP_OPENSSL
  const auto* tls = static_cast<mg_tls*>(conn->tls);
  if (!tls->ssl) {
    MG_ERROR(
        ("errmsg=\"Inconsistent state while looking for OpenSSL context\""));
    return;
  }

  const X509* cert = SSL_get0_peer_certificate(tls->ssl);
  if (!cert) {
    return;
  }

  BIO* bio = BIO_new(BIO_s_mem());
  if (!bio) {
    MG_ERROR(("errmsg=\"Failed to allocate BIO\""));
    return;
  }

  const auto get_x509_name = [bio](const X509_NAME* name) -> std::string {
    BIO_reset(bio);

    X509_NAME_print_ex(bio, name, 0, XN_FLAG_RFC2253);
    char* buffer = nullptr;
    const auto length = static_cast<size_t>(BIO_get_mem_data(bio, &buffer));
    return {buffer, length};
  };
  const auto get_x509_time = [bio](const ASN1_TIME* time) -> std::string {
    BIO_reset(bio);

    ASN1_TIME_print(bio, time);
    char* buffer = nullptr;
    const auto length = static_cast<size_t>(BIO_get_mem_data(bio, &buffer));
    return {buffer, length};
  };

  const std::string subject = get_x509_name(X509_get_subject_name(cert));
  const std::string issuer = get_x509_name(X509_get_issuer_name(cert));
  const std::string before = get_x509_time(X509_get0_notBefore(cert));
  const std::string after = get_x509_time(X509_get0_notAfter(cert));

  // Serial Number
  const auto* asn1 = X509_get0_serialNumber(cert);
  auto* bn = ASN1_INTEGER_to_BN(asn1, nullptr);
  auto* hex = BN_bn2hex(bn);
  const std::string serial = hex;
  OPENSSL_free(hex);
  BN_free(bn);

  // Fingerprint
  unsigned char fp[EVP_MAX_MD_SIZE];
  unsigned int fp_length = 0;
  X509_digest(cert, EVP_sha256(), fp, &fp_length);
  std::string fingerprint;
  for (unsigned int i = 0; i < fp_length; i++) {
    fingerprint += std::format("{:02X}", fp[i]);
    if (i < fp_length - 1) {
      fingerprint += ":";
    }
  }

  BIO_free(bio);

  m_tls_cert = std::make_shared<tls_cert_info>();
  m_tls_cert->buffer.clear();
  m_tls_cert->buffer.reserve(subject.size() + issuer.size() + serial.size() +
                             before.size() + after.size() + fingerprint.size());

  m_tls_cert->buffer.append(subject);
  m_tls_cert->buffer.append(issuer);
  m_tls_cert->buffer.append(serial);
  m_tls_cert->buffer.append(before);
  m_tls_cert->buffer.append(after);
  m_tls_cert->buffer.append(fingerprint);

  const char* base = m_tls_cert->buffer.data();
  m_tls_cert->subject_name = {base, subject.size()};
  m_tls_cert->issuer_name = {base + subject.size(), issuer.size()};
  m_tls_cert->serial_number = {base + subject.size() + issuer.size(),
                               serial.size()};
  m_tls_cert->not_before = {
      base + subject.size() + issuer.size() + serial.size(), before.size()};
  m_tls_cert->not_after = {
      base + subject.size() + issuer.size() + serial.size() + before.size(),
      after.size()};
  m_tls_cert->fingerprint = {base + subject.size() + issuer.size() +
                                 serial.size() + before.size() + after.size(),
                             fingerprint.size()};
#endif
}

void remote_context::handle(mg_connection* conn, const int ev,
                            void* ev_data) const {
  m_endpoint->handle(conn, ev, ev_data);
}
}  // namespace mg