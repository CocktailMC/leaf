#include "leaf/loader/runtime_api.hpp"

#include "leaf/abi/leaf_event_v1.h"
#include "leaf/core/abi_status.hpp"
#include "leaf/events/event_runtime.hpp"
#include "leaf/events/event_types.hpp"
#include "leaf/minecraft/minecraft_abi.hpp"
#include "leaf/object/handle.hpp"
#include "leaf/scheduler/scheduler.hpp"

#include <chrono>
#include <iostream>

namespace leaf {

runtime_api* runtime_api::active_ = nullptr;

runtime_api::runtime_api(capability_set capabilities)
    : capabilities_(std::move(capabilities)) {
    active_ = this;

    api_.abi_major = LEAF_ABI_VERSION_MAJOR;
    api_.abi_minor = LEAF_ABI_VERSION_MINOR;
    api_.struct_size = static_cast<uint32_t>(sizeof(LeafApiV1));
    api_.log = &runtime_api::abi_log;
    api_.has_capability = &runtime_api::abi_has_capability;
    api_.get_server = &runtime_api::abi_get_server;
    api_.send_player_message = &runtime_api::abi_send_player_message;
    api_.broadcast_message = &runtime_api::abi_broadcast_message;
    api_.player_count = &runtime_api::abi_player_count;
    api_.get_player_at = &runtime_api::abi_get_player_at;
    api_.get_player_name = &runtime_api::abi_get_player_name;
    api_.get_inventory_slot = &runtime_api::abi_get_inventory_slot;
    api_.set_inventory_slot = &runtime_api::abi_set_inventory_slot;
    api_.get_inventory_stack = &runtime_api::abi_get_inventory_stack;
    api_.set_inventory_stack = &runtime_api::abi_set_inventory_stack;
    api_.get_block = &runtime_api::abi_get_block;
    api_.set_block = &runtime_api::abi_set_block;
    api_.get_block_dim = &runtime_api::abi_get_block_dim;
    api_.set_block_dim = &runtime_api::abi_set_block_dim;
    api_.get_player_pos = &runtime_api::abi_get_player_pos;
    api_.set_player_pos = &runtime_api::abi_set_player_pos;
    api_.get_player_health = &runtime_api::abi_get_player_health;
    api_.set_player_health = &runtime_api::abi_set_player_health;
    api_.get_player_food = &runtime_api::abi_get_player_food;
    api_.set_player_food = &runtime_api::abi_set_player_food;
    api_.get_player_gamemode = &runtime_api::abi_get_player_gamemode;
    api_.set_player_gamemode = &runtime_api::abi_set_player_gamemode;
    api_.get_player_xp = &runtime_api::abi_get_player_xp;
    api_.set_player_xp_level = &runtime_api::abi_set_player_xp_level;
    api_.get_player_look = &runtime_api::abi_get_player_look;
    api_.set_player_look = &runtime_api::abi_set_player_look;
    api_.play_sound = &runtime_api::abi_play_sound;
    api_.send_actionbar = &runtime_api::abi_send_actionbar;
    api_.send_title = &runtime_api::abi_send_title;
    api_.kick_player = &runtime_api::abi_kick_player;
    api_.give_item = &runtime_api::abi_give_item;
    api_.apply_effect = &runtime_api::abi_apply_effect;
    api_.clear_effects = &runtime_api::abi_clear_effects;
    api_.spawn_particle = &runtime_api::abi_spawn_particle;
    api_.get_world_time = &runtime_api::abi_get_world_time;
    api_.set_world_time = &runtime_api::abi_set_world_time;
    api_.get_player_velocity = &runtime_api::abi_get_player_velocity;
    api_.set_player_velocity = &runtime_api::abi_set_player_velocity;
    api_.get_player_flags = &runtime_api::abi_get_player_flags;
    api_.run_command = &runtime_api::abi_run_command;
    api_.subscribe_event = &runtime_api::abi_subscribe_event;
    api_.unsubscribe_event = &runtime_api::abi_unsubscribe_event;
    api_.subscribe_decision = &runtime_api::abi_subscribe_decision;
    api_.post_main = &runtime_api::abi_post_main;
    api_.delay_main = &runtime_api::abi_delay_main;
    api_.cancel_task = &runtime_api::abi_cancel_task;
    api_.clear_inventory = &runtime_api::abi_clear_inventory;
    api_.broadcast_actionbar = &runtime_api::abi_broadcast_actionbar;
    api_.broadcast_title = &runtime_api::abi_broadcast_title;
    api_.set_player_flight = &runtime_api::abi_set_player_flight;
    api_.get_biome = &runtime_api::abi_get_biome;
    api_.get_difficulty = &runtime_api::abi_get_difficulty;
    api_.set_difficulty = &runtime_api::abi_set_difficulty;
    api_.get_weather = &runtime_api::abi_get_weather;
    api_.set_weather = &runtime_api::abi_set_weather;
    api_.get_light_level = &runtime_api::abi_get_light_level;
    api_.get_player_latency = &runtime_api::abi_get_player_latency;
    api_.get_world_spawn = &runtime_api::abi_get_world_spawn;
    api_.set_world_spawn = &runtime_api::abi_set_world_spawn;
    api_.is_player_op = &runtime_api::abi_is_player_op;
    api_.get_player_uuid = &runtime_api::abi_get_player_uuid;
    api_.get_player_permission_level = &runtime_api::abi_get_player_permission_level;
    api_.find_player_by_uuid = &runtime_api::abi_find_player_by_uuid;
    api_.find_player_by_name = &runtime_api::abi_find_player_by_name;
    api_.get_block_registry_id = &runtime_api::abi_get_block_registry_id;
    api_.set_block_registry_id = &runtime_api::abi_set_block_registry_id;
    api_.give_item_registry_id = &runtime_api::abi_give_item_registry_id;
    api_.get_inventory_item_registry_id =
        &runtime_api::abi_get_inventory_item_registry_id;
    api_.set_inventory_item_registry_id =
        &runtime_api::abi_set_inventory_item_registry_id;
    api_.teleport_player = &runtime_api::abi_teleport_player;
    api_.get_selected_slot = &runtime_api::abi_get_selected_slot;
    api_.set_selected_slot = &runtime_api::abi_set_selected_slot;
    api_.get_held_item_registry_id = &runtime_api::abi_get_held_item_registry_id;
    api_.set_held_item_registry_id = &runtime_api::abi_set_held_item_registry_id;
    api_.get_equipment_item_registry_id =
        &runtime_api::abi_get_equipment_item_registry_id;
    api_.set_equipment_item_registry_id =
        &runtime_api::abi_set_equipment_item_registry_id;
    api_.get_world_seed = &runtime_api::abi_get_world_seed;
    api_.heal_player = &runtime_api::abi_heal_player;
    api_.get_player_absorption = &runtime_api::abi_get_player_absorption;
    api_.set_player_absorption = &runtime_api::abi_set_player_absorption;
    api_.get_player_invulnerable = &runtime_api::abi_get_player_invulnerable;
    api_.set_player_invulnerable = &runtime_api::abi_set_player_invulnerable;
    api_.get_player_air = &runtime_api::abi_get_player_air;
    api_.set_player_air = &runtime_api::abi_set_player_air;
    api_.get_player_fire_ticks = &runtime_api::abi_get_player_fire_ticks;
    api_.set_player_fire_ticks = &runtime_api::abi_set_player_fire_ticks;
    api_.get_player_frozen_ticks = &runtime_api::abi_get_player_frozen_ticks;
    api_.set_player_frozen_ticks = &runtime_api::abi_set_player_frozen_ticks;
    api_.extinguish_player = &runtime_api::abi_extinguish_player;
    api_.unfreeze_player = &runtime_api::abi_unfreeze_player;
    api_.get_player_no_gravity = &runtime_api::abi_get_player_no_gravity;
    api_.set_player_no_gravity = &runtime_api::abi_set_player_no_gravity;
    api_.get_player_silent = &runtime_api::abi_get_player_silent;
    api_.set_player_silent = &runtime_api::abi_set_player_silent;
    api_.get_player_glowing = &runtime_api::abi_get_player_glowing;
    api_.set_player_glowing = &runtime_api::abi_set_player_glowing;
    api_.get_player_invisible = &runtime_api::abi_get_player_invisible;
    api_.set_player_invisible = &runtime_api::abi_set_player_invisible;
    api_.get_player_portal_cooldown = &runtime_api::abi_get_player_portal_cooldown;
    api_.set_player_portal_cooldown = &runtime_api::abi_set_player_portal_cooldown;
    api_.get_player_max_air = &runtime_api::abi_get_player_max_air;
    api_.refill_player_air = &runtime_api::abi_refill_player_air;
    api_.is_player_alive = &runtime_api::abi_is_player_alive;

    log_sink_ = [](int level, std::string_view message) {
        const char* tag = "INFO";
        switch (level) {
            case LEAF_LOG_TRACE: tag = "TRACE"; break;
            case LEAF_LOG_DEBUG: tag = "DEBUG"; break;
            case LEAF_LOG_INFO: tag = "INFO"; break;
            case LEAF_LOG_WARN: tag = "WARN"; break;
            case LEAF_LOG_ERROR: tag = "ERROR"; break;
            case LEAF_LOG_FATAL: tag = "FATAL"; break;
            default: break;
        }
        std::cerr << "[LEAF][" << tag << "] " << message << '\n';
    };
}

void runtime_api::set_log_sink(log_sink sink) {
    std::scoped_lock lock(mutex_);
    if (sink) {
        log_sink_ = std::move(sink);
    }
}

void runtime_api::set_capabilities(capability_set capabilities) {
    std::scoped_lock lock(mutex_);
    capabilities_ = std::move(capabilities);
}

void runtime_api::attach_events(event_runtime* runtime, mod_id default_owner) {
    std::scoped_lock lock(mutex_);
    events_ = runtime;
    default_owner_ = default_owner;
}

void runtime_api::abi_log(int level, const char* message) {
    if (!active_ || !message) {
        return;
    }
    std::scoped_lock lock(active_->mutex_);
    if (active_->log_sink_) {
        active_->log_sink_(level, message);
    }
}

int runtime_api::abi_has_capability(uint32_t capability) {
    if (!active_) {
        return 0;
    }
    std::scoped_lock lock(active_->mutex_);
    return active_->capabilities_.has(static_cast<leaf::capability>(capability))
        ? 1
        : 0;
}

LeafHandle runtime_api::abi_get_server() {
    if (!active_ || !active_->minecraft_) {
        return 0;
    }
    auto server = active_->minecraft_->get_server();
    return server ? server->raw() : 0;
}

LeafStatus runtime_api::abi_send_player_message(
    LeafHandle player,
    const char* message) {
    if (!active_ || !active_->minecraft_ || !message) {
        return LEAF_STATUS_NOT_SUPPORTED;
    }
    auto st = active_->minecraft_->send_player_message(
        player_handle{player},
        message);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_broadcast_message(const char* message) {
    if (!active_ || !active_->minecraft_ || !message) {
        return LEAF_STATUS_NOT_SUPPORTED;
    }
    auto st = active_->minecraft_->broadcast_message(message);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

uint32_t runtime_api::abi_player_count() {
    if (!active_ || !active_->minecraft_) {
        return 0;
    }
    return active_->minecraft_->player_count();
}

LeafStatus runtime_api::abi_get_player_at(uint32_t index, LeafHandle* out_player) {
    if (!active_ || !active_->minecraft_ || !out_player) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    player_handle handle{};
    auto st = active_->minecraft_->get_player_at(index, handle);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    *out_player = handle.raw();
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_get_player_name(
    LeafHandle player,
    char* out_buf,
    uint32_t out_buf_size) {
    if (!active_ || !active_->minecraft_ || !out_buf || out_buf_size == 0) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    auto name = active_->minecraft_->player_name(player_handle{player});
    if (!name) {
        return to_abi_status(name.error().code());
    }
    const auto& s = *name;
    const auto copy = s.size() < (out_buf_size - 1) ? s.size() : (out_buf_size - 1);
    for (std::size_t i = 0; i < copy; ++i) {
        out_buf[i] = s[i];
    }
    out_buf[copy] = '\0';
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_get_inventory_slot(
    LeafHandle player,
    uint32_t slot,
    uint32_t* out_item_id,
    uint32_t* out_count) {
    if (!active_ || !active_->minecraft_ || !out_item_id || !out_count) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    auto st = active_->minecraft_->get_inventory_slot(
        player_handle{player},
        slot,
        *out_item_id,
        *out_count);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_set_inventory_slot(
    LeafHandle player,
    uint32_t slot,
    uint32_t item_id,
    uint32_t count) {
    if (!active_ || !active_->minecraft_) {
        return LEAF_STATUS_NOT_SUPPORTED;
    }
    auto st = active_->minecraft_->set_inventory_slot(
        player_handle{player},
        slot,
        item_id,
        count);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_get_inventory_stack(
    LeafHandle player,
    uint32_t slot,
    LeafItemStackV1* out_stack) {
    if (!active_ || !active_->minecraft_ || !out_stack) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    LeafItemStackV1 stack{};
    auto st = active_->minecraft_->get_inventory_stack(
        player_handle{player},
        slot,
        stack);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    *out_stack = stack;
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_set_inventory_stack(
    LeafHandle player,
    uint32_t slot,
    const LeafItemStackV1* stack) {
    if (!active_ || !active_->minecraft_ || !stack) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    auto st = active_->minecraft_->set_inventory_stack(
        player_handle{player},
        slot,
        *stack);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_get_block(
    int32_t x,
    int32_t y,
    int32_t z,
    uint32_t* out_block_id) {
    if (!active_ || !active_->minecraft_ || !out_block_id) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    auto st = active_->minecraft_->get_block(x, y, z, *out_block_id);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_set_block(
    int32_t x,
    int32_t y,
    int32_t z,
    uint32_t block_id) {
    if (!active_ || !active_->minecraft_) {
        return LEAF_STATUS_NOT_SUPPORTED;
    }
    auto st = active_->minecraft_->set_block(x, y, z, block_id);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_get_block_dim(
    uint32_t dimension,
    int32_t x,
    int32_t y,
    int32_t z,
    uint32_t* out_block_id) {
    if (!active_ || !active_->minecraft_ || !out_block_id) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    auto st = active_->minecraft_->get_block_dim(
        dimension, x, y, z, *out_block_id);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_set_block_dim(
    uint32_t dimension,
    int32_t x,
    int32_t y,
    int32_t z,
    uint32_t block_id) {
    if (!active_ || !active_->minecraft_) {
        return LEAF_STATUS_NOT_SUPPORTED;
    }
    auto st = active_->minecraft_->set_block_dim(dimension, x, y, z, block_id);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_get_player_pos(
    LeafHandle player,
    int32_t* out_x,
    int32_t* out_y,
    int32_t* out_z,
    uint32_t* out_dimension) {
    if (!active_ || !active_->minecraft_ || !out_x || !out_y || !out_z
        || !out_dimension) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    auto st = active_->minecraft_->get_player_pos(
        player_handle{player},
        *out_x,
        *out_y,
        *out_z,
        *out_dimension);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_set_player_pos(
    LeafHandle player,
    int32_t x,
    int32_t y,
    int32_t z,
    uint32_t dimension) {
    if (!active_ || !active_->minecraft_) {
        return LEAF_STATUS_NOT_SUPPORTED;
    }
    auto st = active_->minecraft_->set_player_pos(
        player_handle{player},
        x,
        y,
        z,
        dimension);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_get_player_health(
    LeafHandle player,
    float* out_health,
    float* out_max_health) {
    if (!active_ || !active_->minecraft_ || !out_health || !out_max_health) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    auto st = active_->minecraft_->get_player_health(
        player_handle{player},
        *out_health,
        *out_max_health);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_set_player_health(LeafHandle player, float health) {
    if (!active_ || !active_->minecraft_) {
        return LEAF_STATUS_NOT_SUPPORTED;
    }
    auto st = active_->minecraft_->set_player_health(player_handle{player}, health);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_get_player_food(
    LeafHandle player,
    int32_t* out_food,
    float* out_saturation) {
    if (!active_ || !active_->minecraft_ || !out_food || !out_saturation) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    auto st = active_->minecraft_->get_player_food(
        player_handle{player},
        *out_food,
        *out_saturation);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_set_player_food(
    LeafHandle player,
    int32_t food,
    float saturation) {
    if (!active_ || !active_->minecraft_) {
        return LEAF_STATUS_NOT_SUPPORTED;
    }
    auto st = active_->minecraft_->set_player_food(
        player_handle{player},
        food,
        saturation);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_get_player_gamemode(
    LeafHandle player,
    uint32_t* out_mode) {
    if (!active_ || !active_->minecraft_ || !out_mode) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    auto st = active_->minecraft_->get_player_gamemode(
        player_handle{player},
        *out_mode);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_set_player_gamemode(
    LeafHandle player,
    uint32_t mode) {
    if (!active_ || !active_->minecraft_) {
        return LEAF_STATUS_NOT_SUPPORTED;
    }
    auto st = active_->minecraft_->set_player_gamemode(player_handle{player}, mode);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_get_player_xp(
    LeafHandle player,
    int32_t* out_level,
    float* out_progress) {
    if (!active_ || !active_->minecraft_ || !out_level || !out_progress) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    auto st = active_->minecraft_->get_player_xp(
        player_handle{player},
        *out_level,
        *out_progress);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_set_player_xp_level(LeafHandle player, int32_t level) {
    if (!active_ || !active_->minecraft_) {
        return LEAF_STATUS_NOT_SUPPORTED;
    }
    auto st = active_->minecraft_->set_player_xp_level(player_handle{player}, level);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_get_player_look(
    LeafHandle player,
    float* out_yaw,
    float* out_pitch) {
    if (!active_ || !active_->minecraft_ || !out_yaw || !out_pitch) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    auto st = active_->minecraft_->get_player_look(
        player_handle{player},
        *out_yaw,
        *out_pitch);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_set_player_look(
    LeafHandle player,
    float yaw,
    float pitch) {
    if (!active_ || !active_->minecraft_) {
        return LEAF_STATUS_NOT_SUPPORTED;
    }
    auto st = active_->minecraft_->set_player_look(player_handle{player}, yaw, pitch);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_play_sound(
    LeafHandle player,
    const char* sound_id,
    float volume,
    float pitch,
    int32_t x,
    int32_t y,
    int32_t z,
    uint32_t dimension) {
    if (!active_ || !active_->minecraft_ || !sound_id) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    auto st = active_->minecraft_->play_sound(
        player_handle{player},
        sound_id,
        volume,
        pitch,
        x,
        y,
        z,
        dimension);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_send_actionbar(LeafHandle player, const char* message) {
    if (!active_ || !active_->minecraft_ || !message) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    auto st = active_->minecraft_->send_actionbar(player_handle{player}, message);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_send_title(
    LeafHandle player,
    const char* title,
    const char* subtitle,
    int32_t fade_in_ticks,
    int32_t stay_ticks,
    int32_t fade_out_ticks) {
    if (!active_ || !active_->minecraft_) {
        return LEAF_STATUS_NOT_SUPPORTED;
    }
    auto st = active_->minecraft_->send_title(
        player_handle{player},
        title ? title : "",
        subtitle ? subtitle : "",
        fade_in_ticks,
        stay_ticks,
        fade_out_ticks);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_kick_player(LeafHandle player, const char* reason) {
    if (!active_ || !active_->minecraft_) {
        return LEAF_STATUS_NOT_SUPPORTED;
    }
    auto st = active_->minecraft_->kick_player(
        player_handle{player}, reason ? reason : "");
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_give_item(
    LeafHandle player,
    uint32_t item_id,
    uint32_t count,
    int32_t damage) {
    if (!active_ || !active_->minecraft_) {
        return LEAF_STATUS_NOT_SUPPORTED;
    }
    auto st = active_->minecraft_->give_item(
        player_handle{player}, item_id, count, damage);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_apply_effect(
    LeafHandle player,
    const char* effect_id,
    int32_t duration_ticks,
    int32_t amplifier,
    uint32_t flags) {
    if (!active_ || !active_->minecraft_ || !effect_id) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    auto st = active_->minecraft_->apply_effect(
        player_handle{player},
        effect_id,
        duration_ticks,
        amplifier,
        flags);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_clear_effects(LeafHandle player) {
    if (!active_ || !active_->minecraft_) {
        return LEAF_STATUS_NOT_SUPPORTED;
    }
    auto st = active_->minecraft_->clear_effects(player_handle{player});
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_spawn_particle(
    const char* particle_id,
    double x,
    double y,
    double z,
    uint32_t dimension,
    uint32_t count,
    double dx,
    double dy,
    double dz,
    double speed) {
    if (!active_ || !active_->minecraft_ || !particle_id) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    auto st = active_->minecraft_->spawn_particle(
        particle_id, x, y, z, dimension, count, dx, dy, dz, speed);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_get_world_time(uint32_t dimension, int64_t* out_time) {
    if (!active_ || !active_->minecraft_ || !out_time) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    auto st = active_->minecraft_->get_world_time(dimension, *out_time);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_set_world_time(uint32_t dimension, int64_t time) {
    if (!active_ || !active_->minecraft_) {
        return LEAF_STATUS_NOT_SUPPORTED;
    }
    auto st = active_->minecraft_->set_world_time(dimension, time);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_get_player_velocity(
    LeafHandle player,
    double* out_vx,
    double* out_vy,
    double* out_vz) {
    if (!active_ || !active_->minecraft_ || !out_vx || !out_vy || !out_vz) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    auto st = active_->minecraft_->get_player_velocity(
        player_handle{player}, *out_vx, *out_vy, *out_vz);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_set_player_velocity(
    LeafHandle player,
    double vx,
    double vy,
    double vz) {
    if (!active_ || !active_->minecraft_) {
        return LEAF_STATUS_NOT_SUPPORTED;
    }
    auto st = active_->minecraft_->set_player_velocity(
        player_handle{player}, vx, vy, vz);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_get_player_flags(
    LeafHandle player,
    uint32_t* out_flags) {
    if (!active_ || !active_->minecraft_ || !out_flags) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    auto st = active_->minecraft_->get_player_flags(
        player_handle{player}, *out_flags);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_run_command(LeafHandle player, const char* command) {
    if (!active_ || !active_->minecraft_ || !command) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    auto st = active_->minecraft_->run_command(player_handle{player}, command);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_clear_inventory(LeafHandle player) {
    if (!active_ || !active_->minecraft_) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    auto st = active_->minecraft_->clear_inventory(player_handle{player});
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_broadcast_actionbar(const char* message) {
    if (!active_ || !active_->minecraft_ || !message) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    auto st = active_->minecraft_->broadcast_actionbar(message);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_broadcast_title(
    const char* title,
    const char* subtitle,
    std::int32_t fade_in_ticks,
    std::int32_t stay_ticks,
    std::int32_t fade_out_ticks) {
    if (!active_ || !active_->minecraft_ || !title) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    auto st = active_->minecraft_->broadcast_title(
        title,
        subtitle ? subtitle : "",
        fade_in_ticks,
        stay_ticks,
        fade_out_ticks);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_set_player_flight(
    LeafHandle player,
    std::int32_t allow_flight,
    std::int32_t flying) {
    if (!active_ || !active_->minecraft_) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    auto st = active_->minecraft_->set_player_flight(
        player_handle{player}, allow_flight, flying);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_get_biome(
    std::uint32_t dimension,
    std::int32_t x,
    std::int32_t y,
    std::int32_t z,
    char* out_buf,
    std::uint32_t out_buf_size) {
    if (!active_ || !active_->minecraft_ || !out_buf || out_buf_size == 0) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    std::string id;
    auto st = active_->minecraft_->get_biome(dimension, x, y, z, id);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    const auto copy = id.size() < (out_buf_size - 1) ? id.size() : (out_buf_size - 1);
    for (std::size_t i = 0; i < copy; ++i) {
        out_buf[i] = id[i];
    }
    out_buf[copy] = '\0';
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_get_difficulty(std::uint32_t* out_difficulty) {
    if (!active_ || !active_->minecraft_ || !out_difficulty) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    auto st = active_->minecraft_->get_difficulty(*out_difficulty);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_set_difficulty(std::uint32_t difficulty) {
    if (!active_ || !active_->minecraft_) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    auto st = active_->minecraft_->set_difficulty(difficulty);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_get_weather(
    std::uint32_t dimension,
    std::uint32_t* out_weather) {
    if (!active_ || !active_->minecraft_ || !out_weather) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    auto st = active_->minecraft_->get_weather(dimension, *out_weather);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_set_weather(
    std::uint32_t dimension,
    std::uint32_t weather,
    std::int32_t duration_ticks) {
    if (!active_ || !active_->minecraft_) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    auto st = active_->minecraft_->set_weather(dimension, weather, duration_ticks);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_get_light_level(
    std::uint32_t dimension,
    std::int32_t x,
    std::int32_t y,
    std::int32_t z,
    std::uint32_t* out_block_light,
    std::uint32_t* out_sky_light) {
    if (!active_ || !active_->minecraft_ || !out_block_light || !out_sky_light) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    auto st = active_->minecraft_->get_light_level(
        dimension, x, y, z, *out_block_light, *out_sky_light);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_get_player_latency(
    LeafHandle player,
    std::int32_t* out_ms) {
    if (!active_ || !active_->minecraft_ || !out_ms) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    auto st = active_->minecraft_->get_player_latency(player_handle{player}, *out_ms);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_get_world_spawn(
    std::uint32_t dimension,
    std::int32_t* out_x,
    std::int32_t* out_y,
    std::int32_t* out_z) {
    if (!active_ || !active_->minecraft_ || !out_x || !out_y || !out_z) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    auto st = active_->minecraft_->get_world_spawn(dimension, *out_x, *out_y, *out_z);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_set_world_spawn(
    std::uint32_t dimension,
    std::int32_t x,
    std::int32_t y,
    std::int32_t z) {
    if (!active_ || !active_->minecraft_) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    auto st = active_->minecraft_->set_world_spawn(dimension, x, y, z);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_is_player_op(LeafHandle player, std::int32_t* out_op) {
    if (!active_ || !active_->minecraft_ || !out_op) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    auto st = active_->minecraft_->is_player_op(player_handle{player}, *out_op);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_get_player_uuid(
    LeafHandle player,
    char* out_buf,
    std::uint32_t out_buf_size) {
    if (!active_ || !active_->minecraft_ || !out_buf || out_buf_size == 0) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    std::string uuid;
    auto st = active_->minecraft_->get_player_uuid(player_handle{player}, uuid);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    const auto copy = uuid.size() < (out_buf_size - 1) ? uuid.size() : (out_buf_size - 1);
    for (std::size_t i = 0; i < copy; ++i) {
        out_buf[i] = uuid[i];
    }
    out_buf[copy] = '\0';
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_get_player_permission_level(
    LeafHandle player,
    std::int32_t* out_level) {
    if (!active_ || !active_->minecraft_ || !out_level) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    auto st = active_->minecraft_->get_player_permission_level(
        player_handle{player}, *out_level);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_find_player_by_uuid(
    const char* uuid,
    LeafHandle* out_player) {
    if (!active_ || !active_->minecraft_ || !uuid || !out_player) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    player_handle found{};
    auto st = active_->minecraft_->find_player_by_uuid(uuid, found);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    *out_player = found.raw();
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_find_player_by_name(
    const char* name,
    LeafHandle* out_player) {
    if (!active_ || !active_->minecraft_ || !name || !out_player) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    player_handle found{};
    auto st = active_->minecraft_->find_player_by_name(name, found);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    *out_player = found.raw();
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_get_block_registry_id(
    std::uint32_t dimension,
    std::int32_t x,
    std::int32_t y,
    std::int32_t z,
    char* out_buf,
    std::uint32_t out_buf_size) {
    if (!active_ || !active_->minecraft_ || !out_buf || out_buf_size == 0) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    std::string id;
    auto st = active_->minecraft_->get_block_registry_id(dimension, x, y, z, id);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    const auto copy = id.size() < (out_buf_size - 1) ? id.size() : (out_buf_size - 1);
    for (std::size_t i = 0; i < copy; ++i) {
        out_buf[i] = id[i];
    }
    out_buf[copy] = '\0';
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_set_block_registry_id(
    std::uint32_t dimension,
    std::int32_t x,
    std::int32_t y,
    std::int32_t z,
    const char* block_id) {
    if (!active_ || !active_->minecraft_ || !block_id) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    auto st = active_->minecraft_->set_block_registry_id(
        dimension, x, y, z, block_id);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_give_item_registry_id(
    LeafHandle player,
    const char* item_id,
    std::uint32_t count,
    std::int32_t damage) {
    if (!active_ || !active_->minecraft_ || !item_id) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    auto st = active_->minecraft_->give_item_registry_id(
        player_handle{player}, item_id, count, damage);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_get_inventory_item_registry_id(
    LeafHandle player,
    std::uint32_t slot,
    char* out_buf,
    std::uint32_t out_buf_size,
    std::uint32_t* out_count,
    std::int32_t* out_damage) {
    if (!active_ || !active_->minecraft_ || !out_buf || out_buf_size == 0
        || !out_count || !out_damage) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    std::string id;
    auto st = active_->minecraft_->get_inventory_item_registry_id(
        player_handle{player}, slot, id, *out_count, *out_damage);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    const auto copy = id.size() < (out_buf_size - 1) ? id.size() : (out_buf_size - 1);
    for (std::size_t i = 0; i < copy; ++i) {
        out_buf[i] = id[i];
    }
    out_buf[copy] = '\0';
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_set_inventory_item_registry_id(
    LeafHandle player,
    std::uint32_t slot,
    const char* item_id,
    std::uint32_t count,
    std::int32_t damage) {
    if (!active_ || !active_->minecraft_ || !item_id) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    auto st = active_->minecraft_->set_inventory_item_registry_id(
        player_handle{player}, slot, item_id, count, damage);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_teleport_player(
    LeafHandle player,
    std::int32_t x,
    std::int32_t y,
    std::int32_t z,
    std::uint32_t dimension,
    float yaw,
    float pitch) {
    if (!active_ || !active_->minecraft_) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    auto st = active_->minecraft_->teleport_player(
        player_handle{player}, x, y, z, dimension, yaw, pitch);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_get_selected_slot(
    LeafHandle player,
    std::uint32_t* out_slot) {
    if (!active_ || !active_->minecraft_ || !out_slot) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    auto st = active_->minecraft_->get_selected_slot(
        player_handle{player}, *out_slot);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_set_selected_slot(
    LeafHandle player,
    std::uint32_t slot) {
    if (!active_ || !active_->minecraft_) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    auto st = active_->minecraft_->set_selected_slot(player_handle{player}, slot);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_get_held_item_registry_id(
    LeafHandle player,
    char* out_buf,
    std::uint32_t out_buf_size,
    std::uint32_t* out_count,
    std::int32_t* out_damage) {
    if (!active_ || !active_->minecraft_ || !out_buf || out_buf_size == 0
        || !out_count || !out_damage) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    std::string id;
    auto st = active_->minecraft_->get_held_item_registry_id(
        player_handle{player}, id, *out_count, *out_damage);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    const auto copy = id.size() < (out_buf_size - 1) ? id.size() : (out_buf_size - 1);
    for (std::size_t i = 0; i < copy; ++i) {
        out_buf[i] = id[i];
    }
    out_buf[copy] = '\0';
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_set_held_item_registry_id(
    LeafHandle player,
    const char* item_id,
    std::uint32_t count,
    std::int32_t damage) {
    if (!active_ || !active_->minecraft_ || !item_id) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    auto st = active_->minecraft_->set_held_item_registry_id(
        player_handle{player}, item_id, count, damage);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_get_equipment_item_registry_id(
    LeafHandle player,
    std::uint32_t equip_slot,
    char* out_buf,
    std::uint32_t out_buf_size,
    std::uint32_t* out_count,
    std::int32_t* out_damage) {
    if (!active_ || !active_->minecraft_ || !out_buf || out_buf_size == 0
        || !out_count || !out_damage) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    std::string id;
    auto st = active_->minecraft_->get_equipment_item_registry_id(
        player_handle{player}, equip_slot, id, *out_count, *out_damage);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    const auto copy = id.size() < (out_buf_size - 1) ? id.size() : (out_buf_size - 1);
    for (std::size_t i = 0; i < copy; ++i) {
        out_buf[i] = id[i];
    }
    out_buf[copy] = '\0';
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_set_equipment_item_registry_id(
    LeafHandle player,
    std::uint32_t equip_slot,
    const char* item_id,
    std::uint32_t count,
    std::int32_t damage) {
    if (!active_ || !active_->minecraft_ || !item_id) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    auto st = active_->minecraft_->set_equipment_item_registry_id(
        player_handle{player}, equip_slot, item_id, count, damage);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_get_world_seed(
    std::uint32_t dimension,
    std::int64_t* out_seed) {
    if (!active_ || !active_->minecraft_ || !out_seed) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    auto st = active_->minecraft_->get_world_seed(dimension, *out_seed);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_heal_player(LeafHandle player) {
    if (!active_ || !active_->minecraft_) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    auto st = active_->minecraft_->heal_player(player_handle{player});
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_get_player_absorption(
    LeafHandle player,
    float* out_absorption) {
    if (!active_ || !active_->minecraft_ || !out_absorption) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    auto st = active_->minecraft_->get_player_absorption(
        player_handle{player}, *out_absorption);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_set_player_absorption(
    LeafHandle player,
    float absorption) {
    if (!active_ || !active_->minecraft_) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    auto st = active_->minecraft_->set_player_absorption(
        player_handle{player}, absorption);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_get_player_invulnerable(
    LeafHandle player,
    std::int32_t* out_invulnerable) {
    if (!active_ || !active_->minecraft_ || !out_invulnerable) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    auto st = active_->minecraft_->get_player_invulnerable(
        player_handle{player}, *out_invulnerable);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_set_player_invulnerable(
    LeafHandle player,
    std::int32_t invulnerable) {
    if (!active_ || !active_->minecraft_) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    auto st = active_->minecraft_->set_player_invulnerable(
        player_handle{player}, invulnerable);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_get_player_air(
    LeafHandle player,
    std::int32_t* out_air) {
    if (!active_ || !active_->minecraft_ || !out_air) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    auto st = active_->minecraft_->get_player_air(
        player_handle{player}, *out_air);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_set_player_air(
    LeafHandle player,
    std::int32_t air) {
    if (!active_ || !active_->minecraft_) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    auto st = active_->minecraft_->set_player_air(player_handle{player}, air);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_get_player_fire_ticks(
    LeafHandle player,
    std::int32_t* out_ticks) {
    if (!active_ || !active_->minecraft_ || !out_ticks) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    auto st = active_->minecraft_->get_player_fire_ticks(
        player_handle{player}, *out_ticks);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_set_player_fire_ticks(
    LeafHandle player,
    std::int32_t ticks) {
    if (!active_ || !active_->minecraft_) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    auto st = active_->minecraft_->set_player_fire_ticks(
        player_handle{player}, ticks);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_get_player_frozen_ticks(
    LeafHandle player,
    std::int32_t* out_ticks) {
    if (!active_ || !active_->minecraft_ || !out_ticks) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    auto st = active_->minecraft_->get_player_frozen_ticks(
        player_handle{player}, *out_ticks);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_set_player_frozen_ticks(
    LeafHandle player,
    std::int32_t ticks) {
    if (!active_ || !active_->minecraft_) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    auto st = active_->minecraft_->set_player_frozen_ticks(
        player_handle{player}, ticks);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_extinguish_player(LeafHandle player) {
    if (!active_ || !active_->minecraft_) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    auto st = active_->minecraft_->extinguish_player(player_handle{player});
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_unfreeze_player(LeafHandle player) {
    if (!active_ || !active_->minecraft_) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    auto st = active_->minecraft_->unfreeze_player(player_handle{player});
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_get_player_no_gravity(
    LeafHandle player,
    std::int32_t* out_no_gravity) {
    if (!active_ || !active_->minecraft_ || !out_no_gravity) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    auto st = active_->minecraft_->get_player_no_gravity(
        player_handle{player}, *out_no_gravity);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_set_player_no_gravity(
    LeafHandle player,
    std::int32_t no_gravity) {
    if (!active_ || !active_->minecraft_) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    auto st = active_->minecraft_->set_player_no_gravity(
        player_handle{player}, no_gravity);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_get_player_silent(
    LeafHandle player,
    std::int32_t* out_silent) {
    if (!active_ || !active_->minecraft_ || !out_silent) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    auto st = active_->minecraft_->get_player_silent(
        player_handle{player}, *out_silent);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_set_player_silent(
    LeafHandle player,
    std::int32_t silent) {
    if (!active_ || !active_->minecraft_) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    auto st = active_->minecraft_->set_player_silent(
        player_handle{player}, silent);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_get_player_glowing(
    LeafHandle player,
    std::int32_t* out_glowing) {
    if (!active_ || !active_->minecraft_ || !out_glowing) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    auto st = active_->minecraft_->get_player_glowing(
        player_handle{player}, *out_glowing);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_set_player_glowing(
    LeafHandle player,
    std::int32_t glowing) {
    if (!active_ || !active_->minecraft_) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    auto st = active_->minecraft_->set_player_glowing(
        player_handle{player}, glowing);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_get_player_invisible(
    LeafHandle player,
    std::int32_t* out_invisible) {
    if (!active_ || !active_->minecraft_ || !out_invisible) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    auto st = active_->minecraft_->get_player_invisible(
        player_handle{player}, *out_invisible);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_set_player_invisible(
    LeafHandle player,
    std::int32_t invisible) {
    if (!active_ || !active_->minecraft_) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    auto st = active_->minecraft_->set_player_invisible(
        player_handle{player}, invisible);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_get_player_portal_cooldown(
    LeafHandle player,
    std::int32_t* out_ticks) {
    if (!active_ || !active_->minecraft_ || !out_ticks) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    auto st = active_->minecraft_->get_player_portal_cooldown(
        player_handle{player}, *out_ticks);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_set_player_portal_cooldown(
    LeafHandle player,
    std::int32_t ticks) {
    if (!active_ || !active_->minecraft_) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    auto st = active_->minecraft_->set_player_portal_cooldown(
        player_handle{player}, ticks);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_get_player_max_air(
    LeafHandle player,
    std::int32_t* out_max_air) {
    if (!active_ || !active_->minecraft_ || !out_max_air) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    auto st = active_->minecraft_->get_player_max_air(
        player_handle{player}, *out_max_air);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_refill_player_air(LeafHandle player) {
    if (!active_ || !active_->minecraft_) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    auto st = active_->minecraft_->refill_player_air(player_handle{player});
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_is_player_alive(
    LeafHandle player,
    std::int32_t* out_alive) {
    if (!active_ || !active_->minecraft_ || !out_alive) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    auto st = active_->minecraft_->is_player_alive(
        player_handle{player}, *out_alive);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_subscribe_event(
    uint32_t raw_event_id,
    LeafEventCallback callback,
    void* user_data,
    uint64_t* out_subscription_id) {
    if (!active_ || !active_->events_ || !callback || !out_subscription_id) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }

    const auto eid = static_cast<leaf::event_id>(raw_event_id);
    const auto* desc = active_->events_->registry().find(eid);
    if (!desc) {
        return LEAF_STATUS_NOT_FOUND;
    }

    result<subscription> sub = err<subscription>(ec::not_supported, "subscribe");
    if (desc->kind == event_kind::notification) {
        sub = active_->events_->subscribe_notification(
            eid,
            [callback, user_data](event_packet_view view) {
                callback(static_cast<const void*>(&view.header()), user_data);
            },
            event_priority::normal,
            active_->default_owner_);
    } else if (desc->kind == event_kind::decision) {
        // C ABI observers: vote PASS. Dedicated decision vote ABI can come later.
        sub = active_->events_->subscribe_decision(
            eid,
            [callback, user_data](event_packet_view view) {
                callback(static_cast<const void*>(&view.header()), user_data);
                return decision::pass;
            },
            event_priority::normal,
            active_->default_owner_);
    } else {
        return LEAF_STATUS_NOT_SUPPORTED;
    }

    if (!sub) {
        switch (sub.error().code()) {
            case ec::event_not_found:
                return LEAF_STATUS_NOT_FOUND;
            case ec::event_wrong_kind:
                return LEAF_STATUS_NOT_SUPPORTED;
            default:
                return LEAF_STATUS_ERROR;
        }
    }

    const auto id = sub->id();
    *out_subscription_id = id;

    std::scoped_lock lock(active_->mutex_);
    active_->c_listeners_.emplace(
        id,
        c_listener{
            .callback = callback,
            .user_data = user_data,
            .sub = std::move(*sub),
        });
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_unsubscribe_event(uint64_t raw_subscription_id) {
    if (!active_) {
        return LEAF_STATUS_ERROR;
    }
    std::scoped_lock lock(active_->mutex_);
    auto it = active_->c_listeners_.find(raw_subscription_id);
    if (it == active_->c_listeners_.end()) {
        return LEAF_STATUS_NOT_FOUND;
    }
    it->second.sub.unsubscribe();
    active_->c_listeners_.erase(it);
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_subscribe_decision(
    uint32_t raw_event_id,
    LeafDecisionCallback callback,
    void* user_data,
    uint64_t* out_subscription_id) {
    if (!active_ || !active_->events_ || !callback || !out_subscription_id) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }

    auto sub = active_->events_->subscribe_decision(
        static_cast<leaf::event_id>(raw_event_id),
        [callback, user_data](event_packet_view view) {
            const int vote = callback(
                static_cast<const void*>(&view.header()),
                user_data);
            switch (vote) {
                case LEAF_DECISION_ALLOW:
                    return decision::allow;
                case LEAF_DECISION_DENY:
                    return decision::deny;
                default:
                    return decision::pass;
            }
        },
        event_priority::normal,
        active_->default_owner_);

    if (!sub) {
        switch (sub.error().code()) {
            case ec::event_not_found:
                return LEAF_STATUS_NOT_FOUND;
            case ec::event_wrong_kind:
                return LEAF_STATUS_NOT_SUPPORTED;
            default:
                return LEAF_STATUS_ERROR;
        }
    }

    const auto id = sub->id();
    *out_subscription_id = id;

    std::scoped_lock lock(active_->mutex_);
    active_->c_listeners_.emplace(
        id,
        c_listener{
            .callback = nullptr,
            .user_data = user_data,
            .sub = std::move(*sub),
        });
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_post_main(LeafTaskCallback callback, void* user_data) {
    if (!active_ || !active_->scheduler_ || !callback) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    auto st = active_->scheduler_->post_main(
        [callback, user_data] { callback(user_data); },
        /*inline_if_main=*/false);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_delay_main(
    LeafTaskCallback callback,
    void* user_data,
    uint32_t delay_ms,
    uint64_t* out_task_id) {
    if (!active_ || !active_->scheduler_ || !callback || !out_task_id) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    auto handle = active_->scheduler_->delay(
        [callback, user_data] { callback(user_data); },
        std::chrono::milliseconds{delay_ms});
    if (!handle) {
        return to_abi_status(handle.error().code());
    }
    *out_task_id = handle->id();
    return LEAF_STATUS_OK;
}

LeafStatus runtime_api::abi_cancel_task(uint64_t raw_task_id) {
    if (!active_ || !active_->scheduler_) {
        return LEAF_STATUS_NOT_SUPPORTED;
    }
    auto st = active_->scheduler_->cancel(raw_task_id);
    if (!st) {
        return to_abi_status(st.error().code());
    }
    return LEAF_STATUS_OK;
}

} // namespace leaf
