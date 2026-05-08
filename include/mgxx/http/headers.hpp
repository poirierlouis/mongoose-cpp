#ifndef MGXX_HTTP_HEADERS_HPP
#define MGXX_HTTP_HEADERS_HPP

#include <algorithm>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

namespace mgxx::http {
class headers {
  struct case_insensitive_hash {
    using is_transparent = void;

    static size_t hash_key(const std::string_view key) {
      constexpr size_t k_prime = 1099511628211ULL;
      size_t hash = 14695981039346656037ULL;

      for (unsigned char c : key) {
        c = std::tolower(c);
        hash ^= c;
        hash *= k_prime;
      }

      return hash;
    }

    size_t operator()(const std::string_view key) const {
      return hash_key(key);
    }
    size_t operator()(const std::string& key) const { return hash_key(key); }
  };

  struct case_insensitive_equality {
    using is_transparent = void;

    static bool is_equal(const std::string_view lhs,
                         const std::string_view rhs) {
      return lhs.length() == rhs.length() &&
             std::equal(lhs.begin(), lhs.end(), rhs.begin(),
                        [](const unsigned char l, const unsigned char r) {
                          return std::tolower(l) == std::tolower(r);
                        });
    }

    bool operator()(const std::string_view lhs,
                    const std::string_view rhs) const {
      return is_equal(lhs, rhs);
    }
    bool operator()(const std::string& lhs, const std::string& rhs) const {
      return is_equal(lhs, rhs);
    }
    bool operator()(const std::string& lhs, const std::string_view rhs) const {
      return is_equal(lhs, rhs);
    }
    bool operator()(const std::string_view lhs, const std::string& rhs) const {
      return is_equal(lhs, rhs);
    }
  };

  std::unordered_map<std::string, std::string, case_insensitive_hash,
                     case_insensitive_equality>
      m_headers;

 public:
  [[nodiscard]] size_t size() const;
  [[nodiscard]] bool has(std::string_view name) const;
  [[nodiscard]] std::optional<std::string> get(std::string_view name) const;
  [[nodiscard]] std::optional<std::string_view> get_view(
      std::string_view name) const;
  void set(std::string name, std::string value);
  void remove(std::string_view name);
  void clear();

  [[nodiscard]] std::string format(size_t capacity = 0) const;
};
}  // namespace mgxx::http

#endif  // MGXX_HTTP_HEADERS_HPP
