/* Leaf Bridge Host ABI v1
 *
 * Thin entry surface used by Fabric / Forge / NeoForge bootstrap JARs.
 * Bridges never load .leafmod files — they only start the native engine
 * and forward lifecycle / hooks.
 */

#ifndef LEAF_BRIDGE_V1_H
#define LEAF_BRIDGE_V1_H

#include <stdint.h>

#include "leaf/abi/leaf_error_v1.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32) || defined(__CYGWIN__)
#  if defined(LEAF_BRIDGE_BUILD)
#    define LEAF_BRIDGE_API __declspec(dllexport)
#  else
#    define LEAF_BRIDGE_API __declspec(dllimport)
#  endif
#else
#  define LEAF_BRIDGE_API __attribute__((visibility("default")))
#endif

enum LeafLoaderKind {
    LEAF_LOADER_UNKNOWN = 0,
    LEAF_LOADER_FABRIC = 1,
    LEAF_LOADER_FORGE = 2,
    LEAF_LOADER_NEOFORGE = 3
};

enum LeafMappingKind {
    LEAF_MAPPING_UNKNOWN = 0,
    LEAF_MAPPING_MOJMAP = 1,
    LEAF_MAPPING_YARN = 2,
    LEAF_MAPPING_INTERMEDIARY = 3,
    LEAF_MAPPING_SRG = 4
};

typedef struct LeafBridgeConfigV1 {
    uint32_t struct_size;

    /* e.g. "1.21.1" */
    const char* minecraft_version;

    /* Absolute or game-relative path to the leafmods directory. */
    const char* leafmods_dir;

    /* Optional game root (for logs/config). May be NULL. */
    const char* game_dir;

    uint32_t loader;   /* LeafLoaderKind */
    uint32_t mapping;  /* LeafMappingKind */

    /* 1 = call discover/resolve/load/enable during init.
       0 = init host only (tests / deferred load). */
    uint32_t auto_load_mods;
} LeafBridgeConfigV1;

typedef struct LeafBridgeInfoV1 {
    uint32_t struct_size;
    uint32_t abi_major;
    uint32_t abi_minor;
    const char* engine_version; /* static string, do not free */
    uint32_t loader;
    uint32_t mapping;
    const char* minecraft_version; /* owned by host until shutdown */
} LeafBridgeInfoV1;

/* Create the process-wide bridge host + native engine. */
LEAF_BRIDGE_API leaf_error_code leaf_bridge_init(
    const LeafBridgeConfigV1* config);

/* Flat init helper for FFM / language bindings (avoids struct marshalling). */
LEAF_BRIDGE_API leaf_error_code leaf_bridge_init_flat(
    const char* minecraft_version,
    const char* leafmods_dir,
    const char* game_dir,
    uint32_t loader,
    uint32_t mapping,
    uint32_t auto_load_mods);

LEAF_BRIDGE_API leaf_error_code leaf_bridge_shutdown(void);

LEAF_BRIDGE_API int leaf_bridge_is_initialized(void);

/* Drain main-thread scheduler queue. Call from Minecraft main thread each tick. */
LEAF_BRIDGE_API leaf_error_code leaf_bridge_pump_main(uint32_t budget);

LEAF_BRIDGE_API leaf_error_code leaf_bridge_get_info(LeafBridgeInfoV1* out_info);

/* Introspection for bridges / harnesses (UTF-8 ids, NUL-terminated). */
LEAF_BRIDGE_API uint32_t leaf_bridge_loaded_mod_count(void);
LEAF_BRIDGE_API leaf_error_code leaf_bridge_mod_id_at(
    uint32_t index,
    char* out_buf,
    uint32_t out_buf_size);

/* Lifecycle forwards from loader bridges. */
LEAF_BRIDGE_API leaf_error_code leaf_bridge_on_server_starting(void);
LEAF_BRIDGE_API leaf_error_code leaf_bridge_on_server_started(void);
LEAF_BRIDGE_API leaf_error_code leaf_bridge_on_server_stopping(void);

