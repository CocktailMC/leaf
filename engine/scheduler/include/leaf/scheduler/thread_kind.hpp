#pragma once

#include <cstdint>
#include <string_view>

namespace leaf {

enum class thread_kind : std::uint8_t {
    unknown = 0,
    main = 1,
    worker = 2,
    io = 3,
};

[[nodiscard]] constexpr std::string_view to_string(thread_kind kind) noexcept {
    switch (kind) {
        case thread_kind::unknown: return "unknown";
        case thread_kind::main: return "main";
        case thread_kind::worker: return "worker";
        case thread_kind::io: return "io";
    }
    return "unknown";
}

} // namespace leaf
