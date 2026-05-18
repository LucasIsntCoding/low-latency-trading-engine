#pragma once

#include <chrono>
#include <cstdint>
#include <string>

namespace llt {

inline std::uint64_t now_ns() noexcept {
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());
}

inline std::string wall_clock_iso8601() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t t = std::chrono::system_clock::to_time_t(now);
    char buf[64]{};
#if defined(_WIN32)
    tm tm_buf{};
    gmtime_s(&tm_buf, &t);
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm_buf);
#else
    tm tm_buf{};
    gmtime_r(&t, &tm_buf);
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm_buf);
#endif
    return std::string(buf);
}

}  // namespace llt
