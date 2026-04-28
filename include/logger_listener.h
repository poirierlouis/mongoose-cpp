#ifndef MONGOOSE_CPP_LOGGER_LISTENER_H
#define MONGOOSE_CPP_LOGGER_LISTENER_H

#include <string_view>

namespace mg {
class logger_listener {
 public:
  virtual ~logger_listener() = default;

  virtual void invoke(std::string_view message) const = 0;
};

template <typename F>
class lambda_logger_listener : public logger_listener {
  F m_callback;

 public:
  explicit lambda_logger_listener(F&& callback)
      : m_callback(std::forward<F>(callback)) {}
  ~lambda_logger_listener() override = default;

  void invoke(std::string_view message) const override { m_callback(message); }
};

template <typename T>
class klass_logger_listener : public logger_listener {
  T* m_target;

  void (T::*m_callback)(std::string_view);

 public:
  explicit klass_logger_listener(T* target,
                                 void (T::*callback)(std::string_view))
      : m_target(target), m_callback(callback) {}
  ~klass_logger_listener() override = default;

  void invoke(std::string_view message) const override {
    (m_target->*m_callback)(message);
  }
};

class fn_logger_listener : public logger_listener {
  void (*m_callback)(std::string_view);

 public:
  explicit fn_logger_listener(void (*callback)(std::string_view))
      : m_callback(callback) {}
  ~fn_logger_listener() override = default;

  void invoke(const std::string_view message) const override {
    m_callback(message);
  }
};
}  // namespace mg

#endif  // MONGOOSE_CPP_LOGGER_LISTENER_H
