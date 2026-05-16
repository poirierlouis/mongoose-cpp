#ifndef MGXX_ENDPOINT_HPP
#define MGXX_ENDPOINT_HPP

#include <mongoose.h>

#include <memory>
#include <string>

namespace mgxx {
class server;

namespace http {
class remote_context;
}
}  // namespace mgxx

namespace mgxx {
class endpoint {
  friend void event_manager_handler(mg_connection*, int, void*);

  const std::string m_host;
  mg_connection* m_conn;

  void setup(mg_connection* conn);

  friend class mgxx::server;
  friend class mgxx::http::remote_context;

 protected:
  std::weak_ptr<mg_mgr> m_mgr;

  virtual void handle(mg_connection* conn, int ev, void* ev_data) = 0;

 public:
  explicit endpoint(std::weak_ptr<mg_mgr> mgr, std::string host);
  virtual ~endpoint() = default;

  [[nodiscard]] std::string_view get_host() const;
  [[nodiscard]] unsigned long get_conn_id() const;
};
}  // namespace mgxx

#endif  // MGXX_ENDPOINT_HPP
