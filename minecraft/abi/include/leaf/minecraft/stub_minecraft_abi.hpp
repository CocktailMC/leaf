#pragma once

#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "leaf/minecraft/minecraft_abi.hpp"

namespace leaf {

/// Phase-1 stub ABI used until JNI hooks resolve real Minecraft objects.
/// Stores synthetic players by handle value for tests / headless bridge.
class stub_minecraft_abi final : public minecraft_abi {
public:
    stub_minecraft_abi(version mc, loader_kind loader, capability_set caps);

    [[nodiscard]] version minecraft_version() const noexcept override;
    [[nodiscard]] loader_kind loader() const noexcept override;
    [[nodiscard]] capability_set capabilities() const noexcept override;

    [[nodiscard]] result<server_handle> get_server() override;

    [[nodiscard]] status send_player_message(
        player_handle player,
        std::string_view message) override;

    [[nodiscard]] status broadcast_message(std::string_view message) override;

    [[nodiscard]] std::uint32_t player_count() const noexcept override;

    [[nodiscard]] status get_player_at(
        std::uint32_t index,
        player_handle& out_player) override;

    [[nodiscard]] result<std::string> player_name(player_handle player) override;

    [[nodiscard]] status get_inventory_slot(
        player_handle player,
        std::uint32_t slot,
        std::uint32_t& out_item_id,
        std::uint32_t& out_count) override;

    [[nodiscard]] status set_inventory_slot(
        player_handle player,
        std::uint32_t slot,
        std::uint32_t item_id,
        std::uint32_t count) override;

    [[nodiscard]] status get_inventory_stack(
        player_handle player,
        std::uint32_t slot,
        LeafItemStackV1& out) override;

    [[nodiscard]] status set_inventory_stack(
        player_handle player,
        std::uint32_t slot,
        const LeafItemStackV1& stack) override;

    [[nodiscard]] status get_block(
        std::int32_t x,
        std::int32_t y,
        std::int32_t z,
        std::uint32_t& out_block_id) override;

    [[nodiscard]] status set_block(
        std::int32_t x,
        std::int32_t y,
        std::int32_t z,
        std::uint32_t block_id) override;

    [[nodiscard]] status get_block_dim(
        std::uint32_t dimension,
        std::int32_t x,
        std::int32_t y,
        std::int32_t z,
        std::uint32_t& out_block_id) override;

    [[nodiscard]] status set_block_dim(
        std::uint32_t dimension,
        std::int32_t x,
        std::int32_t y,
        std::int32_t z,
        std::uint32_t block_id) override;

    [[nodiscard]] status get_player_pos(
        player_handle player,
        std::int32_t& out_x,
        std::int32_t& out_y,
        std::int32_t& out_z,
        std::uint32_t& out_dimension) override;

    [[nodiscard]] status set_player_pos(
        player_handle player,
        std::int32_t x,
        std::int32_t y,
        std::int32_t z,
        std::uint32_t dimension) override;

    [[nodiscard]] status get_player_health(
        player_handle player,
        float& out_health,
        float& out_max_health) override;

    [[nodiscard]] status set_player_health(
        player_handle player,
        float health) override;

    [[nodiscard]] status get_player_food(
        player_handle player,
        std::int32_t& out_food,
        float& out_saturation) override;

    [[nodiscard]] status set_player_food(
        player_handle player,
        std::int32_t food,
        float saturation) override;

    [[nodiscard]] status get_player_gamemode(
        player_handle player,
        std::uint32_t& out_mode) override;

    [[nodiscard]] status set_player_gamemode(
        player_handle player,
        std::uint32_t mode) override;

    [[nodiscard]] status get_player_xp(
        player_handle player,
        std::int32_t& out_level,
        float& out_progress) override;

    [[nodiscard]] status set_player_xp_level(
        player_handle player,
        std::int32_t level) override;

    [[nodiscard]] status get_player_look(
        player_handle player,
        float& out_yaw,
        float& out_pitch) override;

    [[nodiscard]] status set_player_look(
        player_handle player,
        float yaw,
        float pitch) override;

    [[nodiscard]] status play_sound(
        player_handle player,
        std::string_view sound_id,
        float volume,
        float pitch,
        std::int32_t x,
        std::int32_t y,
        std::int32_t z,
        std::uint32_t dimension) override;

    [[nodiscard]] status send_actionbar(
        player_handle player,
        std::string_view message) override;

    [[nodiscard]] status send_title(
        player_handle player,
        std::string_view title,
        std::string_view subtitle,
        std::int32_t fade_in_ticks,
        std::int32_t stay_ticks,
        std::int32_t fade_out_ticks) override;

    [[nodiscard]] status kick_player(
        player_handle player,
        std::string_view reason) override;

    [[nodiscard]] status give_item(
        player_handle player,
        std::uint32_t item_id,
        std::uint32_t count,
        std::int32_t damage) override;

    [[nodiscard]] status apply_effect(
        player_handle player,
        std::string_view effect_id,
        std::int32_t duration_ticks,
        std::int32_t amplifier,
        std::uint32_t flags) override;

    [[nodiscard]] status clear_effects(player_handle player) override;

    [[nodiscard]] status spawn_particle(
        std::string_view particle_id,
        double x,
        double y,
        double z,
        std::uint32_t dimension,
        std::uint32_t count,
        double dx,
        double dy,
        double dz,
        double speed) override;

    [[nodiscard]] status get_world_time(
        std::uint32_t dimension,
        std::int64_t& out_time) override;

