#ifndef MONGOOSE_CPP_HTTP_LISTENER_H
#define MONGOOSE_CPP_HTTP_LISTENER_H

#include <memory>

#include "async_response.h"
#include "request.h"
#include "response.h"

namespace mg::http {
class listener {
  const size_t m_groups;
  const bool m_is_async;

 public:
  explicit listener(const size_t groups, const bool is_async)
      : m_groups(groups), m_is_async(is_async) {}
  virtual ~listener() = default;

  [[nodiscard]] size_t get_groups() const { return m_groups; }
  [[nodiscard]] bool is_async() const { return m_is_async; }

  virtual void invoke(const request& req,
                      const std::shared_ptr<response>& res) {};
  virtual void invoke(const request& req,
                      const std::shared_ptr<async_response>& res) {};
};

template <typename F, class R>
class lambda_http_listener : public listener {
  F m_callback;

 public:
  explicit lambda_http_listener(F&& callback, const size_t groups)
      : listener(groups, std::is_same_v<R, async_response>),
        m_callback(std::forward<F>(callback)) {}
  ~lambda_http_listener() override = default;

  void invoke(const request& req, const std::shared_ptr<R>& res) override {
    m_callback(req, res);
  }
};

}  // namespace mg::http

#endif  // MONGOOSE_CPP_HTTP_LISTENER_H
