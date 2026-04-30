#ifndef MONGOOSE_CPP_LISTENER_H
#define MONGOOSE_CPP_LISTENER_H

#include <utility>

namespace mg {
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
}  // namespace mg

#endif  // MONGOOSE_CPP_LISTENER_H
