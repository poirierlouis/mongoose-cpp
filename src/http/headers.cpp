#include "mgxx/http/headers.hpp"

namespace mgxx::http {
size_t headers::size() const { return m_headers.size(); }

bool headers::has(const std::string_view name) const {
  return m_headers.contains(name);
}

std::optional<std::string> headers::get(const std::string_view name) const {
  const auto it = m_headers.find(name);
  if (it == m_headers.end()) {
    return std::nullopt;
  }

  return it->second;
}

std::optional<std::string_view> headers::get_view(
    const std::string_view name) const {
  const auto it = m_headers.find(name);
  if (it == m_headers.end()) {
    return std::nullopt;
  }

  return it->second;
}

void headers::set(std::string name, std::string value) {
  m_headers[std::move(name)] = std::move(value);
}

void headers::remove(const std::string_view name) {
  if (const auto it = m_headers.find(name); it != m_headers.end()) {
    m_headers.erase(it);
  }
}

void headers::clear() { m_headers.clear(); }

std::string headers::format(const size_t capacity) const {
  std::string headers;
  if (capacity > 0) {
    headers.reserve(capacity);
  }
  for (const auto& [name, value] : m_headers) {
    headers.append(name);
    headers.append(": ");
    headers.append(value);
    headers.append("\r\n");
  }
  if (headers.empty()) {
    headers.append("Content-Type: text/plain\r\n");
  }
  return headers;
}
}  // namespace mgxx::http