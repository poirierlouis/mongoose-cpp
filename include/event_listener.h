#ifndef MONGOOSE_CPP_EVENT_LISTENER_H
#define MONGOOSE_CPP_EVENT_LISTENER_H

#include <memory>

#include "request.h"
#include "response.h"

namespace mg::http {
class event_listener {
  const size_t m_groups;

 public:
  explicit event_listener(const size_t groups) : m_groups(groups) {}
  virtual ~event_listener() = default;

  [[nodiscard]] size_t get_groups() const { return m_groups; }

  virtual void invoke(const request& req,
                      const std::shared_ptr<response>& res) = 0;
};

template <typename F>
class lambda_event_listener : public event_listener {
  F m_callback;

 public:
  explicit lambda_event_listener(F&& callback, const size_t groups)
      : event_listener(groups), m_callback(std::forward<F>(callback)) {}
  ~lambda_event_listener() override = default;

  void invoke(const request& req,
              const std::shared_ptr<response>& res) override {
    m_callback(req, res);
  }
};

template <typename T>
class klass_event_listener : public event_listener {
  T* m_target;

  void (T::*m_callback)(const request&, const std::shared_ptr<response>&);

 public:
  explicit klass_event_listener(
      T* target,
      void (T::*callback)(const request&, const std::shared_ptr<response>&), const size_t groups)
      : event_listener(groups), m_target(target), m_callback(callback) {}
  ~klass_event_listener() override = default;

  void invoke(const request& req,
              const std::shared_ptr<response>& res) override {
    (m_target->*m_callback)(req, res);
  }
};

class fn_event_listener : public event_listener {
  void (*m_callback)(const request&, const std::shared_ptr<response>&);

 public:
  explicit fn_event_listener(void (*callback)(const request&,
                                              const std::shared_ptr<response>&), const size_t groups)
      : event_listener(groups), m_callback(callback) {}
  ~fn_event_listener() override = default;

  void invoke(const request& req,
              const std::shared_ptr<response>& res) override {
    m_callback(req, res);
  }
};
}  // namespace mg::http

#endif  // MONGOOSE_CPP_EVENT_LISTENER_H
