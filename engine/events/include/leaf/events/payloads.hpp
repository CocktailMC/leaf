#pragma once

#include <cstdint>

namespace leaf {

/// Schema v1 fixed payloads for core events (POD, ABI-safe).
struct player_join_payload_v1 {
    std::uint64_t player_handle{0};
};

struct player_leave_payload_v1 {
    std::uint64_t player_handle{0};
};

struct player_join_request_payload_v1 {
    std::uint64_t player_handle{0};
};

struct player_chat_payload_v1 {
    std::uint64_t player_handle{0};
    char message[256]{};
};

struct player_death_payload_v1 {
    std::uint64_t player_handle{0};
};

struct entity_lifecycle_payload_v1 {
    std::uint64_t entity_handle{0};
    std::uint32_t entity_type_id{0};
    std::int32_t x{0};
    std::int32_t y{0};
    std::int32_t z{0};
    std::uint32_t dimension{0};
};

struct world_load_payload_v1 {
    std::uint32_t dimension{0};
};

struct block_change_payload_v1 {
    std::uint64_t player_handle{0};
    std::int32_t x{0};
    std::int32_t y{0};
    std::int32_t z{0};
    std::uint32_t block_id{0};
};

} // namespace leaf
