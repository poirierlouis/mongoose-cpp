#ifndef MGXX_LOGGER_LISTENER_HPP
#define MGXX_LOGGER_LISTENER_HPP

#include <string_view>

#include "mgxx/listener.hpp"

namespace mgxx {
using logger_listener = listener<std::string_view>;
template <typename F>
using lambda_logger_listener = lambda_listener<F, std::string_view>;
}  // namespace mgxx

#endif  // MGXX_LOGGER_LISTENER_HPP
