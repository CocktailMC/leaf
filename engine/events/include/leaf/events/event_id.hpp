#pragma once

#include <cstdint>

namespace leaf {

using event_id = std::uint64_t;
using subscription_id = std::uint64_t;
using mod_id = std::uint64_t;

inline constexpr mod_id engine_mod_id = 0;
inline constexpr mod_id test_mod_id = 1;
inline constexpr mod_id dynamic_mod_id_base = 1000;

namespace event_ids {

inline constexpr event_id core_server_starting = 0x0001;
inline constexpr event_id core_server_started = 0x0002;

inline constexpr event_id player_join = 0x0100;
inline constexpr event_id player_leave = 0x0101;
inline constexpr event_id player_join_request = 0x0102;
inline constexpr event_id player_chat = 0x0103;
inline constexpr event_id player_death = 0x0104;

inline constexpr event_id entity_spawn = 0x0200;
inline constexpr event_id entity_remove = 0x0201;

inline constexpr event_id world_load = 0x0300;
inline constexpr event_id block_break = 0x0301;
inline constexpr event_id block_place = 0x0302;

inline constexpr event_id server_tick = 0x0400;

inline constexpr event_id dynamic_id_base = 0x8000'0000ULL;

} // namespace event_ids

} // namespace leaf