    [[nodiscard]] status set_world_time(
        std::uint32_t dimension,
        std::int64_t time) override;

    [[nodiscard]] status get_player_velocity(
        player_handle player,
        double& out_vx,
        double& out_vy,
        double& out_vz) override;

    [[nodiscard]] status set_player_velocity(
        player_handle player,
        double vx,
        double vy,
        double vz) override;

    [[nodiscard]] status get_player_flags(
        player_handle player,
        std::uint32_t& out_flags) override;

    [[nodiscard]] status run_command(
        player_handle player,
        std::string_view command) override;

    [[nodiscard]] status clear_inventory(player_handle player) override;

    [[nodiscard]] status broadcast_actionbar(std::string_view message) override;

    [[nodiscard]] status broadcast_title(
        std::string_view title,
        std::string_view subtitle,
        std::int32_t fade_in_ticks,
        std::int32_t stay_ticks,
        std::int32_t fade_out_ticks) override;

    [[nodiscard]] status set_player_flight(
        player_handle player,
        std::int32_t allow_flight,
        std::int32_t flying) override;

    [[nodiscard]] status get_biome(
        std::uint32_t dimension,
        std::int32_t x,
        std::int32_t y,
        std::int32_t z,
        std::string& out_id) override;

    [[nodiscard]] status get_difficulty(std::uint32_t& out_difficulty) override;

    [[nodiscard]] status set_difficulty(std::uint32_t difficulty) override;

    [[nodiscard]] status get_weather(
        std::uint32_t dimension,
        std::uint32_t& out_weather) override;

    [[nodiscard]] status set_weather(
        std::uint32_t dimension,
        std::uint32_t weather,
        std::int32_t duration_ticks) override;

    [[nodiscard]] status get_light_level(
        std::uint32_t dimension,
        std::int32_t x,
        std::int32_t y,
        std::int32_t z,
        std::uint32_t& out_block_light,
        std::uint32_t& out_sky_light) override;

    [[nodiscard]] status get_player_latency(
        player_handle player,
        std::int32_t& out_ms) override;

    [[nodiscard]] status get_world_spawn(
        std::uint32_t dimension,
        std::int32_t& out_x,
        std::int32_t& out_y,
        std::int32_t& out_z) override;

    [[nodiscard]] status set_world_spawn(
        std::uint32_t dimension,
        std::int32_t x,
        std::int32_t y,
        std::int32_t z) override;

    [[nodiscard]] status is_player_op(
        player_handle player,
        std::int32_t& out_op) override;

    [[nodiscard]] status get_player_uuid(
        player_handle player,
        std::string& out_uuid) override;

    [[nodiscard]] status get_player_permission_level(
        player_handle player,
        std::int32_t& out_level) override;

    [[nodiscard]] status find_player_by_uuid(
        std::string_view uuid,
        player_handle& out_player) override;

    [[nodiscard]] status find_player_by_name(
        std::string_view name,
        player_handle& out_player) override;

    [[nodiscard]] status get_block_registry_id(
        std::uint32_t dimension,
        std::int32_t x,
        std::int32_t y,
        std::int32_t z,
        std::string& out_id) override;

    [[nodiscard]] status set_block_registry_id(
        std::uint32_t dimension,
        std::int32_t x,
        std::int32_t y,
        std::int32_t z,
        std::string_view block_id) override;

    [[nodiscard]] status give_item_registry_id(
        player_handle player,
        std::string_view item_id,
        std::uint32_t count,
        std::int32_t damage) override;

    [[nodiscard]] status get_inventory_item_registry_id(
        player_handle player,
        std::uint32_t slot,
        std::string& out_id,
        std::uint32_t& out_count,
        std::int32_t& out_damage) override;

    [[nodiscard]] status set_inventory_item_registry_id(
        player_handle player,
        std::uint32_t slot,
        std::string_view item_id,
        std::uint32_t count,
        std::int32_t damage) override;

    [[nodiscard]] status teleport_player(
        player_handle player,
        std::int32_t x,
        std::int32_t y,
        std::int32_t z,
        std::uint32_t dimension,
        float yaw,
        float pitch) override;

    [[nodiscard]] status get_selected_slot(
        player_handle player,
        std::uint32_t& out_slot) override;

    [[nodiscard]] status set_selected_slot(
        player_handle player,
        std::uint32_t slot) override;

    [[nodiscard]] status get_world_seed(
        std::uint32_t dimension,
        std::int64_t& out_seed) override;

    [[nodiscard]] status get_player_absorption(
        player_handle player,
        float& out_absorption) override;

    [[nodiscard]] status set_player_absorption(
        player_handle player,
        float absorption) override;

    [[nodiscard]] status get_player_invulnerable(
        player_handle player,
        std::int32_t& out_invulnerable) override;

    [[nodiscard]] status set_player_invulnerable(
        player_handle player,
        std::int32_t invulnerable) override;

    [[nodiscard]] status get_player_air(
        player_handle player,
        std::int32_t& out_air) override;

    [[nodiscard]] status set_player_air(
        player_handle player,
        std::int32_t air) override;

    [[nodiscard]] status get_player_fire_ticks(
        player_handle player,
        std::int32_t& out_ticks) override;

    [[nodiscard]] status set_player_fire_ticks(
        player_handle player,
        std::int32_t ticks) override;

    [[nodiscard]] status get_player_frozen_ticks(
        player_handle player,
        std::int32_t& out_ticks) override;

    [[nodiscard]] status set_player_frozen_ticks(
        player_handle player,
        std::int32_t ticks) override;

