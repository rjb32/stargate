#pragma once

#include <stdlib.h>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>

namespace stargate {

template <typename... Args>
inline void panic(fmt::format_string<Args...>&& fmt, Args&&... args) {
    spdlog::error(fmt, std::forward<Args>(args)...);
    exit(EXIT_FAILURE);
}

}