/* Canonical event emits (handles are opaque LeafHandle values). */
LEAF_BRIDGE_API leaf_error_code leaf_bridge_emit_player_join(uint64_t player_handle);
LEAF_BRIDGE_API leaf_error_code leaf_bridge_emit_player_leave(uint64_t player_handle);

/* Decision emit: out_decision receives LEAF_DECISION_* from leaf_event_v1.h */
LEAF_BRIDGE_API leaf_error_code leaf_bridge_emit_player_join_request(
    uint64_t player_handle,
    uint32_t* out_decision);

/* Decision emit for chat: out_decision receives LEAF_DECISION_* */
LEAF_BRIDGE_API leaf_error_code leaf_bridge_emit_player_chat(
    uint64_t player_handle,
    const char* message,
    uint32_t* out_decision);

LEAF_BRIDGE_API leaf_error_code leaf_bridge_emit_player_death(
    uint64_t player_handle);

LEAF_BRIDGE_API leaf_error_code leaf_bridge_emit_entity_spawn(
    uint64_t entity_handle,
    uint32_t entity_type_id,
    int32_t x,
    int32_t y,
    int32_t z,
    uint32_t dimension);

LEAF_BRIDGE_API leaf_error_code leaf_bridge_emit_entity_remove(
    uint64_t entity_handle,
    uint32_t entity_type_id,
    int32_t x,
    int32_t y,
    int32_t z,
    uint32_t dimension);

LEAF_BRIDGE_API leaf_error_code leaf_bridge_emit_world_load(
    uint32_t dimension);

LEAF_BRIDGE_API leaf_error_code leaf_bridge_emit_block_break(
    uint64_t player_handle,
    int32_t x,
    int32_t y,
    int32_t z,
    uint32_t block_id,
    uint32_t* out_decision);

LEAF_BRIDGE_API leaf_error_code leaf_bridge_emit_block_place(
    uint64_t player_handle,
    int32_t x,
    int32_t y,
    int32_t z,
    uint32_t block_id,
    uint32_t* out_decision);

/* Re-discover leafmods and enable newly added packages (experimental). */
LEAF_BRIDGE_API leaf_error_code leaf_bridge_reload_mods(void);

/* Optional Java/Kotlin upcalls so stub ABI can reach the live game. */
typedef struct LeafMinecraftHooksV1 {
    uint32_t struct_size;
    void (*send_player_message)(uint64_t player_handle, const char* message);
    void (*broadcast_message)(const char* message);
    void (*log)(int level, const char* message);
    /* Return 0 on success. out_* must be non-NULL when provided. */
    int (*get_inventory_slot)(
        uint64_t player_handle,
        uint32_t slot,
        uint32_t* out_item_id,
        uint32_t* out_count);
    int (*set_inventory_slot)(
        uint64_t player_handle,
        uint32_t slot,
        uint32_t item_id,
        uint32_t count);
    int (*get_block)(
        uint32_t dimension,
        int32_t x,
        int32_t y,
        int32_t z,
        uint32_t* out_block_id);
} LeafMinecraftHooksV1;

LEAF_BRIDGE_API leaf_error_code leaf_bridge_set_minecraft_hooks(
    const LeafMinecraftHooksV1* hooks);

/* Flat FFM helper: pass NULL to clear. */
LEAF_BRIDGE_API void leaf_bridge_set_send_player_message_hook(
    void (*fn)(uint64_t player_handle, const char* message));

LEAF_BRIDGE_API void leaf_bridge_set_broadcast_message_hook(
    void (*fn)(const char* message));

LEAF_BRIDGE_API void leaf_bridge_set_inventory_hooks(
    int (*get_fn)(
        uint64_t player_handle,
        uint32_t slot,
        uint32_t* out_item_id,
        uint32_t* out_count),
    int (*set_fn)(
        uint64_t player_handle,
        uint32_t slot,
        uint32_t item_id,
        uint32_t count));

/* Prefer these when damage matters. Pass NULL pair to clear. */
LEAF_BRIDGE_API void leaf_bridge_set_inventory_stack_hooks(
    int (*get_fn)(
        uint64_t player_handle,
        uint32_t slot,
        uint32_t* out_item_id,
        uint32_t* out_count,
        int32_t* out_damage),
    int (*set_fn)(
        uint64_t player_handle,
        uint32_t slot,
        uint32_t item_id,
        uint32_t count,
        int32_t damage));

