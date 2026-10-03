#pragma once

#include <functional>
#include <mutex>
#include <string_view>
#include <unordered_map>

#include "leaf/abi/leaf_abi_v1.h"
#include "leaf/core/capability.hpp"
#include "leaf/events/event_id.hpp"
#include "leaf/events/subscription.hpp"

namespace leaf {

class event_runtime;
class minecraft_abi;

/// Owns the host-side LeafApiV1 table passed into every Leaf Mod.
class runtime_api {
public:
    using log_sink = std::function<void(int level, std::string_view message)>;

    explicit runtime_api(capability_set capabilities = {});

    void set_log_sink(log_sink sink);
    void set_capabilities(capability_set capabilities);

    /// Attach the process event runtime (required for subscribe_event ABI).
    void attach_events(event_runtime* runtime, mod_id default_owner = test_mod_id);

    void attach_scheduler(class scheduler* sched) noexcept { scheduler_ = sched; }

    void attach_minecraft_abi(minecraft_abi* abi) noexcept { minecraft_ = abi; }

    [[nodiscard]] event_runtime* events() noexcept { return events_; }
    [[nodiscard]] scheduler* schedule() noexcept { return scheduler_; }
    [[nodiscard]] minecraft_abi* minecraft() noexcept { return minecraft_; }

    [[nodiscard]] const LeafApiV1& abi() const noexcept { return api_; }
    [[nodiscard]] LeafApiV1& abi() noexcept { return api_; }

    [[nodiscard]] const capability_set& capabilities() const noexcept {
        return capabilities_;
    }

private:
    struct c_listener {
        LeafEventCallback callback{nullptr};
        void* user_data{nullptr};
        subscription sub{};
    };

