#ifndef MONGOOSE_CPP_WEB_LISTENER_H
#define MONGOOSE_CPP_WEB_LISTENER_H

#include <memory>

#include "async_response.h"
#include "request.h"
#include "response.h"

namespace mg {
class web_listener {
  const size_t m_groups;
  const bool m_is_async;

 public:
  explicit web_listener(const size_t groups, const bool is_async)
      : m_groups(groups), m_is_async(is_async) {}
  virtual ~web_listener() = default;

  [[nodiscard]] size_t get_groups() const { return m_groups; }
  [[nodiscard]] bool is_async() const { return m_is_async; }

  virtual void invoke(const http::request& req,
                      const std::shared_ptr<http::response>& res) {};
  virtual void invoke(const http::request& req,
                      const std::shared_ptr<http::async_response>& res) {};
};

template <typename F, class R>
class lambda_web_listener : public web_listener {
  F m_callback;

 public:
  explicit lambda_web_listener(F&& callback, const size_t groups)
      : web_listener(groups, std::is_same_v<R, http::async_response>),
        m_callback(std::forward<F>(callback)) {}
  ~lambda_web_listener() override = default;

  void invoke(const http::request& req,
              const std::shared_ptr<R>& res) override {
    m_callback(req, res);
  }
};

}  // namespace mg

#endif  // MONGOOSE_CPP_WEB_LISTENER_H
