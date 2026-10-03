/* Leaf C ABI v1 — stable binary boundary between LEAFMC and Leaf Mods.
 *
 * Rules:
 *  - Pure C types only (no C++ objects, no exceptions, no STL).
 *  - Additive evolution only within v1 (append fields / functions).
 *  - Breaking changes require LeafApiV2.
 *  - Leaf Mods (C++23 or Kotlin/Native) talk to the engine exclusively
 *    through this ABI (or a thin language SDK wrapping it).
 */

#ifndef LEAF_ABI_V1_H
#define LEAF_ABI_V1_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32) || defined(__CYGWIN__)
#  if defined(LEAF_ENGINE_BUILD)
#    define LEAF_ABI_API __declspec(dllexport)
#  else
#    define LEAF_ABI_API __declspec(dllimport)
#  endif
#else
#  define LEAF_ABI_API __attribute__((visibility("default")))
#endif

#define LEAF_ABI_VERSION_MAJOR 1u
#define LEAF_ABI_VERSION_MINOR 58u

/* get_player_flags bits */
#define LEAF_PLAYER_FLAG_SNEAKING      1u
#define LEAF_PLAYER_FLAG_SPRINTING     2u
#define LEAF_PLAYER_FLAG_SWIMMING      4u
#define LEAF_PLAYER_FLAG_FLYING        8u
#define LEAF_PLAYER_FLAG_ON_GROUND     16u
#define LEAF_PLAYER_FLAG_ALLOW_FLIGHT  32u

/* Weather */
#define LEAF_WEATHER_CLEAR    0u
#define LEAF_WEATHER_RAIN     1u
#define LEAF_WEATHER_THUNDER  2u

/* Equipment slots for get/set_equipment_item_registry_id */
#define LEAF_EQUIP_MAINHAND  0u /* selected hotbar stack */
#define LEAF_EQUIP_OFFHAND   1u /* inventory slot 40 */
#define LEAF_EQUIP_FEET      2u /* inventory slot 36 */
#define LEAF_EQUIP_LEGS      3u /* inventory slot 37 */
#define LEAF_EQUIP_CHEST     4u /* inventory slot 38 */
#define LEAF_EQUIP_HEAD      5u /* inventory slot 39 */

typedef uint64_t LeafHandle;

enum LeafLogLevel {
    LEAF_LOG_TRACE = 0,
    LEAF_LOG_DEBUG = 1,
    LEAF_LOG_INFO = 2,
    LEAF_LOG_WARN = 3,
    LEAF_LOG_ERROR = 4,
    LEAF_LOG_FATAL = 5
};

enum LeafStatus {
    LEAF_STATUS_OK = 0,
    LEAF_STATUS_ERROR = 1,
    LEAF_STATUS_INVALID_ARGUMENT = 2,
    LEAF_STATUS_NOT_FOUND = 3,
    LEAF_STATUS_NOT_SUPPORTED = 4,
    LEAF_STATUS_WRONG_THREAD = 5,
    LEAF_STATUS_INVALID_HANDLE = 6,
    LEAF_STATUS_STALE_HANDLE = 7,
    LEAF_STATUS_CAPABILITY_MISSING = 8,
    LEAF_STATUS_ABI_MISMATCH = 9
};

enum LeafCapability {
    LEAF_CAP_DATA_COMPONENTS = 1,
    LEAF_CAP_LEGACY_ITEM_NBT = 2,
    LEAF_CAP_CUSTOM_PAYLOAD_V2 = 3,
    LEAF_CAP_REGISTRY_MODERN = 4,
    LEAF_CAP_REGISTRY_LEGACY = 5,
    LEAF_CAP_CLIENT_RENDER_V2 = 6,
    LEAF_CAP_SERVER_TRANSFER = 7,
    LEAF_CAP_INVENTORY_STACK_V1 = 8
};

/* Event IDs live in leaf_event_v1.h (LeafCoreEventId). */

typedef void (*LeafEventCallback)(const void* event, void* user_data);

/* Returns LeafDecision (LEAF_DECISION_*). Used with subscribe_decision. */
typedef int (*LeafDecisionCallback)(const void* event, void* user_data);

/* Scheduler task callback (main-thread unless noted). */
typedef void (*LeafTaskCallback)(void* user_data);

typedef struct LeafPlayerJoinEventV1 {
    uint32_t struct_size; /* sizeof(LeafPlayerJoinEventV1) */
    LeafHandle player;
} LeafPlayerJoinEventV1;

