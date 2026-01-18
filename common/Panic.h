#pragma once

#include <spdlog/fmt/fmt.h>

#include "FatalException.h"

namespace stargate {

template <typename... Args>
inline void panic(fmt::format_string<Args...>&& fmt, Args&&... args) {
    throw FatalException(fmt::format(fmt, std::forward<Args>(args)...));
}

}
