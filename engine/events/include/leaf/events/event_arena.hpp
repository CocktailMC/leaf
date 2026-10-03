#pragma once

#include <cstddef>
#include <cstdint>
#include <memory_resource>
#include <span>
#include <vector>

#include "leaf/core/result.hpp"
#include "leaf/events/event_header.hpp"

namespace leaf {

/// Bump allocator for a single publish/dispatch. Reset after dispatch completes.
/// Listeners must NOT retain pointers into this arena past the callback.
class event_arena {
public:
    explicit event_arena(std::size_t initial_bytes = 4096);

    event_arena(const event_arena&) = delete;
    event_arena& operator=(const event_arena&) = delete;

    void reset() noexcept;

    [[nodiscard]] result<std::span<std::byte>> allocate(std::size_t bytes, std::size_t align = 8);

    [[nodiscard]] result<event_packet_view> build_packet(
        event_header header,
        std::span<const std::byte> payload,
        std::span<const std::byte> variable = {});

    [[nodiscard]] std::size_t used_bytes() const noexcept { return used_; }
    [[nodiscard]] std::size_t capacity_bytes() const noexcept { return buffer_.size(); }

private:
    std::vector<std::byte> buffer_;
    std::size_t used_{0};
};

} // namespace leaf
