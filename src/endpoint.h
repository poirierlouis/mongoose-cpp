#ifndef MONGOOSE_CPP_ENDPOINT_H
#define MONGOOSE_CPP_ENDPOINT_H

#include <mongoose.h>

#include <memory>
#include <string>

namespace mg {
class server;

namespace http {
class remote_context;
}
}  // namespace mg

namespace mg {
class endpoint {
  friend void event_manager_handler(mg_connection*, int, void*);

  const std::string m_host;
  mg_connection* m_conn;

  void setup(mg_connection* conn);

  friend class mg::server;
  friend class mg::http::remote_context;

 protected:
  std::weak_ptr<mg_mgr> m_mgr;

  virtual void handle(mg_connection* conn, int ev, void* ev_data) = 0;

 public:
  explicit endpoint(std::weak_ptr<mg_mgr> mgr, std::string host);
  virtual ~endpoint() = default;

  [[nodiscard]] std::string_view get_host() const;
};
}  // namespace mg

#endif  // MONGOOSE_CPP_ENDPOINT_H