    [[nodiscard]] status get_player_no_gravity(
        player_handle player,
        std::int32_t& out_no_gravity) override;

    [[nodiscard]] status set_player_no_gravity(
        player_handle player,
        std::int32_t no_gravity) override;

    [[nodiscard]] status get_player_silent(
        player_handle player,
        std::int32_t& out_silent) override;

    [[nodiscard]] status set_player_silent(
        player_handle player,
        std::int32_t silent) override;

    [[nodiscard]] status get_player_glowing(
        player_handle player,
        std::int32_t& out_glowing) override;

    [[nodiscard]] status set_player_glowing(
        player_handle player,
        std::int32_t glowing) override;

    [[nodiscard]] status get_player_invisible(
        player_handle player,
        std::int32_t& out_invisible) override;

    [[nodiscard]] status set_player_invisible(
        player_handle player,
        std::int32_t invisible) override;

    [[nodiscard]] status get_player_portal_cooldown(
        player_handle player,
        std::int32_t& out_ticks) override;

    [[nodiscard]] status set_player_portal_cooldown(
        player_handle player,
        std::int32_t ticks) override;

    [[nodiscard]] status get_player_max_air(
        player_handle player,
        std::int32_t& out_max_air) override;

    /// Test helper: register a synthetic player name for a raw handle.
    void register_player(player_handle player, std::string name);

    void unregister_player(player_handle player);

    void set_send_hook(
        void (*fn)(std::uint64_t player_handle, const char* message)) noexcept {
        send_hook_ = fn;
    }

    void set_broadcast_hook(void (*fn)(const char* message)) noexcept {
        broadcast_hook_ = fn;
    }