LEAF_BRIDGE_API void leaf_bridge_set_get_block_hook(
    int (*fn)(
        uint32_t dimension,
        int32_t x,
        int32_t y,
        int32_t z,
        uint32_t* out_block_id));

LEAF_BRIDGE_API void leaf_bridge_set_world_hooks(
    int (*get_fn)(
        uint32_t dimension,
        int32_t x,
        int32_t y,
        int32_t z,
        uint32_t* out_block_id),
    int (*set_fn)(
        uint32_t dimension,
        int32_t x,
        int32_t y,
        int32_t z,
        uint32_t block_id));

/* Prefer these for live teleport / position queries. Pass NULL pair to clear. */
LEAF_BRIDGE_API void leaf_bridge_set_player_pos_hooks(
    int (*get_fn)(
        uint64_t player_handle,
        int32_t* out_x,
        int32_t* out_y,
        int32_t* out_z,
        uint32_t* out_dimension),
    int (*set_fn)(
        uint64_t player_handle,
        int32_t x,
        int32_t y,
        int32_t z,
        uint32_t dimension));

LEAF_BRIDGE_API void leaf_bridge_set_player_health_hooks(
    int (*get_fn)(
        uint64_t player_handle,
        float* out_health,
        float* out_max_health),
    int (*set_fn)(uint64_t player_handle, float health));

LEAF_BRIDGE_API void leaf_bridge_set_player_food_hooks(
    int (*get_fn)(
        uint64_t player_handle,
        int32_t* out_food,
        float* out_saturation),
    int (*set_fn)(uint64_t player_handle, int32_t food, float saturation));

LEAF_BRIDGE_API void leaf_bridge_set_player_gamemode_hooks(
    int (*get_fn)(uint64_t player_handle, uint32_t* out_mode),
    int (*set_fn)(uint64_t player_handle, uint32_t mode));

LEAF_BRIDGE_API void leaf_bridge_set_player_xp_hooks(
    int (*get_fn)(
        uint64_t player_handle,
        int32_t* out_level,
        float* out_progress),
    int (*set_fn)(uint64_t player_handle, int32_t level));

LEAF_BRIDGE_API void leaf_bridge_set_player_look_hooks(
    int (*get_fn)(uint64_t player_handle, float* out_yaw, float* out_pitch),
    int (*set_fn)(uint64_t player_handle, float yaw, float pitch));

LEAF_BRIDGE_API void leaf_bridge_set_play_sound_hook(
    int (*fn)(
        uint64_t player_handle,
        const char* sound_id,
        float volume,
        float pitch,
        int32_t x,
        int32_t y,
        int32_t z,
        uint32_t dimension));

LEAF_BRIDGE_API void leaf_bridge_set_actionbar_hook(
    int (*fn)(uint64_t player_handle, const char* message));

LEAF_BRIDGE_API void leaf_bridge_set_title_hook(
    int (*fn)(
        uint64_t player_handle,
        const char* title,
        const char* subtitle,
        int32_t fade_in_ticks,
        int32_t stay_ticks,
        int32_t fade_out_ticks));

LEAF_BRIDGE_API void leaf_bridge_set_kick_player_hook(
    int (*fn)(uint64_t player_handle, const char* reason));

LEAF_BRIDGE_API void leaf_bridge_set_give_item_hook(
    int (*fn)(
        uint64_t player_handle,
        uint32_t item_id,
        uint32_t count,
        int32_t damage));

LEAF_BRIDGE_API void leaf_bridge_set_effect_hooks(
    int (*apply_fn)(
        uint64_t player_handle,
        const char* effect_id,
        int32_t duration_ticks,
        int32_t amplifier,
        uint32_t flags),
    int (*clear_fn)(uint64_t player_handle));