typedef struct LeafItemStackV1 {
    uint32_t struct_size; /* sizeof(LeafItemStackV1) */
    uint32_t item_id;     /* 0 = empty */
    uint32_t count;
    int32_t damage;       /* -1 if unset / not damageable */
} LeafItemStackV1;

typedef struct LeafApiV1 {
    uint32_t abi_major;
    uint32_t abi_minor;
    uint32_t struct_size;

    void (*log)(int level, const char* message);

    int (*has_capability)(uint32_t capability);

    LeafHandle (*get_server)(void);

    LeafStatus (*send_player_message)(LeafHandle player, const char* message);

    LeafStatus (*broadcast_message)(const char* message);

    uint32_t (*player_count)(void);

    /* Index into the current online player list (0 .. player_count-1). */
    LeafStatus (*get_player_at)(uint32_t index, LeafHandle* out_player);

    /* Writes UTF-8 name into out_buf (NUL-terminated). Truncates if needed. */
    LeafStatus (*get_player_name)(
        LeafHandle player,
        char* out_buf,
        uint32_t out_buf_size);

    /* Inventory / world — stub backends return NOT_SUPPORTED until live. */
    LeafStatus (*get_inventory_slot)(
        LeafHandle player,
        uint32_t slot,
        uint32_t* out_item_id,
        uint32_t* out_count);

    LeafStatus (*set_inventory_slot)(
        LeafHandle player,
        uint32_t slot,
        uint32_t item_id,
        uint32_t count);

    /* Richer stack (damage). Prefer over get/set_inventory_slot when available. */
    LeafStatus (*get_inventory_stack)(
        LeafHandle player,
        uint32_t slot,
        LeafItemStackV1* out_stack);

    LeafStatus (*set_inventory_stack)(
        LeafHandle player,
        uint32_t slot,
        const LeafItemStackV1* stack);

    LeafStatus (*get_block)(
        int32_t x,
        int32_t y,
        int32_t z,
        uint32_t* out_block_id);

    LeafStatus (*set_block)(
        int32_t x,
        int32_t y,
        int32_t z,
        uint32_t block_id);

    /* Dimension 0 = overworld, 1 = nether, 2 = end (Minecraft convention). */
    LeafStatus (*get_block_dim)(
        uint32_t dimension,
        int32_t x,
        int32_t y,
        int32_t z,
        uint32_t* out_block_id);

    LeafStatus (*set_block_dim)(
        uint32_t dimension,
        int32_t x,
        int32_t y,
        int32_t z,
        uint32_t block_id);

    /* Block-rounded position + dimension (0/1/2). */
    LeafStatus (*get_player_pos)(
        LeafHandle player,
        int32_t* out_x,
        int32_t* out_y,
        int32_t* out_z,
        uint32_t* out_dimension);

    LeafStatus (*set_player_pos)(
        LeafHandle player,
        int32_t x,
        int32_t y,
        int32_t z,
        uint32_t dimension);

    LeafStatus (*get_player_health)(
        LeafHandle player,
        float* out_health,
        float* out_max_health);

    LeafStatus (*set_player_health)(LeafHandle player, float health);

    /* Food: level 0–20, saturation float. */
    LeafStatus (*get_player_food)(
        LeafHandle player,
        int32_t* out_food,
        float* out_saturation);

    LeafStatus (*set_player_food)(
        LeafHandle player,
        int32_t food,
        float saturation);

    /* Game mode: 0 survival, 1 creative, 2 adventure, 3 spectator. */
    LeafStatus (*get_player_gamemode)(LeafHandle player, uint32_t* out_mode);

    LeafStatus (*set_player_gamemode)(LeafHandle player, uint32_t mode);

    /* XP: level + progress in [0,1). */
    LeafStatus (*get_player_xp)(
        LeafHandle player,
        int32_t* out_level,
        float* out_progress);

    LeafStatus (*set_player_xp_level)(LeafHandle player, int32_t level);

    /* Look: yaw/pitch in degrees (Minecraft convention). */
    LeafStatus (*get_player_look)(
        LeafHandle player,
        float* out_yaw,
        float* out_pitch);

    LeafStatus (*set_player_look)(
        LeafHandle player,
        float yaw,
        float pitch);

    /* Play a registry sound. player=0 plays at (x,y,z,dim) for nearby clients;
       player!=0 plays for that player (coords may be ignored by live hooks). */
    LeafStatus (*play_sound)(
        LeafHandle player,
        const char* sound_id,
        float volume,
        float pitch,
        int32_t x,
        int32_t y,
        int32_t z,
        uint32_t dimension);

    /* HUD overlays (action bar / title screen). */
    LeafStatus (*send_actionbar)(LeafHandle player, const char* message);

    LeafStatus (*send_title)(
        LeafHandle player,
        const char* title,
        const char* subtitle,
        int32_t fade_in_ticks,
        int32_t stay_ticks,
        int32_t fade_out_ticks);

    /* Disconnect a player with an optional reason (UTF-8). */
    LeafStatus (*kick_player)(LeafHandle player, const char* reason);

    /* Insert item into the first suitable inventory slot (merge or empty). */
    LeafStatus (*give_item)(
        LeafHandle player,
        uint32_t item_id,
        uint32_t count,
        int32_t damage);

    /* Potion effects. flags: bit0=ambient, bit1=show_particles, bit2=show_icon. */
    LeafStatus (*apply_effect)(
        LeafHandle player,
        const char* effect_id,
        int32_t duration_ticks,
        int32_t amplifier,
        uint32_t flags);

    LeafStatus (*clear_effects)(LeafHandle player);

    /* Spawn simple (parameterless) registry particles in a dimension. */
    LeafStatus (*spawn_particle)(
        const char* particle_id,
        double x,
        double y,
        double z,
        uint32_t dimension,
        uint32_t count,
        double dx,
        double dy,
        double dz,
        double speed);

    /* World day time in ticks (0–24000). dimension: 0 overworld, 1 nether, 2 end. */
    LeafStatus (*get_world_time)(uint32_t dimension, int64_t* out_time);

    LeafStatus (*set_world_time)(uint32_t dimension, int64_t time);

    /* Player velocity in blocks/tick. */
    LeafStatus (*get_player_velocity)(
        LeafHandle player,
        double* out_vx,
        double* out_vy,
        double* out_vz);

    LeafStatus (*set_player_velocity)(
        LeafHandle player,
        double vx,
        double vy,
        double vz);

    /* Movement/state flags — see LEAF_PLAYER_FLAG_*. */
    LeafStatus (*get_player_flags)(LeafHandle player, uint32_t* out_flags);

    /* Run a command. player=0 uses the server/console source. */
    LeafStatus (*run_command)(LeafHandle player, const char* command);

    LeafStatus (*subscribe_event)(
        uint32_t event_id,
        LeafEventCallback callback,
        void* user_data,
        uint64_t* out_subscription_id);

    LeafStatus (*unsubscribe_event)(uint64_t subscription_id);

    /* Decision events: callback returns LEAF_DECISION_*. */
    LeafStatus (*subscribe_decision)(
        uint32_t event_id,
        LeafDecisionCallback callback,
        void* user_data,
        uint64_t* out_subscription_id);

    /* Queue work on the Minecraft / LEAF main thread (runs on next pump_main). */
    LeafStatus (*post_main)(LeafTaskCallback callback, void* user_data);

    /* One-shot delayed main-thread task. delay_ms from now. */
    LeafStatus (*delay_main)(
        LeafTaskCallback callback,
        void* user_data,
        uint32_t delay_ms,
        uint64_t* out_task_id);

    LeafStatus (*cancel_task)(uint64_t task_id);

    /* Clear all player inventory slots (including armor / offhand). */
    LeafStatus (*clear_inventory)(LeafHandle player);

    /* Broadcast HUD overlays to every online player. */
    LeafStatus (*broadcast_actionbar)(const char* message);

    LeafStatus (*broadcast_title)(
        const char* title,
        const char* subtitle,
        int32_t fade_in_ticks,
        int32_t stay_ticks,
        int32_t fade_out_ticks);

    /* Flight: allow_flight/flying use 0=off, 1=on, -1=unchanged. */
    LeafStatus (*set_player_flight)(
        LeafHandle player,
        int32_t allow_flight,
        int32_t flying);

    /* Biome registry id at block coords (e.g. "minecraft:plains"). */
    LeafStatus (*get_biome)(
        uint32_t dimension,
        int32_t x,
        int32_t y,
        int32_t z,
        char* out_buf,
        uint32_t out_buf_size);

    /* World difficulty: 0 peaceful, 1 easy, 2 normal, 3 hard. */
    LeafStatus (*get_difficulty)(uint32_t* out_difficulty);

    LeafStatus (*set_difficulty)(uint32_t difficulty);

    /* Weather in a dimension (typically overworld). See LEAF_WEATHER_*.
       duration_ticks: 0 = engine default length. */
    LeafStatus (*get_weather)(uint32_t dimension, uint32_t* out_weather);

    LeafStatus (*set_weather)(
        uint32_t dimension,
        uint32_t weather,
        int32_t duration_ticks);

    /* Block / sky light levels at coords (0–15 each). */
    LeafStatus (*get_light_level)(
        uint32_t dimension,
        int32_t x,
        int32_t y,
        int32_t z,
        uint32_t* out_block_light,
        uint32_t* out_sky_light);

    /* Player network latency in milliseconds. */
    LeafStatus (*get_player_latency)(LeafHandle player, int32_t* out_ms);

    /* World spawn point (block coords) per dimension. */
    LeafStatus (*get_world_spawn)(
        uint32_t dimension,
        int32_t* out_x,
        int32_t* out_y,
        int32_t* out_z);

    LeafStatus (*set_world_spawn)(
        uint32_t dimension,
        int32_t x,
        int32_t y,
        int32_t z);

    /* Operator / permission level: out_op 1 if OP, else 0. */
    LeafStatus (*is_player_op)(LeafHandle player, int32_t* out_op);

    /* Player UUID as canonical lowercase hyphenated string (36 chars + NUL). */
    LeafStatus (*get_player_uuid)(
        LeafHandle player,
        char* out_buf,
        uint32_t out_buf_size);

    /* Vanilla permission level 0–4 (0 = none, 4 = owner). */
    LeafStatus (*get_player_permission_level)(
        LeafHandle player,
        int32_t* out_level);

    /* Resolve an online player handle from a hyphenated UUID string. */
    LeafStatus (*find_player_by_uuid)(const char* uuid, LeafHandle* out_player);

    /* Resolve an online player handle from exact display/login name. */
    LeafStatus (*find_player_by_name)(const char* name, LeafHandle* out_player);

    /* Block registry id string (e.g. "minecraft:stone") at coords. */
    LeafStatus (*get_block_registry_id)(
        uint32_t dimension,
        int32_t x,
        int32_t y,
        int32_t z,
        char* out_buf,
        uint32_t out_buf_size);

    LeafStatus (*set_block_registry_id)(
        uint32_t dimension,
        int32_t x,
        int32_t y,
        int32_t z,
        const char* block_id);

    /* Give an item by registry id (e.g. "minecraft:diamond"). */
    LeafStatus (*give_item_registry_id)(
        LeafHandle player,
        const char* item_id,
        uint32_t count,
        int32_t damage);

    /* Inventory slot as registry id + count + damage (-1 if undamaged). */
    LeafStatus (*get_inventory_item_registry_id)(
        LeafHandle player,
        uint32_t slot,
        char* out_buf,
        uint32_t out_buf_size,
        uint32_t* out_count,
        int32_t* out_damage);

    LeafStatus (*set_inventory_item_registry_id)(
        LeafHandle player,
        uint32_t slot,
        const char* item_id,
        uint32_t count,
        int32_t damage);

    /* Teleport: block pos + dimension + look angles in one call. */
    LeafStatus (*teleport_player)(
        LeafHandle player,
        int32_t x,
        int32_t y,
        int32_t z,
        uint32_t dimension,
        float yaw,
        float pitch);

    /* Selected hotbar slot (0–8). */
    LeafStatus (*get_selected_slot)(LeafHandle player, uint32_t* out_slot);

    LeafStatus (*set_selected_slot)(LeafHandle player, uint32_t slot);

    /* Held (selected hotbar) item — convenience over selected_slot + inventory. */
    LeafStatus (*get_held_item_registry_id)(
        LeafHandle player,
        char* out_buf,
        uint32_t out_buf_size,
        uint32_t* out_count,
        int32_t* out_damage);

    LeafStatus (*set_held_item_registry_id)(
        LeafHandle player,
        const char* item_id,
        uint32_t count,
        int32_t damage);

    /* Equipment (mainhand/offhand/armor). See LEAF_EQUIP_*. */
    LeafStatus (*get_equipment_item_registry_id)(
        LeafHandle player,
        uint32_t equip_slot,
        char* out_buf,
        uint32_t out_buf_size,
        uint32_t* out_count,
        int32_t* out_damage);

    LeafStatus (*set_equipment_item_registry_id)(
        LeafHandle player,
        uint32_t equip_slot,
        const char* item_id,
        uint32_t count,
        int32_t damage);

    /* World generation seed for a dimension (typically same across dims). */
    LeafStatus (*get_world_seed)(uint32_t dimension, int64_t* out_seed);

    /* Restore health to max (composes get/set_player_health). */
    LeafStatus (*heal_player)(LeafHandle player);

    /* Absorption hearts (gold hearts overlay). */
    LeafStatus (*get_player_absorption)(LeafHandle player, float* out_absorption);

    LeafStatus (*set_player_absorption)(LeafHandle player, float absorption);

    /* Invulnerability (creative-like damage immunity flag). 0/1. */
    LeafStatus (*get_player_invulnerable)(LeafHandle player, int32_t* out_invulnerable);

    LeafStatus (*set_player_invulnerable)(LeafHandle player, int32_t invulnerable);

    /* Remaining air ticks while underwater (vanilla max typically 300). */
    LeafStatus (*get_player_air)(LeafHandle player, int32_t* out_air);

    LeafStatus (*set_player_air)(LeafHandle player, int32_t air);

    /* Remaining on-fire ticks (0 = not on fire). */
    LeafStatus (*get_player_fire_ticks)(LeafHandle player, int32_t* out_ticks);

    LeafStatus (*set_player_fire_ticks)(LeafHandle player, int32_t ticks);

    /* Powder-snow freeze ticks (0 = not frozen). */
    LeafStatus (*get_player_frozen_ticks)(LeafHandle player, int32_t* out_ticks);

    LeafStatus (*set_player_frozen_ticks)(LeafHandle player, int32_t ticks);

    /* Clear fire / freeze (compose set_*_ticks to 0). */
    LeafStatus (*extinguish_player)(LeafHandle player);

    LeafStatus (*unfreeze_player)(LeafHandle player);

    /* Entity no-gravity flag (0/1; Entity.isNoGravity / setNoGravity). */
    LeafStatus (*get_player_no_gravity)(LeafHandle player, int32_t* out_no_gravity);

    LeafStatus (*set_player_no_gravity)(LeafHandle player, int32_t no_gravity);

    /* Entity silent flag (0/1; Entity.isSilent / setSilent). */
    LeafStatus (*get_player_silent)(LeafHandle player, int32_t* out_silent);

    LeafStatus (*set_player_silent)(LeafHandle player, int32_t silent);

    /* Entity glowing flag (0/1; yarn isGlowing/setGlowing;
     * mojmap isCurrentlyGlowing/setGlowingTag). */
    LeafStatus (*get_player_glowing)(LeafHandle player, int32_t* out_glowing);

    LeafStatus (*set_player_glowing)(LeafHandle player, int32_t glowing);

    /* Entity invisible flag (0/1; Entity.isInvisible / setInvisible). */
    LeafStatus (*get_player_invisible)(LeafHandle player, int32_t* out_invisible);

    LeafStatus (*set_player_invisible)(LeafHandle player, int32_t invisible);

    /* Entity portal cooldown ticks (Entity.getPortalCooldown / setPortalCooldown). */
    LeafStatus (*get_player_portal_cooldown)(LeafHandle player, int32_t* out_ticks);

    LeafStatus (*set_player_portal_cooldown)(LeafHandle player, int32_t ticks);

    /* Maximum air ticks (Entity.getMaxAir / getMaxAirSupply; typically 300). */
    LeafStatus (*get_player_max_air)(LeafHandle player, int32_t* out_max_air);

    /* Restore air to max (composes get_player_max_air + set_player_air). */
    LeafStatus (*refill_player_air)(LeafHandle player);

    /* Alive if health > 0 (composes get_player_health). out_alive is 0/1. */
    LeafStatus (*is_player_alive)(LeafHandle player, int32_t* out_alive);
} LeafApiV1;

/* Mod entry exported by every .leafmod native library. */
typedef struct LeafModInfoV1 {
    uint32_t struct_size;
    const char* id;
    const char* name;
    const char* version;
} LeafModInfoV1;

typedef struct LeafModExportsV1 {
    uint32_t struct_size;
    void (*on_load)(const LeafApiV1* api);
    void (*on_enable)(const LeafApiV1* api);
    void (*on_disable)(const LeafApiV1* api);
    void (*on_unload)(const LeafApiV1* api);
} LeafModExportsV1;

/* Signature of the required exported symbol `leaf_mod_entry`. */
typedef LeafStatus (*LeafModEntryFn)(
    const LeafApiV1* api,
    LeafModInfoV1* out_info,
    LeafModExportsV1* out_exports);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* LEAF_ABI_V1_H */