    using inv_get_fn = int (*)(
        std::uint64_t,
        std::uint32_t,
        std::uint32_t*,
        std::uint32_t*);
    using inv_set_fn = int (*)(
        std::uint64_t,
        std::uint32_t,
        std::uint32_t,
        std::uint32_t);
    using inv_stack_get_fn = int (*)(
        std::uint64_t,
        std::uint32_t,
        std::uint32_t*,
        std::uint32_t*,
        std::int32_t*);
    using inv_stack_set_fn = int (*)(
        std::uint64_t,
        std::uint32_t,
        std::uint32_t,
        std::uint32_t,
        std::int32_t);
    using block_get_fn = int (*)(
        std::uint32_t,
        std::int32_t,
        std::int32_t,
        std::int32_t,
        std::uint32_t*);
    using block_set_fn = int (*)(
        std::uint32_t,
        std::int32_t,
        std::int32_t,
        std::int32_t,
        std::uint32_t);
    using player_pos_get_fn = int (*)(
        std::uint64_t,
        std::int32_t*,
        std::int32_t*,
        std::int32_t*,
        std::uint32_t*);
    using player_pos_set_fn = int (*)(
        std::uint64_t,
        std::int32_t,
        std::int32_t,
        std::int32_t,
        std::uint32_t);
    using player_health_get_fn = int (*)(std::uint64_t, float*, float*);
    using player_health_set_fn = int (*)(std::uint64_t, float);
    using player_food_get_fn = int (*)(std::uint64_t, std::int32_t*, float*);
    using player_food_set_fn = int (*)(std::uint64_t, std::int32_t, float);
    using player_gamemode_get_fn = int (*)(std::uint64_t, std::uint32_t*);
    using player_gamemode_set_fn = int (*)(std::uint64_t, std::uint32_t);
    using player_xp_get_fn = int (*)(std::uint64_t, std::int32_t*, float*);
    using player_xp_set_fn = int (*)(std::uint64_t, std::int32_t);
    using player_look_get_fn = int (*)(std::uint64_t, float*, float*);
    using player_look_set_fn = int (*)(std::uint64_t, float, float);
    using play_sound_fn = int (*)(
        std::uint64_t,
        const char*,
        float,
        float,
        std::int32_t,
        std::int32_t,
        std::int32_t,
        std::uint32_t);
    using actionbar_fn = int (*)(std::uint64_t, const char*);
    using title_fn = int (*)(
        std::uint64_t,
        const char*,
        const char*,
        std::int32_t,
        std::int32_t,
        std::int32_t);
    using kick_fn = int (*)(std::uint64_t, const char*);
    using give_item_fn = int (*)(
        std::uint64_t,
        std::uint32_t,
        std::uint32_t,
        std::int32_t);
    using apply_effect_fn = int (*)(
        std::uint64_t,
        const char*,
        std::int32_t,
        std::int32_t,
        std::uint32_t);
    using clear_effects_fn = int (*)(std::uint64_t);
    using spawn_particle_fn = int (*)(
        const char*,
        double,
        double,
        double,
        std::uint32_t,
        std::uint32_t,
        double,
        double,
        double,
        double);
    using world_time_get_fn = int (*)(std::uint32_t, std::int64_t*);
    using world_time_set_fn = int (*)(std::uint32_t, std::int64_t);
    using player_velocity_get_fn = int (*)(std::uint64_t, double*, double*, double*);
    using player_velocity_set_fn = int (*)(std::uint64_t, double, double, double);
    using player_flags_fn = int (*)(std::uint64_t, std::uint32_t*);
    using run_command_fn = int (*)(std::uint64_t, const char*);
    using clear_inventory_fn = int (*)(std::uint64_t);
    using player_flight_fn = int (*)(std::uint64_t, std::int32_t, std::int32_t);
    using get_biome_fn = int (*)(
        std::uint32_t,
        std::int32_t,
        std::int32_t,
        std::int32_t,
        char*,
        std::uint32_t);
    using difficulty_get_fn = int (*)(std::uint32_t*);
    using difficulty_set_fn = int (*)(std::uint32_t);
    using weather_get_fn = int (*)(std::uint32_t, std::uint32_t*);
    using weather_set_fn = int (*)(std::uint32_t, std::uint32_t, std::int32_t);
    using get_light_level_fn = int (*)(
        std::uint32_t,
        std::int32_t,
        std::int32_t,
        std::int32_t,
        std::uint32_t*,
        std::uint32_t*);
    using player_latency_fn = int (*)(std::uint64_t, std::int32_t*);
    using world_spawn_get_fn = int (*)(
        std::uint32_t,
        std::int32_t*,
        std::int32_t*,
        std::int32_t*);
    using world_spawn_set_fn = int (*)(
        std::uint32_t,
        std::int32_t,
        std::int32_t,
        std::int32_t);
    using player_op_fn = int (*)(std::uint64_t, std::int32_t*);
    using player_uuid_fn = int (*)(std::uint64_t, char*, std::uint32_t);
    using player_permission_fn = int (*)(std::uint64_t, std::int32_t*);
    using find_player_uuid_fn = int (*)(const char*, std::uint64_t*);
    using find_player_name_fn = int (*)(const char*, std::uint64_t*);
    using get_block_registry_fn = int (*)(
        std::uint32_t,
        std::int32_t,
        std::int32_t,
        std::int32_t,
        char*,
        std::uint32_t);
    using set_block_registry_fn = int (*)(
        std::uint32_t,
        std::int32_t,
        std::int32_t,
        std::int32_t,
        const char*);
    using give_item_registry_fn = int (*)(
        std::uint64_t,
        const char*,
        std::uint32_t,
        std::int32_t);
    using inv_registry_get_fn = int (*)(
        std::uint64_t,
        std::uint32_t,
        char*,
        std::uint32_t,
        std::uint32_t*,
        std::int32_t*);
    using inv_registry_set_fn = int (*)(
        std::uint64_t,
        std::uint32_t,
        const char*,
        std::uint32_t,
        std::int32_t);
    using teleport_fn = int (*)(
        std::uint64_t,
        std::int32_t,
        std::int32_t,
        std::int32_t,
        std::uint32_t,
        float,
        float);
    using selected_slot_get_fn = int (*)(std::uint64_t, std::uint32_t*);
    using selected_slot_set_fn = int (*)(std::uint64_t, std::uint32_t);
    using get_world_seed_fn = int (*)(std::uint32_t, std::int64_t*);
    using player_absorption_get_fn = int (*)(std::uint64_t, float*);
    using player_absorption_set_fn = int (*)(std::uint64_t, float);
    using player_invulnerable_get_fn = int (*)(std::uint64_t, std::int32_t*);
    using player_invulnerable_set_fn = int (*)(std::uint64_t, std::int32_t);
    using player_air_get_fn = int (*)(std::uint64_t, std::int32_t*);
    using player_air_set_fn = int (*)(std::uint64_t, std::int32_t);
    using player_fire_ticks_get_fn = int (*)(std::uint64_t, std::int32_t*);
    using player_fire_ticks_set_fn = int (*)(std::uint64_t, std::int32_t);
    using player_frozen_ticks_get_fn = int (*)(std::uint64_t, std::int32_t*);
    using player_frozen_ticks_set_fn = int (*)(std::uint64_t, std::int32_t);
    using player_no_gravity_get_fn = int (*)(std::uint64_t, std::int32_t*);
    using player_no_gravity_set_fn = int (*)(std::uint64_t, std::int32_t);
    using player_silent_get_fn = int (*)(std::uint64_t, std::int32_t*);
    using player_silent_set_fn = int (*)(std::uint64_t, std::int32_t);
    using player_glowing_get_fn = int (*)(std::uint64_t, std::int32_t*);
    using player_glowing_set_fn = int (*)(std::uint64_t, std::int32_t);
    using player_invisible_get_fn = int (*)(std::uint64_t, std::int32_t*);
    using player_invisible_set_fn = int (*)(std::uint64_t, std::int32_t);
    using player_portal_cooldown_get_fn = int (*)(std::uint64_t, std::int32_t*);
    using player_portal_cooldown_set_fn = int (*)(std::uint64_t, std::int32_t);
    using get_player_max_air_fn = int (*)(std::uint64_t, std::int32_t*);

    void set_inventory_hooks(inv_get_fn get_fn, inv_set_fn set_fn) noexcept {
        inv_get_hook_ = get_fn;
        inv_set_hook_ = set_fn;
    }

    void set_inventory_stack_hooks(
        inv_stack_get_fn get_fn,
        inv_stack_set_fn set_fn) noexcept {
        inv_stack_get_hook_ = get_fn;
        inv_stack_set_hook_ = set_fn;
    }

    void set_world_hooks(block_get_fn get_fn, block_set_fn set_fn) noexcept {
        block_get_hook_ = get_fn;
        block_set_hook_ = set_fn;
    }

    void set_player_pos_hooks(
        player_pos_get_fn get_fn,
        player_pos_set_fn set_fn) noexcept {
        player_pos_get_hook_ = get_fn;
        player_pos_set_hook_ = set_fn;
    }

    void set_player_health_hooks(
        player_health_get_fn get_fn,
        player_health_set_fn set_fn) noexcept {
        player_health_get_hook_ = get_fn;
        player_health_set_hook_ = set_fn;
    }

