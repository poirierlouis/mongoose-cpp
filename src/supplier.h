#ifndef MONGOOSE_CPP_SUPPLIER_H
#define MONGOOSE_CPP_SUPPLIER_H

#include <utility>

namespace mg {
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
}  // namespace mg

#endif  // MONGOOSE_CPP_SUPPLIER_H
