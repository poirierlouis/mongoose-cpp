#ifndef MGXX_ASSET_HPP
#define MGXX_ASSET_HPP

#include <string>

namespace mgxx::http {
struct asset {
  bool is_dir{false};
  std::string path;
  std::string mime_type;
  std::string headers;
};
}  // namespace mgxx::http

#endif  // MGXX_ASSET_HPP