    static void abi_log(int level, const char* message);
    static int abi_has_capability(uint32_t capability);
    static LeafHandle abi_get_server();
    static LeafStatus abi_send_player_message(LeafHandle player, const char* message);
    static LeafStatus abi_broadcast_message(const char* message);
    static std::uint32_t abi_player_count();
    static LeafStatus abi_get_player_at(uint32_t index, LeafHandle* out_player);
    static LeafStatus abi_get_player_name(
        LeafHandle player,
        char* out_buf,
        std::uint32_t out_buf_size);
    static LeafStatus abi_get_inventory_slot(
        LeafHandle player,
        std::uint32_t slot,
        std::uint32_t* out_item_id,
        std::uint32_t* out_count);
    static LeafStatus abi_set_inventory_slot(
        LeafHandle player,
        std::uint32_t slot,
        std::uint32_t item_id,
        std::uint32_t count);
    static LeafStatus abi_get_inventory_stack(
        LeafHandle player,
        std::uint32_t slot,
        LeafItemStackV1* out_stack);
    static LeafStatus abi_set_inventory_stack(
        LeafHandle player,
        std::uint32_t slot,
        const LeafItemStackV1* stack);
    static LeafStatus abi_get_block(
        std::int32_t x,
        std::int32_t y,
        std::int32_t z,
        std::uint32_t* out_block_id);
    static LeafStatus abi_set_block(
        std::int32_t x,
        std::int32_t y,
        std::int32_t z,
        std::uint32_t block_id);
    static LeafStatus abi_get_block_dim(
        std::uint32_t dimension,
        std::int32_t x,
        std::int32_t y,
        std::int32_t z,
        std::uint32_t* out_block_id);
    static LeafStatus abi_set_block_dim(
        std::uint32_t dimension,
        std::int32_t x,
        std::int32_t y,
        std::int32_t z,
        std::uint32_t block_id);
    static LeafStatus abi_get_player_pos(
        LeafHandle player,
        std::int32_t* out_x,
        std::int32_t* out_y,
        std::int32_t* out_z,
        std::uint32_t* out_dimension);
    static LeafStatus abi_set_player_pos(
        LeafHandle player,
        std::int32_t x,
        std::int32_t y,
        std::int32_t z,
        std::uint32_t dimension);
    static LeafStatus abi_get_player_health(
        LeafHandle player,
        float* out_health,
        float* out_max_health);
    static LeafStatus abi_set_player_health(LeafHandle player, float health);
    static LeafStatus abi_get_player_food(
        LeafHandle player,
        std::int32_t* out_food,
        float* out_saturation);
    static LeafStatus abi_set_player_food(
        LeafHandle player,
        std::int32_t food,
        float saturation);
    static LeafStatus abi_get_player_gamemode(
        LeafHandle player,
        std::uint32_t* out_mode);
    static LeafStatus abi_set_player_gamemode(
        LeafHandle player,
        std::uint32_t mode);
    static LeafStatus abi_get_player_xp(
        LeafHandle player,
        std::int32_t* out_level,
        float* out_progress);
    static LeafStatus abi_set_player_xp_level(LeafHandle player, std::int32_t level);
    static LeafStatus abi_get_player_look(
        LeafHandle player,
        float* out_yaw,
        float* out_pitch);
    static LeafStatus abi_set_player_look(
        LeafHandle player,
        float yaw,
        float pitch);
    static LeafStatus abi_play_sound(
        LeafHandle player,
        const char* sound_id,
        float volume,
        float pitch,
        std::int32_t x,
        std::int32_t y,
        std::int32_t z,
        std::uint32_t dimension);
    static LeafStatus abi_send_actionbar(LeafHandle player, const char* message);
    static LeafStatus abi_send_title(
        LeafHandle player,
        const char* title,
        const char* subtitle,
        std::int32_t fade_in_ticks,
        std::int32_t stay_ticks,
        std::int32_t fade_out_ticks);
    static LeafStatus abi_kick_player(LeafHandle player, const char* reason);
    static LeafStatus abi_give_item(
        LeafHandle player,
        std::uint32_t item_id,
        std::uint32_t count,
        std::int32_t damage);
    static LeafStatus abi_apply_effect(
        LeafHandle player,
        const char* effect_id,
        std::int32_t duration_ticks,
        std::int32_t amplifier,
        std::uint32_t flags);
    static LeafStatus abi_clear_effects(LeafHandle player);
    static LeafStatus abi_spawn_particle(
        const char* particle_id,
        double x,
        double y,
        double z,
        std::uint32_t dimension,
        std::uint32_t count,
        double dx,
        double dy,
        double dz,
        double speed);
    static LeafStatus abi_get_world_time(std::uint32_t dimension, std::int64_t* out_time);
    static LeafStatus abi_set_world_time(std::uint32_t dimension, std::int64_t time);
    static LeafStatus abi_get_player_velocity(
        LeafHandle player,
        double* out_vx,
        double* out_vy,
        double* out_vz);
    static LeafStatus abi_set_player_velocity(
        LeafHandle player,
        double vx,
        double vy,
        double vz);
    static LeafStatus abi_get_player_flags(LeafHandle player, std::uint32_t* out_flags);
    static LeafStatus abi_run_command(LeafHandle player, const char* command);
    static LeafStatus abi_clear_inventory(LeafHandle player);
    static LeafStatus abi_broadcast_actionbar(const char* message);
    static LeafStatus abi_broadcast_title(
        const char* title,
        const char* subtitle,
        std::int32_t fade_in_ticks,
        std::int32_t stay_ticks,
        std::int32_t fade_out_ticks);
    static LeafStatus abi_set_player_flight(
        LeafHandle player,
        std::int32_t allow_flight,
        std::int32_t flying);
    static LeafStatus abi_get_biome(
        std::uint32_t dimension,
        std::int32_t x,
        std::int32_t y,
        std::int32_t z,
        char* out_buf,
        std::uint32_t out_buf_size);
    static LeafStatus abi_get_difficulty(std::uint32_t* out_difficulty);
    static LeafStatus abi_set_difficulty(std::uint32_t difficulty);
    static LeafStatus abi_get_weather(
        std::uint32_t dimension,
        std::uint32_t* out_weather);
    static LeafStatus abi_set_weather(
        std::uint32_t dimension,
        std::uint32_t weather,
        std::int32_t duration_ticks);
    static LeafStatus abi_get_light_level(
        std::uint32_t dimension,
        std::int32_t x,
        std::int32_t y,
        std::int32_t z,
        std::uint32_t* out_block_light,
        std::uint32_t* out_sky_light);
    static LeafStatus abi_get_player_latency(LeafHandle player, std::int32_t* out_ms);
    static LeafStatus abi_get_world_spawn(
        std::uint32_t dimension,
        std::int32_t* out_x,
        std::int32_t* out_y,
        std::int32_t* out_z);
    static LeafStatus abi_set_world_spawn(
        std::uint32_t dimension,
        std::int32_t x,
        std::int32_t y,
        std::int32_t z);
    static LeafStatus abi_is_player_op(LeafHandle player, std::int32_t* out_op);
    static LeafStatus abi_get_player_uuid(
        LeafHandle player,
        char* out_buf,
        std::uint32_t out_buf_size);
    static LeafStatus abi_get_player_permission_level(
        LeafHandle player,
        std::int32_t* out_level);
    static LeafStatus abi_find_player_by_uuid(
        const char* uuid,
        LeafHandle* out_player);
    static LeafStatus abi_find_player_by_name(
        const char* name,
        LeafHandle* out_player);
    static LeafStatus abi_get_block_registry_id(
        std::uint32_t dimension,
        std::int32_t x,
        std::int32_t y,
        std::int32_t z,
        char* out_buf,
        std::uint32_t out_buf_size);
    static LeafStatus abi_set_block_registry_id(
        std::uint32_t dimension,
        std::int32_t x,
        std::int32_t y,
        std::int32_t z,
        const char* block_id);
    static LeafStatus abi_give_item_registry_id(
        LeafHandle player,
        const char* item_id,
        std::uint32_t count,
        std::int32_t damage);
    static LeafStatus abi_get_inventory_item_registry_id(
        LeafHandle player,
        std::uint32_t slot,
        char* out_buf,
        std::uint32_t out_buf_size,
        std::uint32_t* out_count,
        std::int32_t* out_damage);
    static LeafStatus abi_set_inventory_item_registry_id(
        LeafHandle player,
        std::uint32_t slot,
        const char* item_id,
        std::uint32_t count,
        std::int32_t damage);
    static LeafStatus abi_teleport_player(
        LeafHandle player,
        std::int32_t x,
        std::int32_t y,
        std::int32_t z,
        std::uint32_t dimension,
        float yaw,
        float pitch);
    static LeafStatus abi_get_selected_slot(
        LeafHandle player,
        std::uint32_t* out_slot);
    static LeafStatus abi_set_selected_slot(
        LeafHandle player,
        std::uint32_t slot);
    static LeafStatus abi_get_held_item_registry_id(
        LeafHandle player,
        char* out_buf,
        std::uint32_t out_buf_size,
        std::uint32_t* out_count,
        std::int32_t* out_damage);
    static LeafStatus abi_set_held_item_registry_id(
        LeafHandle player,
        const char* item_id,
        std::uint32_t count,
        std::int32_t damage);
    static LeafStatus abi_get_equipment_item_registry_id(
        LeafHandle player,
        std::uint32_t equip_slot,
        char* out_buf,
        std::uint32_t out_buf_size,
        std::uint32_t* out_count,
        std::int32_t* out_damage);
    static LeafStatus abi_set_equipment_item_registry_id(
        LeafHandle player,
        std::uint32_t equip_slot,
        const char* item_id,
        std::uint32_t count,
        std::int32_t damage);
    static LeafStatus abi_get_world_seed(
        std::uint32_t dimension,
        std::int64_t* out_seed);
    static LeafStatus abi_heal_player(LeafHandle player);
    static LeafStatus abi_get_player_absorption(
        LeafHandle player,
        float* out_absorption);
    static LeafStatus abi_set_player_absorption(
        LeafHandle player,
        float absorption);
    static LeafStatus abi_get_player_invulnerable(
        LeafHandle player,
        std::int32_t* out_invulnerable);
    static LeafStatus abi_set_player_invulnerable(
        LeafHandle player,
        std::int32_t invulnerable);
    static LeafStatus abi_get_player_air(
        LeafHandle player,
        std::int32_t* out_air);
    static LeafStatus abi_set_player_air(
        LeafHandle player,
        std::int32_t air);
    static LeafStatus abi_get_player_fire_ticks(
        LeafHandle player,
        std::int32_t* out_ticks);
    static LeafStatus abi_set_player_fire_ticks(
        LeafHandle player,
        std::int32_t ticks);
    static LeafStatus abi_get_player_frozen_ticks(
        LeafHandle player,
        std::int32_t* out_ticks);
    static LeafStatus abi_set_player_frozen_ticks(
        LeafHandle player,
        std::int32_t ticks);
    static LeafStatus abi_extinguish_player(LeafHandle player);
    static LeafStatus abi_unfreeze_player(LeafHandle player);
    static LeafStatus abi_get_player_no_gravity(
        LeafHandle player,
        std::int32_t* out_no_gravity);
    static LeafStatus abi_set_player_no_gravity(
        LeafHandle player,
        std::int32_t no_gravity);
    static LeafStatus abi_get_player_silent(
        LeafHandle player,
        std::int32_t* out_silent);
    static LeafStatus abi_set_player_silent(
        LeafHandle player,
        std::int32_t silent);
    static LeafStatus abi_get_player_glowing(
        LeafHandle player,
        std::int32_t* out_glowing);
    static LeafStatus abi_set_player_glowing(
        LeafHandle player,
        std::int32_t glowing);
    static LeafStatus abi_get_player_invisible(
        LeafHandle player,
        std::int32_t* out_invisible);
    static LeafStatus abi_set_player_invisible(
        LeafHandle player,
        std::int32_t invisible);
    static LeafStatus abi_get_player_portal_cooldown(
        LeafHandle player,
        std::int32_t* out_ticks);
    static LeafStatus abi_set_player_portal_cooldown(
        LeafHandle player,
        std::int32_t ticks);
    static LeafStatus abi_get_player_max_air(
        LeafHandle player,
        std::int32_t* out_max_air);
    static LeafStatus abi_refill_player_air(LeafHandle player);
    static LeafStatus abi_is_player_alive(
        LeafHandle player,
        std::int32_t* out_alive);
    static LeafStatus abi_subscribe_event(
        uint32_t event_id,
        LeafEventCallback callback,
        void* user_data,
        uint64_t* out_subscription_id);
    static LeafStatus abi_unsubscribe_event(uint64_t subscription_id);
    static LeafStatus abi_subscribe_decision(
        uint32_t event_id,
        LeafDecisionCallback callback,
        void* user_data,
        uint64_t* out_subscription_id);
    static LeafStatus abi_post_main(LeafTaskCallback callback, void* user_data);
    static LeafStatus abi_delay_main(
        LeafTaskCallback callback,
        void* user_data,
        uint32_t delay_ms,
        uint64_t* out_task_id);
    static LeafStatus abi_cancel_task(uint64_t task_id);

    static runtime_api* active_;

    capability_set capabilities_;
    log_sink log_sink_;
    mutable std::mutex mutex_;
    LeafApiV1 api_{};

    event_runtime* events_{nullptr};
    class scheduler* scheduler_{nullptr};
    minecraft_abi* minecraft_{nullptr};
    mod_id default_owner_{test_mod_id};
    std::unordered_map<subscription_id, c_listener> c_listeners_;
};

} // namespace leaf