LEAF_BRIDGE_API void leaf_bridge_set_spawn_particle_hook(
    int (*fn)(
        const char* particle_id,
        double x,
        double y,
        double z,
        uint32_t dimension,
        uint32_t count,
        double dx,
        double dy,
        double dz,
        double speed));

LEAF_BRIDGE_API void leaf_bridge_set_world_time_hooks(
    int (*get_fn)(uint32_t dimension, int64_t* out_time),
    int (*set_fn)(uint32_t dimension, int64_t time));

LEAF_BRIDGE_API void leaf_bridge_set_player_velocity_hooks(
    int (*get_fn)(uint64_t player_handle, double* out_vx, double* out_vy, double* out_vz),
    int (*set_fn)(uint64_t player_handle, double vx, double vy, double vz));

LEAF_BRIDGE_API void leaf_bridge_set_player_flags_hook(
    int (*fn)(uint64_t player_handle, uint32_t* out_flags));

LEAF_BRIDGE_API void leaf_bridge_set_run_command_hook(
    int (*fn)(uint64_t player_handle, const char* command));

LEAF_BRIDGE_API void leaf_bridge_set_clear_inventory_hook(
    int (*fn)(uint64_t player_handle));

LEAF_BRIDGE_API void leaf_bridge_set_player_flight_hook(
    int (*fn)(uint64_t player_handle, int32_t allow_flight, int32_t flying));

LEAF_BRIDGE_API void leaf_bridge_set_get_biome_hook(
    int (*fn)(
        uint32_t dimension,
        int32_t x,
        int32_t y,
        int32_t z,
        char* out_buf,
        uint32_t out_buf_size));

LEAF_BRIDGE_API void leaf_bridge_set_difficulty_hooks(
    int (*get_fn)(uint32_t* out_difficulty),
    int (*set_fn)(uint32_t difficulty));

LEAF_BRIDGE_API void leaf_bridge_set_weather_hooks(
    int (*get_fn)(uint32_t dimension, uint32_t* out_weather),
    int (*set_fn)(uint32_t dimension, uint32_t weather, int32_t duration_ticks));

LEAF_BRIDGE_API void leaf_bridge_set_get_light_level_hook(
    int (*fn)(
        uint32_t dimension,
        int32_t x,
        int32_t y,
        int32_t z,
        uint32_t* out_block_light,
        uint32_t* out_sky_light));

LEAF_BRIDGE_API void leaf_bridge_set_player_latency_hook(
    int (*fn)(uint64_t player_handle, int32_t* out_ms));

LEAF_BRIDGE_API void leaf_bridge_set_world_spawn_hooks(
    int (*get_fn)(
        uint32_t dimension,
        int32_t* out_x,
        int32_t* out_y,
        int32_t* out_z),
    int (*set_fn)(uint32_t dimension, int32_t x, int32_t y, int32_t z));

LEAF_BRIDGE_API void leaf_bridge_set_player_op_hook(
    int (*fn)(uint64_t player_handle, int32_t* out_op));

LEAF_BRIDGE_API void leaf_bridge_set_player_uuid_hook(
    int (*fn)(uint64_t player_handle, char* out_buf, uint32_t out_buf_size));

LEAF_BRIDGE_API void leaf_bridge_set_player_permission_hook(
    int (*fn)(uint64_t player_handle, int32_t* out_level));

LEAF_BRIDGE_API void leaf_bridge_set_find_player_uuid_hook(
    int (*fn)(const char* uuid, uint64_t* out_player));

LEAF_BRIDGE_API void leaf_bridge_set_find_player_name_hook(
    int (*fn)(const char* name, uint64_t* out_player));

LEAF_BRIDGE_API void leaf_bridge_set_block_registry_hooks(
    int (*get_fn)(
        uint32_t dimension,
        int32_t x,
        int32_t y,
        int32_t z,
        char* out_buf,
        uint32_t out_buf_size),
    int (*set_fn)(
        uint32_t dimension,
        int32_t x,
        int32_t y,
        int32_t z,
        const char* block_id));

LEAF_BRIDGE_API void leaf_bridge_set_give_item_registry_hook(
    int (*fn)(
        uint64_t player_handle,
        const char* item_id,
        uint32_t count,
        int32_t damage));

