#pragma once

#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>

#include "leaf/abi/leaf_bridge_v1.h"
#include "leaf/loader/mod_engine.hpp"
#include "leaf/minecraft/minecraft_abi.hpp"
#include "leaf/minecraft/stub_minecraft_abi.hpp"

namespace leaf {

/// Process-wide host used by all loader bridges. Not for Leaf Mods.
class LEAF_BRIDGE_API bridge_host {
public:
    [[nodiscard]] static bridge_host& instance();

    [[nodiscard]] status init(const LeafBridgeConfigV1& config);
    [[nodiscard]] status shutdown();

    [[nodiscard]] bool initialized() const noexcept;

    [[nodiscard]] status pump_main(std::uint32_t budget);
    [[nodiscard]] status fill_info(LeafBridgeInfoV1& out);

    [[nodiscard]] std::uint32_t loaded_mod_count() const noexcept;
    [[nodiscard]] status mod_id_at(
        std::uint32_t index,
        char* out_buf,
        std::uint32_t out_buf_size) const;

    [[nodiscard]] status on_server_starting();
    [[nodiscard]] status on_server_started();
    [[nodiscard]] status on_server_stopping();

    [[nodiscard]] status emit_player_join(std::uint64_t player);
    [[nodiscard]] status emit_player_leave(std::uint64_t player);
    [[nodiscard]] result<decision> emit_player_join_request(std::uint64_t player);
    [[nodiscard]] result<decision> emit_player_chat(
        std::uint64_t player,
        std::string_view message);

    [[nodiscard]] status emit_player_death(std::uint64_t player);

    [[nodiscard]] status emit_entity_spawn(
        std::uint64_t entity,
        std::uint32_t entity_type_id,
        std::int32_t x,
        std::int32_t y,
        std::int32_t z,
        std::uint32_t dimension);

    [[nodiscard]] status emit_entity_remove(
        std::uint64_t entity,
        std::uint32_t entity_type_id,
        std::int32_t x,
        std::int32_t y,
        std::int32_t z,
        std::uint32_t dimension);

    [[nodiscard]] status emit_world_load(std::uint32_t dimension);

    [[nodiscard]] result<decision> emit_block_break(
        std::uint64_t player,
        std::int32_t x,
        std::int32_t y,
        std::int32_t z,
        std::uint32_t block_id);

    [[nodiscard]] result<decision> emit_block_place(
        std::uint64_t player,
        std::int32_t x,
        std::int32_t y,
        std::int32_t z,
        std::uint32_t block_id);

    [[nodiscard]] status reload_mods();

    void set_send_player_message_hook(
        void (*fn)(std::uint64_t player_handle, const char* message));

    void set_broadcast_message_hook(void (*fn)(const char* message));

    void set_inventory_hooks(
        stub_minecraft_abi::inv_get_fn get_fn,
        stub_minecraft_abi::inv_set_fn set_fn);

    void set_inventory_stack_hooks(
        stub_minecraft_abi::inv_stack_get_fn get_fn,
        stub_minecraft_abi::inv_stack_set_fn set_fn);

    void set_get_block_hook(stub_minecraft_abi::block_get_fn fn);

    void set_world_hooks(
        stub_minecraft_abi::block_get_fn get_fn,
        stub_minecraft_abi::block_set_fn set_fn);

    void set_player_pos_hooks(
        stub_minecraft_abi::player_pos_get_fn get_fn,
        stub_minecraft_abi::player_pos_set_fn set_fn);

    void set_player_health_hooks(
        stub_minecraft_abi::player_health_get_fn get_fn,
        stub_minecraft_abi::player_health_set_fn set_fn);

    void set_player_food_hooks(
        stub_minecraft_abi::player_food_get_fn get_fn,
        stub_minecraft_abi::player_food_set_fn set_fn);

    void set_player_gamemode_hooks(
        stub_minecraft_abi::player_gamemode_get_fn get_fn,
        stub_minecraft_abi::player_gamemode_set_fn set_fn);

    void set_player_xp_hooks(
        stub_minecraft_abi::player_xp_get_fn get_fn,
        stub_minecraft_abi::player_xp_set_fn set_fn);

    void set_player_look_hooks(
        stub_minecraft_abi::player_look_get_fn get_fn,
        stub_minecraft_abi::player_look_set_fn set_fn);

    void set_play_sound_hook(stub_minecraft_abi::play_sound_fn fn);

    void set_actionbar_hook(stub_minecraft_abi::actionbar_fn fn);

    void set_title_hook(stub_minecraft_abi::title_fn fn);

    void set_kick_hook(stub_minecraft_abi::kick_fn fn);

