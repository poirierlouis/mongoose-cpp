#ifndef MGXX_LISTENER_HPP
#define MGXX_LISTENER_HPP

#include <utility>

namespace mgxx {
template <typename... Args>
class listener {
 public:
  virtual ~listener() = default;

  virtual void invoke(Args...) = 0;
};

template <typename F, typename... Args>
class lambda_listener : public listener<Args...> {
  F m_callback;

 public:
  explicit lambda_listener(F&& callback)
      : m_callback(std::forward<F>(callback)) {}
  ~lambda_listener() override = default;

  void invoke(Args... args) override {
    m_callback(std::forward<Args>(args)...);
  }
};
}  // namespace mgxx

#endif  // MGXX_LISTENER_HPP
