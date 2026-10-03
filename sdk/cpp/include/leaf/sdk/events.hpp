#pragma once

#include "leaf/abi/leaf_event_v1.h"

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace leaf::sdk {

/// Immutable view of a C ABI event packet: `[LeafEventHeaderV1][payload…]`.
/// Valid only for the duration of the listener callback.
class event_view {
public:
    explicit event_view(const void* event) noexcept
        : header_(static_cast<const LeafEventHeaderV1*>(event)) {}

    [[nodiscard]] bool valid() const noexcept { return header_ != nullptr; }

    [[nodiscard]] const LeafEventHeaderV1& header() const noexcept { return *header_; }

    [[nodiscard]] std::uint64_t id() const noexcept { return header_->event_id; }

    [[nodiscard]] std::uint32_t schema_version() const noexcept {
        return header_->schema_version;
    }

    [[nodiscard]] std::uint32_t payload_size() const noexcept {
        return header_->payload_size;
    }

    [[nodiscard]] const std::byte* payload_bytes() const noexcept {
        if (!header_ || header_->payload_size == 0) {
            return nullptr;
        }
        return reinterpret_cast<const std::byte*>(header_ + 1);
    }

    template <typename T>
        requires std::is_trivially_copyable_v<T>
    [[nodiscard]] const T* payload_as() const noexcept {
        if (!header_ || header_->payload_size < sizeof(T)) {
            return nullptr;
        }
        return reinterpret_cast<const T*>(header_ + 1);
    }

private:
    const LeafEventHeaderV1* header_{nullptr};
};

using event_callback = void (*)(event_view view, void* user_data);

struct player_join_event {
    LeafHandle player{0};
};

struct player_leave_event {
    LeafHandle player{0};
};

struct player_join_request_event {
    LeafHandle player{0};
};

struct player_chat_event {
    LeafHandle player{0};
    const char* message{""};
};

struct player_death_event {
    LeafHandle player{0};
};

struct entity_lifecycle_event {
    LeafHandle entity{0};
    std::uint32_t entity_type_id{0};
    std::int32_t x{0};
    std::int32_t y{0};
    std::int32_t z{0};
    std::uint32_t dimension{0};
};

struct world_load_event {
    std::uint32_t dimension{0};
};

[[nodiscard]] inline player_join_event as_player_join(event_view view) noexcept {
    player_join_event out{};
    if (auto* p = view.payload_as<LeafPlayerJoinPayloadV1>()) {
        out.player = p->player;
    }
    return out;
}

[[nodiscard]] inline player_leave_event as_player_leave(event_view view) noexcept {
    player_leave_event out{};
    if (auto* p = view.payload_as<LeafPlayerLeavePayloadV1>()) {
        out.player = p->player;
    }
    return out;
}

[[nodiscard]] inline player_join_request_event as_player_join_request(
    event_view view) noexcept {
    player_join_request_event out{};
    if (auto* p = view.payload_as<LeafPlayerJoinRequestPayloadV1>()) {
        out.player = p->player;
    }
    return out;
}

[[nodiscard]] inline player_chat_event as_player_chat(event_view view) noexcept {
    player_chat_event out{};
    if (auto* p = view.payload_as<LeafPlayerChatPayloadV1>()) {
        out.player = p->player;
        out.message = p->message;
    }
    return out;
}

[[nodiscard]] inline player_death_event as_player_death(event_view view) noexcept {
    player_death_event out{};
    if (auto* p = view.payload_as<LeafPlayerDeathPayloadV1>()) {
        out.player = p->player;
    }
    return out;
}

[[nodiscard]] inline entity_lifecycle_event as_entity_lifecycle(
    event_view view) noexcept {
    entity_lifecycle_event out{};
    if (auto* p = view.payload_as<LeafEntityLifecyclePayloadV1>()) {
        out.entity = p->entity;
        out.entity_type_id = p->entity_type_id;
        out.x = p->x;
        out.y = p->y;
        out.z = p->z;
        out.dimension = p->dimension;
    }
    return out;
}

[[nodiscard]] inline world_load_event as_world_load(event_view view) noexcept {
    world_load_event out{};
    if (auto* p = view.payload_as<LeafWorldLoadPayloadV1>()) {
        out.dimension = p->dimension;
    }
    return out;
}

struct block_change_event {
    LeafHandle player{0};
    std::int32_t x{0};
    std::int32_t y{0};
    std::int32_t z{0};
    std::uint32_t block_id{0};
};

[[nodiscard]] inline block_change_event as_block_change(event_view view) noexcept {
    block_change_event out{};
    if (auto* p = view.payload_as<LeafBlockChangePayloadV1>()) {
        out.player = p->player;
        out.x = p->x;
        out.y = p->y;
        out.z = p->z;
        out.block_id = p->block_id;
    }
    return out;
}

} // namespace leaf::sdk