    void set_give_item_hook(stub_minecraft_abi::give_item_fn fn);

    void set_effect_hooks(
        stub_minecraft_abi::apply_effect_fn apply_fn,
        stub_minecraft_abi::clear_effects_fn clear_fn);

    void set_spawn_particle_hook(stub_minecraft_abi::spawn_particle_fn fn);

    void set_world_time_hooks(
        stub_minecraft_abi::world_time_get_fn get_fn,
        stub_minecraft_abi::world_time_set_fn set_fn);

    void set_player_velocity_hooks(
        stub_minecraft_abi::player_velocity_get_fn get_fn,
        stub_minecraft_abi::player_velocity_set_fn set_fn);

    void set_player_flags_hook(stub_minecraft_abi::player_flags_fn fn);

    void set_run_command_hook(stub_minecraft_abi::run_command_fn fn);

    void set_clear_inventory_hook(stub_minecraft_abi::clear_inventory_fn fn);

    void set_player_flight_hook(stub_minecraft_abi::player_flight_fn fn);

    void set_get_biome_hook(stub_minecraft_abi::get_biome_fn fn);

    void set_difficulty_hooks(
        stub_minecraft_abi::difficulty_get_fn get_fn,
        stub_minecraft_abi::difficulty_set_fn set_fn);

    void set_weather_hooks(
        stub_minecraft_abi::weather_get_fn get_fn,
        stub_minecraft_abi::weather_set_fn set_fn);

    void set_get_light_level_hook(stub_minecraft_abi::get_light_level_fn fn);

    void set_player_latency_hook(stub_minecraft_abi::player_latency_fn fn);

    void set_world_spawn_hooks(
        stub_minecraft_abi::world_spawn_get_fn get_fn,
        stub_minecraft_abi::world_spawn_set_fn set_fn);

    void set_player_op_hook(stub_minecraft_abi::player_op_fn fn);

    void set_player_uuid_hook(stub_minecraft_abi::player_uuid_fn fn);

    void set_player_permission_hook(stub_minecraft_abi::player_permission_fn fn);

    void set_find_player_uuid_hook(stub_minecraft_abi::find_player_uuid_fn fn);

    void set_find_player_name_hook(stub_minecraft_abi::find_player_name_fn fn);

    void set_block_registry_hooks(
        stub_minecraft_abi::get_block_registry_fn get_fn,
        stub_minecraft_abi::set_block_registry_fn set_fn);

    void set_give_item_registry_hook(stub_minecraft_abi::give_item_registry_fn fn);

    void set_inventory_registry_hooks(
        stub_minecraft_abi::inv_registry_get_fn get_fn,
        stub_minecraft_abi::inv_registry_set_fn set_fn);

    void set_teleport_hook(stub_minecraft_abi::teleport_fn fn);

    void set_selected_slot_hooks(
        stub_minecraft_abi::selected_slot_get_fn get_fn,
        stub_minecraft_abi::selected_slot_set_fn set_fn);

    void set_get_world_seed_hook(stub_minecraft_abi::get_world_seed_fn fn);

    void set_player_absorption_hooks(
        stub_minecraft_abi::player_absorption_get_fn get_fn,
        stub_minecraft_abi::player_absorption_set_fn set_fn);

    void set_player_invulnerable_hooks(
        stub_minecraft_abi::player_invulnerable_get_fn get_fn,
        stub_minecraft_abi::player_invulnerable_set_fn set_fn);

    void set_player_air_hooks(
        stub_minecraft_abi::player_air_get_fn get_fn,
        stub_minecraft_abi::player_air_set_fn set_fn);

    void set_player_fire_ticks_hooks(
        stub_minecraft_abi::player_fire_ticks_get_fn get_fn,
        stub_minecraft_abi::player_fire_ticks_set_fn set_fn);

    void set_player_frozen_ticks_hooks(
        stub_minecraft_abi::player_frozen_ticks_get_fn get_fn,
        stub_minecraft_abi::player_frozen_ticks_set_fn set_fn);

    void set_player_no_gravity_hooks(
        stub_minecraft_abi::player_no_gravity_get_fn get_fn,
        stub_minecraft_abi::player_no_gravity_set_fn set_fn);

    void set_player_silent_hooks(
        stub_minecraft_abi::player_silent_get_fn get_fn,
        stub_minecraft_abi::player_silent_set_fn set_fn);

    void set_player_glowing_hooks(
        stub_minecraft_abi::player_glowing_get_fn get_fn,
        stub_minecraft_abi::player_glowing_set_fn set_fn);

