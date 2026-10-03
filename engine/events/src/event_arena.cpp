#include "leaf/events/event_arena.hpp"

#include <cstring>

namespace leaf {

event_arena::event_arena(std::size_t initial_bytes)
    : buffer_(initial_bytes) {}

void event_arena::reset() noexcept {
    used_ = 0;
}

result<std::span<std::byte>> event_arena::allocate(std::size_t bytes, std::size_t align) {
    if (align == 0 || (align & (align - 1)) != 0) {
        return err<std::span<std::byte>>(
            ec::invalid_argument,
            "alignment must be power of two");
    }

    const std::size_t aligned = (used_ + (align - 1)) & ~(align - 1);
    if (aligned + bytes > buffer_.size()) {
        const std::size_t need = aligned + bytes;
        std::size_t cap = buffer_.empty() ? 4096 : buffer_.size();
        while (cap < need) {
            cap *= 2;
        }
        buffer_.resize(cap);
    }

    used_ = aligned + bytes;
    return std::span<std::byte>{buffer_.data() + aligned, bytes};
}

result<event_packet_view> event_arena::build_packet(
    event_header header,
    std::span<const std::byte> payload,
    std::span<const std::byte> variable) {
    header.payload_size = static_cast<std::uint32_t>(payload.size());
    if (!variable.empty()) {
        header.flags |= event_flag_has_variable_data;
    }

    // Contiguous layout for C ABI: [header][payload][variable]
    const std::size_t total =
        sizeof(event_header) + payload.size() + variable.size();
    auto mem = allocate(total, alignof(event_header));
    if (!mem) {
        return err<event_packet_view>(mem.error());
    }

    std::byte* base = mem->data();
    std::memcpy(base, &header, sizeof(event_header));
    if (!payload.empty()) {
        std::memcpy(base + sizeof(event_header), payload.data(), payload.size());
    }
    if (!variable.empty()) {
        std::memcpy(
            base + sizeof(event_header) + payload.size(),
            variable.data(),
            variable.size());
    }

    auto* hdr = reinterpret_cast<event_header*>(base);
    const std::byte* payload_ptr =
        payload.empty() ? nullptr : base + sizeof(event_header);
    const std::byte* var_ptr = variable.empty()
        ? nullptr
        : base + sizeof(event_header) + payload.size();

    return event_packet_view{hdr, payload_ptr, var_ptr, variable.size()};
}

} // namespace leaf
