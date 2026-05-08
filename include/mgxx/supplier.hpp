#ifndef MGXX_SUPPLIER_HPP
#define MGXX_SUPPLIER_HPP

#include <utility>

namespace mgxx {
template <typename T>
class supplier {
 public:
  virtual ~supplier() = default;

  virtual T get() = 0;
};

template <typename T, typename F>
class lambda_supplier : public supplier<T> {
  F m_callback;

 public:
  explicit lambda_supplier(F&& callback)
      : m_callback(std::forward<F>(callback)) {}
  ~lambda_supplier() override = default;

  T get() override { return m_callback(); }
};
}  // namespace mgxx

#endif  // MGXX_SUPPLIER_HPP