    void set_player_food_hooks(
        player_food_get_fn get_fn,
        player_food_set_fn set_fn) noexcept {
        player_food_get_hook_ = get_fn;
        player_food_set_hook_ = set_fn;
    }

    void set_player_gamemode_hooks(
        player_gamemode_get_fn get_fn,
        player_gamemode_set_fn set_fn) noexcept {
        player_gamemode_get_hook_ = get_fn;
        player_gamemode_set_hook_ = set_fn;
    }

    void set_player_xp_hooks(
        player_xp_get_fn get_fn,
        player_xp_set_fn set_fn) noexcept {
        player_xp_get_hook_ = get_fn;
        player_xp_set_hook_ = set_fn;
    }

    void set_player_look_hooks(
        player_look_get_fn get_fn,
        player_look_set_fn set_fn) noexcept {
        player_look_get_hook_ = get_fn;
        player_look_set_hook_ = set_fn;
    }

    void set_play_sound_hook(play_sound_fn fn) noexcept { play_sound_hook_ = fn; }

    void set_actionbar_hook(actionbar_fn fn) noexcept { actionbar_hook_ = fn; }

    void set_title_hook(title_fn fn) noexcept { title_hook_ = fn; }

    void set_kick_hook(kick_fn fn) noexcept { kick_hook_ = fn; }

    void set_give_item_hook(give_item_fn fn) noexcept { give_item_hook_ = fn; }

    void set_effect_hooks(apply_effect_fn apply_fn, clear_effects_fn clear_fn) noexcept {
        apply_effect_hook_ = apply_fn;
        clear_effects_hook_ = clear_fn;
    }

    void set_spawn_particle_hook(spawn_particle_fn fn) noexcept {
        spawn_particle_hook_ = fn;
    }

    void set_world_time_hooks(
        world_time_get_fn get_fn,
        world_time_set_fn set_fn) noexcept {
        world_time_get_hook_ = get_fn;
        world_time_set_hook_ = set_fn;
    }

    void set_player_velocity_hooks(
        player_velocity_get_fn get_fn,
        player_velocity_set_fn set_fn) noexcept {
        player_velocity_get_hook_ = get_fn;
        player_velocity_set_hook_ = set_fn;
    }

    void set_player_flags_hook(player_flags_fn fn) noexcept {
        player_flags_hook_ = fn;
    }

    void set_run_command_hook(run_command_fn fn) noexcept {
        run_command_hook_ = fn;
    }

    void set_clear_inventory_hook(clear_inventory_fn fn) noexcept {
        clear_inventory_hook_ = fn;
    }

    void set_player_flight_hook(player_flight_fn fn) noexcept {
        player_flight_hook_ = fn;
    }

    void set_get_biome_hook(get_biome_fn fn) noexcept {
        get_biome_hook_ = fn;
    }

    void set_difficulty_hooks(
        difficulty_get_fn get_fn,
        difficulty_set_fn set_fn) noexcept {
        difficulty_get_hook_ = get_fn;
        difficulty_set_hook_ = set_fn;
    }

    void set_weather_hooks(weather_get_fn get_fn, weather_set_fn set_fn) noexcept {
        weather_get_hook_ = get_fn;
        weather_set_hook_ = set_fn;
    }

    void set_get_light_level_hook(get_light_level_fn fn) noexcept {
        get_light_level_hook_ = fn;
    }

    void set_player_latency_hook(player_latency_fn fn) noexcept {
        player_latency_hook_ = fn;
    }

    void set_world_spawn_hooks(
        world_spawn_get_fn get_fn,
        world_spawn_set_fn set_fn) noexcept {
        world_spawn_get_hook_ = get_fn;
        world_spawn_set_hook_ = set_fn;
    }

    void set_player_op_hook(player_op_fn fn) noexcept {
        player_op_hook_ = fn;
    }

    void set_player_uuid_hook(player_uuid_fn fn) noexcept {
        player_uuid_hook_ = fn;
    }

    void set_player_permission_hook(player_permission_fn fn) noexcept {
        player_permission_hook_ = fn;
    }

    void set_find_player_uuid_hook(find_player_uuid_fn fn) noexcept {
        find_player_uuid_hook_ = fn;
    }

    void set_find_player_name_hook(find_player_name_fn fn) noexcept {
        find_player_name_hook_ = fn;
    }

    void set_block_registry_hooks(
        get_block_registry_fn get_fn,
        set_block_registry_fn set_fn) noexcept {
        get_block_registry_hook_ = get_fn;
        set_block_registry_hook_ = set_fn;
    }

    void set_give_item_registry_hook(give_item_registry_fn fn) noexcept {
        give_item_registry_hook_ = fn;
    }

    void set_inventory_registry_hooks(
        inv_registry_get_fn get_fn,
        inv_registry_set_fn set_fn) noexcept {
        inv_registry_get_hook_ = get_fn;
        inv_registry_set_hook_ = set_fn;
    }

    void set_teleport_hook(teleport_fn fn) noexcept {
        teleport_hook_ = fn;
    }

    void set_selected_slot_hooks(
        selected_slot_get_fn get_fn,
        selected_slot_set_fn set_fn) noexcept {
        selected_slot_get_hook_ = get_fn;
        selected_slot_set_hook_ = set_fn;
    }

    void set_get_world_seed_hook(get_world_seed_fn fn) noexcept {
        get_world_seed_hook_ = fn;
    }

