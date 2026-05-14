#ifndef MGXX_INTERNAL_QUEUE_HPP
#define MGXX_INTERNAL_QUEUE_HPP

#include <mongoose.h>

#include <algorithm>
#include <optional>

namespace mgxx::internal {
template <typename T, std::size_t N = 128>
class queue {
  mg_queue m_queue{};
  alignas(T) char m_buffer[N * sizeof(T)]{};

 public:
  queue() { mg_queue_init(&m_queue, m_buffer, sizeof(m_buffer)); }

  [[nodiscard]] bool push(T&& item) {
    char* slot;
    if (mg_queue_book(&m_queue, &slot, sizeof(T)) < sizeof(T)) {
      // TODO: dynamically grow?
      return false;
    }

    new (slot) T(std::move(item));
    mg_queue_add(&m_queue, sizeof(T));
    return true;
  }

  [[nodiscard]] std::optional<T> pop() {
    char* slot;
    size_t length;
    if ((length = mg_queue_next(&m_queue, &slot)) == 0) {
      return std::nullopt;
    }

    T result = std::move(*reinterpret_cast<T*>(slot));
    reinterpret_cast<T*>(slot)->~T();
    mg_queue_del(&m_queue, length);

    return result;
  }
};
}  // namespace mgxx::internal

#endif  // MGXX_INTERNAL_QUEUE_HPP