    void set_player_invisible_hooks(
        stub_minecraft_abi::player_invisible_get_fn get_fn,
        stub_minecraft_abi::player_invisible_set_fn set_fn);

    void set_player_portal_cooldown_hooks(
        stub_minecraft_abi::player_portal_cooldown_get_fn get_fn,
        stub_minecraft_abi::player_portal_cooldown_set_fn set_fn);

    void set_get_player_max_air_hook(stub_minecraft_abi::get_player_max_air_fn fn);

    [[nodiscard]] status register_player(
        std::uint64_t player,
        std::string name);

    [[nodiscard]] status unregister_player(std::uint64_t player);

    [[nodiscard]] mod_engine* engine() noexcept { return engine_ ? &*engine_ : nullptr; }
    [[nodiscard]] minecraft_abi* minecraft() noexcept { return minecraft_.get(); }
    [[nodiscard]] stub_minecraft_abi* stub_minecraft() noexcept {
        return dynamic_cast<stub_minecraft_abi*>(minecraft_.get());
    }

private:
    bridge_host() = default;

    [[nodiscard]] capability_set capabilities_for_(
        const version& mc,
        std::uint32_t loader) const;

    mutable std::mutex mutex_;
    bool ready_{false};
    std::optional<mod_engine> engine_;
    std::unique_ptr<minecraft_abi> minecraft_;

