#pragma once

#include <cstdint>
#include <functional>

namespace leaf {

/// Opaque typed handle. Leaf Mods never see Minecraft Java objects —
/// only these value types that the engine resolves via handle_table.
template <typename Tag>
class handle {
public:
    using value_type = std::uint64_t;

    static constexpr value_type null_value = 0;

    constexpr handle() noexcept = default;

    constexpr explicit handle(value_type value) noexcept
        : value_(value) {}

    [[nodiscard]] constexpr value_type raw() const noexcept { return value_; }

    [[nodiscard]] constexpr bool valid() const noexcept { return value_ != null_value; }

    [[nodiscard]] constexpr explicit operator bool() const noexcept { return valid(); }

    [[nodiscard]] friend constexpr bool operator==(handle, handle) noexcept = default;

    [[nodiscard]] friend constexpr auto operator<=>(handle, handle) noexcept = default;

private:
    value_type value_{null_value};
};

struct player_tag {};
struct world_tag {};
struct entity_tag {};
struct server_tag {};
struct item_stack_tag {};
struct block_pos_tag {};

using player_handle = handle<player_tag>;
using world_handle = handle<world_tag>;
using entity_handle = handle<entity_tag>;
using server_handle = handle<server_tag>;
using item_stack_handle = handle<item_stack_tag>;
using block_pos_handle = handle<block_pos_tag>;

/// Pack index (low 32) + generation (high 32) into a raw handle value.
[[nodiscard]] constexpr std::uint64_t pack_handle_bits(
    std::uint32_t index,
    std::uint32_t generation) noexcept {
    return (static_cast<std::uint64_t>(generation) << 32)
        | static_cast<std::uint64_t>(index);
}

[[nodiscard]] constexpr std::uint32_t handle_index(std::uint64_t raw) noexcept {
    return static_cast<std::uint32_t>(raw & 0xFFFFFFFFu);
}

[[nodiscard]] constexpr std::uint32_t handle_generation(std::uint64_t raw) noexcept {
    return static_cast<std::uint32_t>(raw >> 32);
}

} // namespace leaf

template <typename Tag>
struct std::hash<leaf::handle<Tag>> {
    [[nodiscard]] std::size_t operator()(leaf::handle<Tag> h) const noexcept {
        return std::hash<std::uint64_t>{}(h.raw());
    }
};