    void set_player_absorption_hooks(
        player_absorption_get_fn get_fn,
        player_absorption_set_fn set_fn) noexcept {
        player_absorption_get_hook_ = get_fn;
        player_absorption_set_hook_ = set_fn;
    }

    void set_player_invulnerable_hooks(
        player_invulnerable_get_fn get_fn,
        player_invulnerable_set_fn set_fn) noexcept {
        player_invulnerable_get_hook_ = get_fn;
        player_invulnerable_set_hook_ = set_fn;
    }

    void set_player_air_hooks(
        player_air_get_fn get_fn,
        player_air_set_fn set_fn) noexcept {
        player_air_get_hook_ = get_fn;
        player_air_set_hook_ = set_fn;
    }

    void set_player_fire_ticks_hooks(
        player_fire_ticks_get_fn get_fn,
        player_fire_ticks_set_fn set_fn) noexcept {
        player_fire_ticks_get_hook_ = get_fn;
        player_fire_ticks_set_hook_ = set_fn;
    }

    void set_player_frozen_ticks_hooks(
        player_frozen_ticks_get_fn get_fn,
        player_frozen_ticks_set_fn set_fn) noexcept {
        player_frozen_ticks_get_hook_ = get_fn;
        player_frozen_ticks_set_hook_ = set_fn;
    }

    void set_player_no_gravity_hooks(
        player_no_gravity_get_fn get_fn,
        player_no_gravity_set_fn set_fn) noexcept {
        player_no_gravity_get_hook_ = get_fn;
        player_no_gravity_set_hook_ = set_fn;
    }

    void set_player_silent_hooks(
        player_silent_get_fn get_fn,
        player_silent_set_fn set_fn) noexcept {
        player_silent_get_hook_ = get_fn;
        player_silent_set_hook_ = set_fn;
    }

    void set_player_glowing_hooks(
        player_glowing_get_fn get_fn,
        player_glowing_set_fn set_fn) noexcept {
        player_glowing_get_hook_ = get_fn;
        player_glowing_set_hook_ = set_fn;
    }

    void set_player_invisible_hooks(
        player_invisible_get_fn get_fn,
        player_invisible_set_fn set_fn) noexcept {
        player_invisible_get_hook_ = get_fn;
        player_invisible_set_hook_ = set_fn;
    }

    void set_player_portal_cooldown_hooks(
        player_portal_cooldown_get_fn get_fn,
        player_portal_cooldown_set_fn set_fn) noexcept {
        player_portal_cooldown_get_hook_ = get_fn;
        player_portal_cooldown_set_hook_ = set_fn;
    }

    void set_get_player_max_air_hook(get_player_max_air_fn fn) noexcept {
        get_player_max_air_hook_ = fn;
    }

    void set_world_seed_for_test(std::uint32_t dimension, std::int64_t seed) {
        world_seeds_[dimension] = seed;
    }

    void set_player_absorption_for_test(player_handle player, float amount) {
        absorption_[player.raw()] = amount;
    }

    void set_player_invulnerable_for_test(player_handle player, bool invuln) {
        invulnerable_[player.raw()] = invuln ? 1 : 0;
    }

    void set_player_air_for_test(player_handle player, std::int32_t air) {
        air_[player.raw()] = air;
    }

    void set_player_fire_ticks_for_test(player_handle player, std::int32_t ticks) {
        fire_ticks_[player.raw()] = ticks;
    }

    void set_player_frozen_ticks_for_test(player_handle player, std::int32_t ticks) {
        frozen_ticks_[player.raw()] = ticks;
    }

    void set_player_no_gravity_for_test(player_handle player, bool no_gravity) {
        no_gravity_[player.raw()] = no_gravity ? 1 : 0;
    }

    void set_player_silent_for_test(player_handle player, bool silent) {
        silent_[player.raw()] = silent ? 1 : 0;
    }

    void set_player_glowing_for_test(player_handle player, bool glowing) {
        glowing_[player.raw()] = glowing ? 1 : 0;
    }

    void set_player_invisible_for_test(player_handle player, bool invisible) {
        invisible_[player.raw()] = invisible ? 1 : 0;
    }

    void set_player_portal_cooldown_for_test(
        player_handle player,
        std::int32_t ticks) {
        portal_cooldown_[player.raw()] = ticks;
    }

    void set_player_max_air_for_test(player_handle player, std::int32_t max_air) {
        max_air_[player.raw()] = max_air;
    }

    void set_selected_slot_for_test(player_handle player, std::uint32_t slot) {
        selected_slots_[player.raw()] = slot;
    }

    void set_player_flags_for_test(player_handle player, std::uint32_t flags) {
        flags_[player.raw()] = flags;
    }

    void set_player_latency_for_test(player_handle player, std::int32_t ms) {
        latencies_[player.raw()] = ms;
    }

    void set_player_op_for_test(player_handle player, bool is_op) {
        ops_[player.raw()] = is_op ? 1 : 0;
    }

    void set_player_uuid_for_test(player_handle player, std::string uuid) {
        uuids_[player.raw()] = std::move(uuid);
    }

    void set_player_permission_for_test(player_handle player, std::int32_t level) {
        permissions_[player.raw()] = level;
    }

    void set_get_block_hook(block_get_fn fn) noexcept { block_get_hook_ = fn; }

    [[nodiscard]] std::size_t message_log_size() const noexcept {
        return message_log_.size();
    }

    [[nodiscard]] const std::string& message_at(std::size_t index) const {
        return message_log_.at(index);
    }

    [[nodiscard]] std::size_t sound_log_size() const noexcept {
        return sound_log_.size();
    }

