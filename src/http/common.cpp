#include "common.h"

#include <mongoose.h>

#include <format>

#ifdef WIN32
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#endif

namespace mg {
std::string format_ip(const mg_addr* ip) {
  if (ip->is_ip6) {
    char buffer[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, &ip->addr.ip6, buffer, sizeof(buffer));
    return std::format("[{}]:{}", buffer, mg_ntohs(ip->port));
  }

  char buffer[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &ip->addr.ip, buffer, sizeof(buffer));
  return std::format("{}:{}", buffer, mg_ntohs(ip->port));
}
}  // namespace mg