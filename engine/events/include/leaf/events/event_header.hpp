#pragma once

#include <cstddef>
#include <cstdint>

#include "leaf/events/event_id.hpp"
#include "leaf/events/event_types.hpp"

namespace leaf {

inline constexpr std::uint32_t event_header_size_v1 = 40;

struct alignas(8) event_header {
    event_id id{0};
    std::uint32_t schema_version{1};
    std::uint32_t payload_size{0};
    std::uint64_t timestamp_ns{0};
    std::uint64_t sequence{0};
    std::uint32_t flags{0};
    std::uint32_t source{static_cast<std::uint32_t>(event_source::unknown)};
};

static_assert(sizeof(event_header) == event_header_size_v1);
static_assert(alignof(event_header) == 8);

inline constexpr std::uint32_t event_flag_has_variable_data = 1u << 0;

/// Immutable view of a published event packet.
/// Valid only for the duration of a listener callback (or publish call).
class event_packet_view {
public:
    event_packet_view() = default;

    event_packet_view(
        const event_header* header,
        const std::byte* payload,
        const std::byte* variable,
        std::size_t variable_size) noexcept
        : header_(header)
        , payload_(payload)
        , variable_(variable)
        , variable_size_(variable_size) {}

    [[nodiscard]] bool valid() const noexcept { return header_ != nullptr; }

    [[nodiscard]] const event_header& header() const noexcept { return *header_; }

    [[nodiscard]] event_id id() const noexcept { return header_->id; }

    [[nodiscard]] std::uint32_t schema_version() const noexcept {
        return header_->schema_version;
    }

    [[nodiscard]] const std::byte* payload_bytes() const noexcept { return payload_; }

    [[nodiscard]] std::size_t payload_size() const noexcept {
        return header_->payload_size;
    }

    template <typename T>
    [[nodiscard]] const T* payload_as() const noexcept {
        if (!payload_ || header_->payload_size < sizeof(T)) {
            return nullptr;
        }
        return reinterpret_cast<const T*>(payload_);
    }

    [[nodiscard]] const std::byte* variable_data() const noexcept { return variable_; }

    [[nodiscard]] std::size_t variable_size() const noexcept { return variable_size_; }

private:
    const event_header* header_{nullptr};
    const std::byte* payload_{nullptr};
    const std::byte* variable_{nullptr};
    std::size_t variable_size_{0};
};

} // namespace leaf