    std::string minecraft_version_text_;
    version minecraft_version_{};
    std::uint32_t loader_{LEAF_LOADER_UNKNOWN};
    std::uint32_t mapping_{LEAF_MAPPING_UNKNOWN};
    std::filesystem::path leafmods_dir_;
    std::filesystem::path game_dir_;
    void (*send_hook_)(std::uint64_t, const char*){nullptr};
    void (*broadcast_hook_)(const char*){nullptr};
    stub_minecraft_abi::inv_get_fn inv_get_hook_{nullptr};
    stub_minecraft_abi::inv_set_fn inv_set_hook_{nullptr};
    stub_minecraft_abi::inv_stack_get_fn inv_stack_get_hook_{nullptr};
    stub_minecraft_abi::inv_stack_set_fn inv_stack_set_hook_{nullptr};
    stub_minecraft_abi::block_get_fn block_get_hook_{nullptr};
    stub_minecraft_abi::block_set_fn block_set_hook_{nullptr};
    stub_minecraft_abi::player_pos_get_fn player_pos_get_hook_{nullptr};
    stub_minecraft_abi::player_pos_set_fn player_pos_set_hook_{nullptr};
    stub_minecraft_abi::player_health_get_fn player_health_get_hook_{nullptr};
    stub_minecraft_abi::player_health_set_fn player_health_set_hook_{nullptr};
    stub_minecraft_abi::player_food_get_fn player_food_get_hook_{nullptr};
    stub_minecraft_abi::player_food_set_fn player_food_set_hook_{nullptr};
    stub_minecraft_abi::player_gamemode_get_fn player_gamemode_get_hook_{nullptr};
    stub_minecraft_abi::player_gamemode_set_fn player_gamemode_set_hook_{nullptr};
    stub_minecraft_abi::player_xp_get_fn player_xp_get_hook_{nullptr};
    stub_minecraft_abi::player_xp_set_fn player_xp_set_hook_{nullptr};
    stub_minecraft_abi::player_look_get_fn player_look_get_hook_{nullptr};
    stub_minecraft_abi::player_look_set_fn player_look_set_hook_{nullptr};
    stub_minecraft_abi::play_sound_fn play_sound_hook_{nullptr};
    stub_minecraft_abi::actionbar_fn actionbar_hook_{nullptr};
    stub_minecraft_abi::title_fn title_hook_{nullptr};
    stub_minecraft_abi::kick_fn kick_hook_{nullptr};
    stub_minecraft_abi::give_item_fn give_item_hook_{nullptr};
    stub_minecraft_abi::apply_effect_fn apply_effect_hook_{nullptr};
    stub_minecraft_abi::clear_effects_fn clear_effects_hook_{nullptr};
    stub_minecraft_abi::spawn_particle_fn spawn_particle_hook_{nullptr};
    stub_minecraft_abi::world_time_get_fn world_time_get_hook_{nullptr};
    stub_minecraft_abi::world_time_set_fn world_time_set_hook_{nullptr};
    stub_minecraft_abi::player_velocity_get_fn player_velocity_get_hook_{nullptr};
    stub_minecraft_abi::player_velocity_set_fn player_velocity_set_hook_{nullptr};
    stub_minecraft_abi::player_flags_fn player_flags_hook_{nullptr};
    stub_minecraft_abi::run_command_fn run_command_hook_{nullptr};
    stub_minecraft_abi::clear_inventory_fn clear_inventory_hook_{nullptr};
    stub_minecraft_abi::player_flight_fn player_flight_hook_{nullptr};
    stub_minecraft_abi::get_biome_fn get_biome_hook_{nullptr};
    stub_minecraft_abi::difficulty_get_fn difficulty_get_hook_{nullptr};
    stub_minecraft_abi::difficulty_set_fn difficulty_set_hook_{nullptr};
    stub_minecraft_abi::weather_get_fn weather_get_hook_{nullptr};
    stub_minecraft_abi::weather_set_fn weather_set_hook_{nullptr};
    stub_minecraft_abi::get_light_level_fn get_light_level_hook_{nullptr};
    stub_minecraft_abi::player_latency_fn player_latency_hook_{nullptr};
    stub_minecraft_abi::world_spawn_get_fn world_spawn_get_hook_{nullptr};
    stub_minecraft_abi::world_spawn_set_fn world_spawn_set_hook_{nullptr};
    stub_minecraft_abi::player_op_fn player_op_hook_{nullptr};
    stub_minecraft_abi::player_uuid_fn player_uuid_hook_{nullptr};
    stub_minecraft_abi::player_permission_fn player_permission_hook_{nullptr};
    stub_minecraft_abi::find_player_uuid_fn find_player_uuid_hook_{nullptr};
    stub_minecraft_abi::find_player_name_fn find_player_name_hook_{nullptr};
    stub_minecraft_abi::get_block_registry_fn get_block_registry_hook_{nullptr};
    stub_minecraft_abi::set_block_registry_fn set_block_registry_hook_{nullptr};
    stub_minecraft_abi::give_item_registry_fn give_item_registry_hook_{nullptr};
    stub_minecraft_abi::inv_registry_get_fn inv_registry_get_hook_{nullptr};
    stub_minecraft_abi::inv_registry_set_fn inv_registry_set_hook_{nullptr};
    stub_minecraft_abi::teleport_fn teleport_hook_{nullptr};
    stub_minecraft_abi::selected_slot_get_fn selected_slot_get_hook_{nullptr};
    stub_minecraft_abi::selected_slot_set_fn selected_slot_set_hook_{nullptr};
    stub_minecraft_abi::get_world_seed_fn get_world_seed_hook_{nullptr};
    stub_minecraft_abi::player_absorption_get_fn player_absorption_get_hook_{nullptr};
    stub_minecraft_abi::player_absorption_set_fn player_absorption_set_hook_{nullptr};
    stub_minecraft_abi::player_invulnerable_get_fn player_invulnerable_get_hook_{nullptr};
    stub_minecraft_abi::player_invulnerable_set_fn player_invulnerable_set_hook_{nullptr};
    stub_minecraft_abi::player_air_get_fn player_air_get_hook_{nullptr};
    stub_minecraft_abi::player_air_set_fn player_air_set_hook_{nullptr};
    stub_minecraft_abi::player_fire_ticks_get_fn player_fire_ticks_get_hook_{nullptr};
    stub_minecraft_abi::player_fire_ticks_set_fn player_fire_ticks_set_hook_{nullptr};
    stub_minecraft_abi::player_frozen_ticks_get_fn player_frozen_ticks_get_hook_{nullptr};
    stub_minecraft_abi::player_frozen_ticks_set_fn player_frozen_ticks_set_hook_{nullptr};
    stub_minecraft_abi::player_no_gravity_get_fn player_no_gravity_get_hook_{nullptr};
    stub_minecraft_abi::player_no_gravity_set_fn player_no_gravity_set_hook_{nullptr};
    stub_minecraft_abi::player_silent_get_fn player_silent_get_hook_{nullptr};
    stub_minecraft_abi::player_silent_set_fn player_silent_set_hook_{nullptr};
    stub_minecraft_abi::player_glowing_get_fn player_glowing_get_hook_{nullptr};
    stub_minecraft_abi::player_glowing_set_fn player_glowing_set_hook_{nullptr};
    stub_minecraft_abi::player_invisible_get_fn player_invisible_get_hook_{nullptr};
    stub_minecraft_abi::player_invisible_set_fn player_invisible_set_hook_{nullptr};
    stub_minecraft_abi::player_portal_cooldown_get_fn
        player_portal_cooldown_get_hook_{nullptr};
    stub_minecraft_abi::player_portal_cooldown_set_fn
        player_portal_cooldown_set_hook_{nullptr};
    stub_minecraft_abi::get_player_max_air_fn get_player_max_air_hook_{nullptr};
};

} // namespace leaf
