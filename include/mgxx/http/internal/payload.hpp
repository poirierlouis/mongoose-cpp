#ifndef MGXX_HTTP_INTERNAL_PAYLOAD_HPP
#define MGXX_HTTP_INTERNAL_PAYLOAD_HPP

#include <string>

#include "mgxx/internal/queue.hpp"

#ifndef MGXX_HTTP_QUEUE_RESPONSE_SIZE
#define MGXX_HTTP_QUEUE_RESPONSE_SIZE 512
#endif

#ifndef MGXX_HTTP_QUEUE_STREAM_SIZE
#define MGXX_HTTP_QUEUE_STREAM_SIZE 128
#endif

namespace mgxx::http::internal {
struct payload {
  unsigned long conn{0};
  int code{-1};
  std::string headers;
  std::string body;

  payload() = default;
  ~payload() = default;

  payload(const payload&) = default;
  payload& operator=(const payload&) = default;

  payload(payload&&) noexcept = default;
  payload& operator=(payload&&) = default;
};

struct payload_stream {
  enum class state : uint8_t { preamble, chunk };

  unsigned long conn{0};
  state state{state::preamble};
  std::string data;
  void* stream{nullptr};
};

using queue_response =
    mgxx::internal::queue<internal::payload, MGXX_HTTP_QUEUE_RESPONSE_SIZE>;
using queue_stream = mgxx::internal::queue<internal::payload_stream,
                                           MGXX_HTTP_QUEUE_STREAM_SIZE>;
}  // namespace mgxx::http::internal

#endif  // MGXX_HTTP_INTERNAL_PAYLOAD_HPP
