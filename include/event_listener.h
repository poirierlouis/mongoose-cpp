#ifndef MONGOOSE_CPP_EVENT_LISTENER_H
#define MONGOOSE_CPP_EVENT_LISTENER_H

#include <memory>

#include "async_response.h"
#include "request.h"
#include "response.h"

namespace mg::http {
class event_listener {
  const size_t m_groups;
  const bool m_is_async;

 public:
  explicit event_listener(const size_t groups, const bool is_async)
      : m_groups(groups), m_is_async(is_async) {}
  virtual ~event_listener() = default;

  [[nodiscard]] size_t get_groups() const { return m_groups; }

  [[nodiscard]] bool is_async() const { return m_is_async; }

  virtual void invoke(const request& req,
                      const std::shared_ptr<response>& res) {}
  virtual void invoke(const request& req,
                      const std::shared_ptr<async_response>& res) {}
};

template <typename F, class R>
class lambda_event_listener : public event_listener {
  F m_callback;

 public:
  explicit lambda_event_listener(F&& callback, const size_t groups)
      : event_listener(groups, std::is_same_v<R, async_response>),
        m_callback(std::forward<F>(callback)) {}
  ~lambda_event_listener() override = default;

  void invoke(const request& req, const std::shared_ptr<R>& res) override {
    m_callback(req, res);
  }
};

template <typename T, class R>
class klass_event_listener : public event_listener {
  T* m_target;

  void (T::*m_callback)(const request&, const std::shared_ptr<R>&);

 public:
  explicit klass_event_listener(T* target,
                                void (T::*callback)(const request&,
                                                    const std::shared_ptr<R>&),
                                const size_t groups)
      : event_listener(groups, std::is_same_v<R, async_response>),
        m_target(target),
        m_callback(callback) {}
  ~klass_event_listener() override = default;

  void invoke(const request& req, const std::shared_ptr<R>& res) override {
    (m_target->*m_callback)(req, res);
  }
};

template <class R>
class fn_event_listener : public event_listener {
  void (*m_callback)(const request&, const std::shared_ptr<R>&);

 public:
  explicit fn_event_listener(void (*callback)(const request&,
                                              const std::shared_ptr<R>&),
                             const size_t groups)
      : event_listener(groups, std::is_same_v<R, async_response>),
        m_callback(callback) {}
  ~fn_event_listener() override = default;

  void invoke(const request& req, const std::shared_ptr<R>& res) override {
    m_callback(req, res);
  }
};
}  // namespace mg::http

#endif  // MONGOOSE_CPP_EVENT_LISTENER_H