    [[nodiscard]] const std::string& sound_at(std::size_t index) const {
        return sound_log_.at(index);
    }

    [[nodiscard]] std::size_t actionbar_log_size() const noexcept {
        return actionbar_log_.size();
    }

    [[nodiscard]] const std::string& actionbar_at(std::size_t index) const {
        return actionbar_log_.at(index);
    }

    [[nodiscard]] std::size_t title_log_size() const noexcept {
        return title_log_.size();
    }

    [[nodiscard]] const std::string& title_at(std::size_t index) const {
        return title_log_.at(index);
    }

    [[nodiscard]] std::size_t kick_log_size() const noexcept {
        return kick_log_.size();
    }

    [[nodiscard]] const std::string& kick_at(std::size_t index) const {
        return kick_log_.at(index);
    }

    [[nodiscard]] std::size_t effect_log_size() const noexcept {
        return effect_log_.size();
    }

    [[nodiscard]] const std::string& effect_at(std::size_t index) const {
        return effect_log_.at(index);
    }

    [[nodiscard]] std::size_t particle_log_size() const noexcept {
        return particle_log_.size();
    }

    [[nodiscard]] const std::string& particle_at(std::size_t index) const {
        return particle_log_.at(index);
    }

    [[nodiscard]] std::size_t command_log_size() const noexcept {
        return command_log_.size();
    }

    [[nodiscard]] const std::string& command_at(std::size_t index) const {
        return command_log_.at(index);
    }