LEAF_BRIDGE_API void leaf_bridge_set_inventory_registry_hooks(
    int (*get_fn)(
        uint64_t player_handle,
        uint32_t slot,
        char* out_buf,
        uint32_t out_buf_size,
        uint32_t* out_count,
        int32_t* out_damage),
    int (*set_fn)(
        uint64_t player_handle,
        uint32_t slot,
        const char* item_id,
        uint32_t count,
        int32_t damage));

LEAF_BRIDGE_API void leaf_bridge_set_teleport_hook(
    int (*fn)(
        uint64_t player_handle,
        int32_t x,
        int32_t y,
        int32_t z,
        uint32_t dimension,
        float yaw,
        float pitch));

LEAF_BRIDGE_API void leaf_bridge_set_selected_slot_hooks(
    int (*get_fn)(uint64_t player_handle, uint32_t* out_slot),
    int (*set_fn)(uint64_t player_handle, uint32_t slot));

LEAF_BRIDGE_API void leaf_bridge_set_get_world_seed_hook(
    int (*fn)(uint32_t dimension, int64_t* out_seed));

LEAF_BRIDGE_API void leaf_bridge_set_player_absorption_hooks(
    int (*get_fn)(uint64_t player_handle, float* out_absorption),
    int (*set_fn)(uint64_t player_handle, float absorption));

LEAF_BRIDGE_API void leaf_bridge_set_player_invulnerable_hooks(
    int (*get_fn)(uint64_t player_handle, int32_t* out_invulnerable),
    int (*set_fn)(uint64_t player_handle, int32_t invulnerable));

LEAF_BRIDGE_API void leaf_bridge_set_player_air_hooks(
    int (*get_fn)(uint64_t player_handle, int32_t* out_air),
    int (*set_fn)(uint64_t player_handle, int32_t air));

LEAF_BRIDGE_API void leaf_bridge_set_player_fire_ticks_hooks(
    int (*get_fn)(uint64_t player_handle, int32_t* out_ticks),
    int (*set_fn)(uint64_t player_handle, int32_t ticks));

LEAF_BRIDGE_API void leaf_bridge_set_player_frozen_ticks_hooks(
    int (*get_fn)(uint64_t player_handle, int32_t* out_ticks),
    int (*set_fn)(uint64_t player_handle, int32_t ticks));

LEAF_BRIDGE_API void leaf_bridge_set_player_no_gravity_hooks(
    int (*get_fn)(uint64_t player_handle, int32_t* out_no_gravity),
    int (*set_fn)(uint64_t player_handle, int32_t no_gravity));

LEAF_BRIDGE_API void leaf_bridge_set_player_silent_hooks(
    int (*get_fn)(uint64_t player_handle, int32_t* out_silent),
    int (*set_fn)(uint64_t player_handle, int32_t silent));

LEAF_BRIDGE_API void leaf_bridge_set_player_glowing_hooks(
    int (*get_fn)(uint64_t player_handle, int32_t* out_glowing),
    int (*set_fn)(uint64_t player_handle, int32_t glowing));

LEAF_BRIDGE_API void leaf_bridge_set_player_invisible_hooks(
    int (*get_fn)(uint64_t player_handle, int32_t* out_invisible),
    int (*set_fn)(uint64_t player_handle, int32_t invisible));

LEAF_BRIDGE_API void leaf_bridge_set_player_portal_cooldown_hooks(
    int (*get_fn)(uint64_t player_handle, int32_t* out_ticks),
    int (*set_fn)(uint64_t player_handle, int32_t ticks));

LEAF_BRIDGE_API void leaf_bridge_set_get_player_max_air_hook(
    int (*fn)(uint64_t player_handle, int32_t* out_max_air));

/* Register a live player name so get_player_name / player_count stay accurate. */
LEAF_BRIDGE_API leaf_error_code leaf_bridge_register_player(
    uint64_t player_handle,
    const char* name);

LEAF_BRIDGE_API leaf_error_code leaf_bridge_unregister_player(
    uint64_t player_handle);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* LEAF_BRIDGE_V1_H */
