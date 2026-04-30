#ifndef MONGOOSE_CPP_LOGGER_LISTENER_H
#define MONGOOSE_CPP_LOGGER_LISTENER_H

#include <string_view>

#include "listener.h"

namespace mg {
using logger_listener = listener<std::string_view>;
template <typename F>
using lambda_logger_listener = lambda_listener<F, std::string_view>;
}  // namespace mg

#endif  // MONGOOSE_CPP_LOGGER_LISTENER_H