    /// Test helper: place a block in the stub world.
    void set_block_for_test(
        std::int32_t x,
        std::int32_t y,
        std::int32_t z,
        std::uint32_t block_id);

private:
    version version_;
    loader_kind loader_;
    capability_set caps_;
    server_handle server_{server_handle{1}};
    std::unordered_map<std::uint64_t, std::string> players_;
    /// player -> (slot -> (item_id, count, damage))
    std::unordered_map<
        std::uint64_t,
        std::unordered_map<std::uint32_t, std::tuple<std::uint32_t, std::uint32_t, std::int32_t>>>
        inventories_;
    /// packed block key (x,y,z) -> block id (air=0)
    std::unordered_map<std::uint64_t, std::uint32_t> blocks_;
    /// player -> (x, y, z, dimension)
    std::unordered_map<
        std::uint64_t,
        std::tuple<std::int32_t, std::int32_t, std::int32_t, std::uint32_t>>
        positions_;
    /// player -> (health, max_health)
    std::unordered_map<std::uint64_t, std::pair<float, float>> health_;
    /// player -> (food, saturation)
    std::unordered_map<std::uint64_t, std::pair<std::int32_t, float>> food_;
    /// player -> gamemode
    std::unordered_map<std::uint64_t, std::uint32_t> gamemodes_;
    /// player -> (level, progress)
    std::unordered_map<std::uint64_t, std::pair<std::int32_t, float>> xp_;
    /// player -> (yaw, pitch)
    std::unordered_map<std::uint64_t, std::pair<float, float>> looks_;
    /// player -> (vx, vy, vz)
    std::unordered_map<std::uint64_t, std::tuple<double, double, double>> velocities_;
    /// player -> LEAF_PLAYER_FLAG_*
    std::unordered_map<std::uint64_t, std::uint32_t> flags_;
    /// player -> active effect ids (stub only)
    std::unordered_map<std::uint64_t, std::unordered_set<std::string>> effects_;
    std::vector<std::string> message_log_;
    std::vector<std::string> sound_log_;
    std::vector<std::string> actionbar_log_;
    std::vector<std::string> title_log_;
    std::vector<std::string> kick_log_;
    std::vector<std::string> effect_log_;
    std::vector<std::string> particle_log_;
    std::vector<std::string> command_log_;
    void (*send_hook_)(std::uint64_t, const char*){nullptr};
    void (*broadcast_hook_)(const char*){nullptr};
    inv_get_fn inv_get_hook_{nullptr};
    inv_set_fn inv_set_hook_{nullptr};
    inv_stack_get_fn inv_stack_get_hook_{nullptr};
    inv_stack_set_fn inv_stack_set_hook_{nullptr};
    block_get_fn block_get_hook_{nullptr};
    block_set_fn block_set_hook_{nullptr};
    player_pos_get_fn player_pos_get_hook_{nullptr};
    player_pos_set_fn player_pos_set_hook_{nullptr};
    player_health_get_fn player_health_get_hook_{nullptr};
    player_health_set_fn player_health_set_hook_{nullptr};
    player_food_get_fn player_food_get_hook_{nullptr};
    player_food_set_fn player_food_set_hook_{nullptr};
    player_gamemode_get_fn player_gamemode_get_hook_{nullptr};
    player_gamemode_set_fn player_gamemode_set_hook_{nullptr};
    player_xp_get_fn player_xp_get_hook_{nullptr};
    player_xp_set_fn player_xp_set_hook_{nullptr};
    player_look_get_fn player_look_get_hook_{nullptr};
    player_look_set_fn player_look_set_hook_{nullptr};
    play_sound_fn play_sound_hook_{nullptr};
    actionbar_fn actionbar_hook_{nullptr};
    title_fn title_hook_{nullptr};
    kick_fn kick_hook_{nullptr};
    give_item_fn give_item_hook_{nullptr};
    apply_effect_fn apply_effect_hook_{nullptr};
    clear_effects_fn clear_effects_hook_{nullptr};
    spawn_particle_fn spawn_particle_hook_{nullptr};
    world_time_get_fn world_time_get_hook_{nullptr};
    world_time_set_fn world_time_set_hook_{nullptr};
    player_velocity_get_fn player_velocity_get_hook_{nullptr};
    player_velocity_set_fn player_velocity_set_hook_{nullptr};
    player_flags_fn player_flags_hook_{nullptr};
    run_command_fn run_command_hook_{nullptr};
    clear_inventory_fn clear_inventory_hook_{nullptr};
    player_flight_fn player_flight_hook_{nullptr};
    get_biome_fn get_biome_hook_{nullptr};
    difficulty_get_fn difficulty_get_hook_{nullptr};
    difficulty_set_fn difficulty_set_hook_{nullptr};
    weather_get_fn weather_get_hook_{nullptr};
    weather_set_fn weather_set_hook_{nullptr};
    get_light_level_fn get_light_level_hook_{nullptr};
    player_latency_fn player_latency_hook_{nullptr};
    world_spawn_get_fn world_spawn_get_hook_{nullptr};
    world_spawn_set_fn world_spawn_set_hook_{nullptr};
    player_op_fn player_op_hook_{nullptr};
    player_uuid_fn player_uuid_hook_{nullptr};
    player_permission_fn player_permission_hook_{nullptr};
    find_player_uuid_fn find_player_uuid_hook_{nullptr};
    find_player_name_fn find_player_name_hook_{nullptr};
    get_block_registry_fn get_block_registry_hook_{nullptr};
    set_block_registry_fn set_block_registry_hook_{nullptr};
    give_item_registry_fn give_item_registry_hook_{nullptr};
    inv_registry_get_fn inv_registry_get_hook_{nullptr};
    inv_registry_set_fn inv_registry_set_hook_{nullptr};
    teleport_fn teleport_hook_{nullptr};
    selected_slot_get_fn selected_slot_get_hook_{nullptr};
    selected_slot_set_fn selected_slot_set_hook_{nullptr};
    get_world_seed_fn get_world_seed_hook_{nullptr};
    player_absorption_get_fn player_absorption_get_hook_{nullptr};
    player_absorption_set_fn player_absorption_set_hook_{nullptr};
    player_invulnerable_get_fn player_invulnerable_get_hook_{nullptr};
    player_invulnerable_set_fn player_invulnerable_set_hook_{nullptr};
    player_air_get_fn player_air_get_hook_{nullptr};
    player_air_set_fn player_air_set_hook_{nullptr};
    player_fire_ticks_get_fn player_fire_ticks_get_hook_{nullptr};
    player_fire_ticks_set_fn player_fire_ticks_set_hook_{nullptr};
    player_frozen_ticks_get_fn player_frozen_ticks_get_hook_{nullptr};
    player_frozen_ticks_set_fn player_frozen_ticks_set_hook_{nullptr};
    player_no_gravity_get_fn player_no_gravity_get_hook_{nullptr};
    player_no_gravity_set_fn player_no_gravity_set_hook_{nullptr};
    player_silent_get_fn player_silent_get_hook_{nullptr};
    player_silent_set_fn player_silent_set_hook_{nullptr};
    player_glowing_get_fn player_glowing_get_hook_{nullptr};
    player_glowing_set_fn player_glowing_set_hook_{nullptr};
    player_invisible_get_fn player_invisible_get_hook_{nullptr};
    player_invisible_set_fn player_invisible_set_hook_{nullptr};
    player_portal_cooldown_get_fn player_portal_cooldown_get_hook_{nullptr};
    player_portal_cooldown_set_fn player_portal_cooldown_set_hook_{nullptr};
    get_player_max_air_fn get_player_max_air_hook_{nullptr};
    std::uint32_t difficulty_{2}; // normal
    std::unordered_map<std::uint32_t, std::uint32_t> weather_;
    std::unordered_map<std::uint64_t, std::int32_t> latencies_;
    std::unordered_map<std::uint64_t, std::int32_t> ops_;
    std::unordered_map<std::uint64_t, std::string> uuids_;
    std::unordered_map<std::uint64_t, std::int32_t> permissions_;
    std::unordered_map<std::uint64_t, std::string> block_registry_ids_;
    std::unordered_map<std::uint64_t, std::string> inv_registry_ids_;
    std::unordered_map<std::uint64_t, std::uint32_t> selected_slots_;
    std::unordered_map<std::uint64_t, float> absorption_;
    std::unordered_map<std::uint64_t, std::int32_t> invulnerable_;
    std::unordered_map<std::uint64_t, std::int32_t> air_;
    std::unordered_map<std::uint64_t, std::int32_t> fire_ticks_;
    std::unordered_map<std::uint64_t, std::int32_t> frozen_ticks_;
    std::unordered_map<std::uint64_t, std::int32_t> no_gravity_;
    std::unordered_map<std::uint64_t, std::int32_t> silent_;
    std::unordered_map<std::uint64_t, std::int32_t> glowing_;
    std::unordered_map<std::uint64_t, std::int32_t> invisible_;
    std::unordered_map<std::uint64_t, std::int32_t> portal_cooldown_;
    std::unordered_map<std::uint64_t, std::int32_t> max_air_;
    std::unordered_map<std::uint32_t, std::int64_t> world_seeds_;
    std::unordered_map<
        std::uint32_t,
        std::tuple<std::int32_t, std::int32_t, std::int32_t>>
        spawns_;
    std::unordered_map<std::uint32_t, std::int64_t> world_times_;
};

} // namespace leaf
