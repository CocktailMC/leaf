#pragma once

#include <bitset>
#include <cstdint>

namespace leaf {

/// Capability identifiers. Prefer these over loader/version branching in mods.
enum class capability : std::uint32_t {
    data_components = 1,
    legacy_item_nbt = 2,
    custom_payload_v2 = 3,
    registry_modern = 4,
    registry_legacy = 5,
    client_render_v2 = 6,
    server_transfer = 7,
    inventory_stack_v1 = 8,

    _count = 9, // sentinel; not a real capability
};

class capability_set {
public:
    capability_set() = default;

    void enable(capability cap) {
        bits_.set(index_of(cap));
    }

    void disable(capability cap) {
        bits_.reset(index_of(cap));
    }

    [[nodiscard]] bool has(capability cap) const noexcept {
        return bits_.test(index_of(cap));
    }

    [[nodiscard]] bool empty() const noexcept { return bits_.none(); }

    [[nodiscard]] std::size_t count() const noexcept { return bits_.count(); }

    [[nodiscard]] friend bool operator==(
        const capability_set&,
        const capability_set&) noexcept = default;

private:
    static constexpr std::size_t index_of(capability cap) noexcept {
        return static_cast<std::size_t>(cap);
    }

    // Bit 0 unused so enum values map directly.
    std::bitset<64> bits_{};
};

} // namespace leaf
