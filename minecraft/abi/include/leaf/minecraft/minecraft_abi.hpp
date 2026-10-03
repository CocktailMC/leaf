#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

#include "leaf/abi/leaf_abi_v1.h"
#include "leaf/core/capability.hpp"
#include "leaf/core/error.hpp"
#include "leaf/core/result.hpp"
#include "leaf/core/version.hpp"
#include "leaf/object/handle.hpp"

namespace leaf {

enum class loader_kind : std::uint8_t {
    unknown = 0,
    fabric = 1,
    forge = 2,
    neoforge = 3,
};

/// Per-version Minecraft operations. Leaf Mods never see this — only Leaf ABI.
class minecraft_abi {
public:
    virtual ~minecraft_abi() = default;

    [[nodiscard]] virtual version minecraft_version() const noexcept = 0;
    [[nodiscard]] virtual loader_kind loader() const noexcept = 0;
    [[nodiscard]] virtual capability_set capabilities() const noexcept = 0;

    [[nodiscard]] virtual result<server_handle> get_server() = 0;

    [[nodiscard]] virtual status send_player_message(
        player_handle player,
        std::string_view message) = 0;

    [[nodiscard]] virtual status broadcast_message(std::string_view message) = 0;

    [[nodiscard]] virtual std::uint32_t player_count() const noexcept = 0;

    /// Index into the current online player list (0 .. player_count-1).
    [[nodiscard]] virtual status get_player_at(
        std::uint32_t index,
        player_handle& out_player) {
        (void)index;
        (void)out_player;
        return err(ec::not_supported, "player enumeration not available");
    }

    [[nodiscard]] virtual result<std::string> player_name(player_handle player) = 0;

    /// Slot indices follow vanilla player inventory (0–40). Default: not supported.
    [[nodiscard]] virtual status get_inventory_slot(
        player_handle player,
        std::uint32_t slot,
        std::uint32_t& out_item_id,
        std::uint32_t& out_count) {
        (void)player;
        (void)slot;
        (void)out_item_id;
        (void)out_count;
        return err(ec::not_supported, "inventory not available");
    }

    [[nodiscard]] virtual status set_inventory_slot(
        player_handle player,
        std::uint32_t slot,
        std::uint32_t item_id,
        std::uint32_t count) {
        (void)player;
        (void)slot;
        (void)item_id;
        (void)count;
        return err(ec::not_supported, "inventory not available");
    }

    [[nodiscard]] virtual status get_inventory_stack(
        player_handle player,
        std::uint32_t slot,
        LeafItemStackV1& out) {
        std::uint32_t id = 0;
        std::uint32_t count = 0;
        auto st = get_inventory_slot(player, slot, id, count);
        if (!st) {
            return st;
        }
        out.struct_size = static_cast<std::uint32_t>(sizeof(LeafItemStackV1));
        out.item_id = id;
        out.count = count;
        out.damage = -1;
        return ok();
    }

    [[nodiscard]] virtual status set_inventory_stack(
        player_handle player,
        std::uint32_t slot,
        const LeafItemStackV1& stack) {
        return set_inventory_slot(player, slot, stack.item_id, stack.count);
    }

    [[nodiscard]] virtual status get_block(
        std::int32_t x,
        std::int32_t y,
        std::int32_t z,
        std::uint32_t& out_block_id) {
        (void)x;
        (void)y;
        (void)z;
        (void)out_block_id;
        return err(ec::not_supported, "world not available");
    }

    [[nodiscard]] virtual status set_block(
        std::int32_t x,
        std::int32_t y,
        std::int32_t z,
        std::uint32_t block_id) {
        (void)x;
        (void)y;
        (void)z;
        (void)block_id;
        return err(ec::not_supported, "world not available");
    }

    /// dimension: 0 overworld, 1 nether, 2 end
    [[nodiscard]] virtual status get_block_dim(
        std::uint32_t dimension,
        std::int32_t x,
        std::int32_t y,
        std::int32_t z,
        std::uint32_t& out_block_id) {
        if (dimension != 0) {
            return err(ec::not_supported, "dimension not available");
        }
        return get_block(x, y, z, out_block_id);
    }

    [[nodiscard]] virtual status set_block_dim(
        std::uint32_t dimension,
        std::int32_t x,
        std::int32_t y,
        std::int32_t z,
        std::uint32_t block_id) {
        if (dimension != 0) {
            return err(ec::not_supported, "dimension not available");
        }
        return set_block(x, y, z, block_id);
    }

    /// Block-rounded player position + dimension (0/1/2). Default: not supported.
    [[nodiscard]] virtual status get_player_pos(
        player_handle player,
        std::int32_t& out_x,
        std::int32_t& out_y,
        std::int32_t& out_z,
        std::uint32_t& out_dimension) {
        (void)player;
        (void)out_x;
        (void)out_y;
        (void)out_z;
        (void)out_dimension;
        return err(ec::not_supported, "player position not available");
    }

    [[nodiscard]] virtual status set_player_pos(
        player_handle player,
        std::int32_t x,
        std::int32_t y,
        std::int32_t z,
        std::uint32_t dimension) {
        (void)player;
        (void)x;
        (void)y;
        (void)z;
        (void)dimension;
        return err(ec::not_supported, "player teleport not available");
    }

    [[nodiscard]] virtual status get_player_health(
        player_handle player,
        float& out_health,
        float& out_max_health) {
        (void)player;
        (void)out_health;
        (void)out_max_health;
        return err(ec::not_supported, "player health not available");
    }

    [[nodiscard]] virtual status set_player_health(
        player_handle player,
        float health) {
        (void)player;
        (void)health;
        return err(ec::not_supported, "player health not available");
    }

    [[nodiscard]] virtual status get_player_food(
        player_handle player,
        std::int32_t& out_food,
        float& out_saturation) {
        (void)player;
        (void)out_food;
        (void)out_saturation;
        return err(ec::not_supported, "player food not available");
    }

    [[nodiscard]] virtual status set_player_food(
        player_handle player,
        std::int32_t food,
        float saturation) {
        (void)player;
        (void)food;
        (void)saturation;
        return err(ec::not_supported, "player food not available");
    }

    /// 0 survival, 1 creative, 2 adventure, 3 spectator
    [[nodiscard]] virtual status get_player_gamemode(
        player_handle player,
        std::uint32_t& out_mode) {
        (void)player;
        (void)out_mode;
        return err(ec::not_supported, "player gamemode not available");
    }

    [[nodiscard]] virtual status set_player_gamemode(
        player_handle player,
        std::uint32_t mode) {
        (void)player;
        (void)mode;
        return err(ec::not_supported, "player gamemode not available");
    }

    [[nodiscard]] virtual status get_player_xp(
        player_handle player,
        std::int32_t& out_level,
        float& out_progress) {
        (void)player;
        (void)out_level;
        (void)out_progress;
        return err(ec::not_supported, "player xp not available");
    }

    [[nodiscard]] virtual status set_player_xp_level(
        player_handle player,
        std::int32_t level) {
        (void)player;
        (void)level;
        return err(ec::not_supported, "player xp not available");
    }

    [[nodiscard]] virtual status get_player_look(
        player_handle player,
        float& out_yaw,
        float& out_pitch) {
        (void)player;
        (void)out_yaw;
        (void)out_pitch;
        return err(ec::not_supported, "player look not available");
    }

    [[nodiscard]] virtual status set_player_look(
        player_handle player,
        float yaw,
        float pitch) {
        (void)player;
        (void)yaw;
        (void)pitch;
        return err(ec::not_supported, "player look not available");
    }

    [[nodiscard]] virtual status play_sound(
        player_handle player,
        std::string_view sound_id,
        float volume,
        float pitch,
        std::int32_t x,
        std::int32_t y,
        std::int32_t z,
        std::uint32_t dimension) {
        (void)player;
        (void)sound_id;
        (void)volume;
        (void)pitch;
        (void)x;
        (void)y;
        (void)z;
        (void)dimension;
        return err(ec::not_supported, "play_sound not available");
    }

    [[nodiscard]] virtual status send_actionbar(
        player_handle player,
        std::string_view message) {
        (void)player;
        (void)message;
        return err(ec::not_supported, "actionbar not available");
    }

    [[nodiscard]] virtual status send_title(
        player_handle player,
        std::string_view title,
        std::string_view subtitle,
        std::int32_t fade_in_ticks,
        std::int32_t stay_ticks,
        std::int32_t fade_out_ticks) {
        (void)player;
        (void)title;
        (void)subtitle;
        (void)fade_in_ticks;
        (void)stay_ticks;
        (void)fade_out_ticks;
        return err(ec::not_supported, "title not available");
    }

    [[nodiscard]] virtual status kick_player(
        player_handle player,
        std::string_view reason) {
        (void)player;
        (void)reason;
        return err(ec::not_supported, "kick_player not available");
    }

    [[nodiscard]] virtual status give_item(
        player_handle player,
        std::uint32_t item_id,
        std::uint32_t count,
        std::int32_t damage) {
        (void)player;
        (void)item_id;
        (void)count;
        (void)damage;
        return err(ec::not_supported, "give_item not available");
    }

    [[nodiscard]] virtual status apply_effect(
        player_handle player,
        std::string_view effect_id,
        std::int32_t duration_ticks,
        std::int32_t amplifier,
        std::uint32_t flags) {
        (void)player;
        (void)effect_id;
        (void)duration_ticks;
        (void)amplifier;
        (void)flags;
        return err(ec::not_supported, "apply_effect not available");
    }

    [[nodiscard]] virtual status clear_effects(player_handle player) {
        (void)player;
        return err(ec::not_supported, "clear_effects not available");
    }

    [[nodiscard]] virtual status spawn_particle(
        std::string_view particle_id,
        double x,
        double y,
        double z,
        std::uint32_t dimension,
        std::uint32_t count,
        double dx,
        double dy,
        double dz,
        double speed) {
        (void)particle_id;
        (void)x;
        (void)y;
        (void)z;
        (void)dimension;
        (void)count;
        (void)dx;
        (void)dy;
        (void)dz;
        (void)speed;
        return err(ec::not_supported, "spawn_particle not available");
    }

    [[nodiscard]] virtual status get_world_time(
        std::uint32_t dimension,
        std::int64_t& out_time) {
        (void)dimension;
        (void)out_time;
        return err(ec::not_supported, "world time not available");
    }

    [[nodiscard]] virtual status set_world_time(
        std::uint32_t dimension,
        std::int64_t time) {
        (void)dimension;
        (void)time;
        return err(ec::not_supported, "world time not available");
    }

    [[nodiscard]] virtual status get_player_velocity(
        player_handle player,
        double& out_vx,
        double& out_vy,
        double& out_vz) {
        (void)player;
        (void)out_vx;
        (void)out_vy;
        (void)out_vz;
        return err(ec::not_supported, "player velocity not available");
    }

    [[nodiscard]] virtual status set_player_velocity(
        player_handle player,
        double vx,
        double vy,
        double vz) {
        (void)player;
        (void)vx;
        (void)vy;
        (void)vz;
        return err(ec::not_supported, "player velocity not available");
    }

    [[nodiscard]] virtual status get_player_flags(
        player_handle player,
        std::uint32_t& out_flags) {
        (void)player;
        (void)out_flags;
        return err(ec::not_supported, "player flags not available");
    }

    [[nodiscard]] virtual status run_command(
        player_handle player,
        std::string_view command) {
        (void)player;
        (void)command;
        return err(ec::not_supported, "run_command not available");
    }

    [[nodiscard]] virtual status clear_inventory(player_handle player) {
        (void)player;
        return err(ec::not_supported, "clear_inventory not available");
    }

    [[nodiscard]] virtual status broadcast_actionbar(std::string_view message) {
        (void)message;
        return err(ec::not_supported, "broadcast_actionbar not available");
    }

    [[nodiscard]] virtual status broadcast_title(
        std::string_view title,
        std::string_view subtitle,
        std::int32_t fade_in_ticks,
        std::int32_t stay_ticks,
        std::int32_t fade_out_ticks) {
        (void)title;
        (void)subtitle;
        (void)fade_in_ticks;
        (void)stay_ticks;
        (void)fade_out_ticks;
        return err(ec::not_supported, "broadcast_title not available");
    }

    /// allow_flight/flying: 0=off, 1=on, -1=leave unchanged.
    [[nodiscard]] virtual status set_player_flight(
        player_handle player,
        std::int32_t allow_flight,
        std::int32_t flying) {
        (void)player;
        (void)allow_flight;
        (void)flying;
        return err(ec::not_supported, "set_player_flight not available");
    }

    [[nodiscard]] virtual status get_biome(
        std::uint32_t dimension,
        std::int32_t x,
        std::int32_t y,
        std::int32_t z,
        std::string& out_id) {
        (void)dimension;
        (void)x;
        (void)y;
        (void)z;
        (void)out_id;
        return err(ec::not_supported, "get_biome not available");
    }

    [[nodiscard]] virtual status get_difficulty(std::uint32_t& out_difficulty) {
        (void)out_difficulty;
        return err(ec::not_supported, "get_difficulty not available");
    }

    [[nodiscard]] virtual status set_difficulty(std::uint32_t difficulty) {
        (void)difficulty;
        return err(ec::not_supported, "set_difficulty not available");
    }

    [[nodiscard]] virtual status get_weather(
        std::uint32_t dimension,
        std::uint32_t& out_weather) {
        (void)dimension;
        (void)out_weather;
        return err(ec::not_supported, "get_weather not available");
    }

    [[nodiscard]] virtual status set_weather(
        std::uint32_t dimension,
        std::uint32_t weather,
        std::int32_t duration_ticks) {
        (void)dimension;
        (void)weather;
        (void)duration_ticks;
        return err(ec::not_supported, "set_weather not available");
    }

    [[nodiscard]] virtual status get_light_level(
        std::uint32_t dimension,
        std::int32_t x,
        std::int32_t y,
        std::int32_t z,
        std::uint32_t& out_block_light,
        std::uint32_t& out_sky_light) {
        (void)dimension;
        (void)x;
        (void)y;
        (void)z;
        (void)out_block_light;
        (void)out_sky_light;
        return err(ec::not_supported, "get_light_level not available");
    }

    [[nodiscard]] virtual status get_player_latency(
        player_handle player,
        std::int32_t& out_ms) {
        (void)player;
        (void)out_ms;
        return err(ec::not_supported, "get_player_latency not available");
    }

    [[nodiscard]] virtual status get_world_spawn(
        std::uint32_t dimension,
        std::int32_t& out_x,
        std::int32_t& out_y,
        std::int32_t& out_z) {
        (void)dimension;
        (void)out_x;
        (void)out_y;
        (void)out_z;
        return err(ec::not_supported, "get_world_spawn not available");
    }

    [[nodiscard]] virtual status set_world_spawn(
        std::uint32_t dimension,
        std::int32_t x,
        std::int32_t y,
        std::int32_t z) {
        (void)dimension;
        (void)x;
        (void)y;
        (void)z;
        return err(ec::not_supported, "set_world_spawn not available");
    }

    [[nodiscard]] virtual status is_player_op(
        player_handle player,
        std::int32_t& out_op) {
        (void)player;
        (void)out_op;
        return err(ec::not_supported, "is_player_op not available");
    }

    [[nodiscard]] virtual status get_player_uuid(
        player_handle player,
        std::string& out_uuid) {
        (void)player;
        (void)out_uuid;
        return err(ec::not_supported, "get_player_uuid not available");
    }

    [[nodiscard]] virtual status get_player_permission_level(
        player_handle player,
        std::int32_t& out_level) {
        (void)player;
        (void)out_level;
        return err(ec::not_supported, "get_player_permission_level not available");
    }

    [[nodiscard]] virtual status find_player_by_uuid(
        std::string_view uuid,
        player_handle& out_player) {
        (void)uuid;
        (void)out_player;
        return err(ec::not_supported, "find_player_by_uuid not available");
    }

    [[nodiscard]] virtual status find_player_by_name(
        std::string_view name,
        player_handle& out_player) {
        (void)name;
        (void)out_player;
        return err(ec::not_supported, "find_player_by_name not available");
    }

    [[nodiscard]] virtual status get_block_registry_id(
        std::uint32_t dimension,
        std::int32_t x,
        std::int32_t y,
        std::int32_t z,
        std::string& out_id) {
        (void)dimension;
        (void)x;
        (void)y;
        (void)z;
        (void)out_id;
        return err(ec::not_supported, "get_block_registry_id not available");
    }

    [[nodiscard]] virtual status set_block_registry_id(
        std::uint32_t dimension,
        std::int32_t x,
        std::int32_t y,
        std::int32_t z,
        std::string_view block_id) {
        (void)dimension;
        (void)x;
        (void)y;
        (void)z;
        (void)block_id;
        return err(ec::not_supported, "set_block_registry_id not available");
    }

    [[nodiscard]] virtual status give_item_registry_id(
        player_handle player,
        std::string_view item_id,
        std::uint32_t count,
        std::int32_t damage) {
        (void)player;
        (void)item_id;
        (void)count;
        (void)damage;
        return err(ec::not_supported, "give_item_registry_id not available");
    }

    [[nodiscard]] virtual status get_inventory_item_registry_id(
        player_handle player,
        std::uint32_t slot,
        std::string& out_id,
        std::uint32_t& out_count,
        std::int32_t& out_damage) {
        (void)player;
        (void)slot;
        (void)out_id;
        (void)out_count;
        (void)out_damage;
        return err(ec::not_supported, "get_inventory_item_registry_id not available");
    }

    [[nodiscard]] virtual status set_inventory_item_registry_id(
        player_handle player,
        std::uint32_t slot,
        std::string_view item_id,
        std::uint32_t count,
        std::int32_t damage) {
        (void)player;
        (void)slot;
        (void)item_id;
        (void)count;
        (void)damage;
        return err(ec::not_supported, "set_inventory_item_registry_id not available");
    }

    [[nodiscard]] virtual status teleport_player(
        player_handle player,
        std::int32_t x,
        std::int32_t y,
        std::int32_t z,
        std::uint32_t dimension,
        float yaw,
        float pitch) {
        (void)player;
        (void)x;
        (void)y;
        (void)z;
        (void)dimension;
        (void)yaw;
        (void)pitch;
        return err(ec::not_supported, "teleport_player not available");
    }

    [[nodiscard]] virtual status get_selected_slot(
        player_handle player,
        std::uint32_t& out_slot) {
        (void)player;
        (void)out_slot;
        return err(ec::not_supported, "get_selected_slot not available");
    }

    [[nodiscard]] virtual status set_selected_slot(
        player_handle player,
        std::uint32_t slot) {
        (void)player;
        (void)slot;
        return err(ec::not_supported, "set_selected_slot not available");
    }

    /// Held (selected hotbar) item — composes selected_slot + inventory registry.
    [[nodiscard]] virtual status get_held_item_registry_id(
        player_handle player,
        std::string& out_id,
        std::uint32_t& out_count,
        std::int32_t& out_damage) {
        std::uint32_t slot = 0;
        auto st = get_selected_slot(player, slot);
        if (!st) {
            return st;
        }
        return get_inventory_item_registry_id(
            player, slot, out_id, out_count, out_damage);
    }

    [[nodiscard]] virtual status set_held_item_registry_id(
        player_handle player,
        std::string_view item_id,
        std::uint32_t count,
        std::int32_t damage) {
        std::uint32_t slot = 0;
        auto st = get_selected_slot(player, slot);
        if (!st) {
            return st;
        }
        return set_inventory_item_registry_id(
            player, slot, item_id, count, damage);
    }

    /// Map LEAF_EQUIP_* → inventory slot (or selected for mainhand).
    [[nodiscard]] virtual status resolve_equipment_inventory_slot(
        player_handle player,
        std::uint32_t equip_slot,
        std::uint32_t& out_inv_slot) {
        switch (equip_slot) {
            case LEAF_EQUIP_MAINHAND: {
                return get_selected_slot(player, out_inv_slot);
            }
            case LEAF_EQUIP_OFFHAND:
                out_inv_slot = 40;
                return ok();
            case LEAF_EQUIP_FEET:
                out_inv_slot = 36;
                return ok();
            case LEAF_EQUIP_LEGS:
                out_inv_slot = 37;
                return ok();
            case LEAF_EQUIP_CHEST:
                out_inv_slot = 38;
                return ok();
            case LEAF_EQUIP_HEAD:
                out_inv_slot = 39;
                return ok();
            default:
                return err(ec::invalid_argument, "unknown equipment slot");
        }
    }

    [[nodiscard]] virtual status get_equipment_item_registry_id(
        player_handle player,
        std::uint32_t equip_slot,
        std::string& out_id,
        std::uint32_t& out_count,
        std::int32_t& out_damage) {
        std::uint32_t inv = 0;
        auto st = resolve_equipment_inventory_slot(player, equip_slot, inv);
        if (!st) {
            return st;
        }
        return get_inventory_item_registry_id(
            player, inv, out_id, out_count, out_damage);
    }

    [[nodiscard]] virtual status set_equipment_item_registry_id(
        player_handle player,
        std::uint32_t equip_slot,
        std::string_view item_id,
        std::uint32_t count,
        std::int32_t damage) {
        std::uint32_t inv = 0;
        auto st = resolve_equipment_inventory_slot(player, equip_slot, inv);
        if (!st) {
            return st;
        }
        return set_inventory_item_registry_id(
            player, inv, item_id, count, damage);
    }

    [[nodiscard]] virtual status get_world_seed(
        std::uint32_t dimension,
        std::int64_t& out_seed) {
        (void)dimension;
        (void)out_seed;
        return err(ec::not_supported, "get_world_seed not available");
    }

    [[nodiscard]] virtual status heal_player(player_handle player) {
        float health = 0.0f;
        float max_health = 0.0f;
        auto st = get_player_health(player, health, max_health);
        if (!st) {
            return st;
        }
        return set_player_health(player, max_health);
    }

    [[nodiscard]] virtual status get_player_absorption(
        player_handle player,
        float& out_absorption) {
        (void)player;
        (void)out_absorption;
        return err(ec::not_supported, "get_player_absorption not available");
    }

    [[nodiscard]] virtual status set_player_absorption(
        player_handle player,
        float absorption) {
        (void)player;
        (void)absorption;
        return err(ec::not_supported, "set_player_absorption not available");
    }

    [[nodiscard]] virtual status get_player_invulnerable(
        player_handle player,
        std::int32_t& out_invulnerable) {
        (void)player;
        (void)out_invulnerable;
        return err(ec::not_supported, "get_player_invulnerable not available");
    }

    [[nodiscard]] virtual status set_player_invulnerable(
        player_handle player,
        std::int32_t invulnerable) {
        (void)player;
        (void)invulnerable;
        return err(ec::not_supported, "set_player_invulnerable not available");
    }

    [[nodiscard]] virtual status get_player_air(
        player_handle player,
        std::int32_t& out_air) {
        (void)player;
        (void)out_air;
        return err(ec::not_supported, "get_player_air not available");
    }

    [[nodiscard]] virtual status set_player_air(
        player_handle player,
        std::int32_t air) {
        (void)player;
        (void)air;
        return err(ec::not_supported, "set_player_air not available");
    }

    [[nodiscard]] virtual status get_player_fire_ticks(
        player_handle player,
        std::int32_t& out_ticks) {
        (void)player;
        (void)out_ticks;
        return err(ec::not_supported, "get_player_fire_ticks not available");
    }

    [[nodiscard]] virtual status set_player_fire_ticks(
        player_handle player,
        std::int32_t ticks) {
        (void)player;
        (void)ticks;
        return err(ec::not_supported, "set_player_fire_ticks not available");
    }

    [[nodiscard]] virtual status get_player_frozen_ticks(
        player_handle player,
        std::int32_t& out_ticks) {
        (void)player;
        (void)out_ticks;
        return err(ec::not_supported, "get_player_frozen_ticks not available");
    }

    [[nodiscard]] virtual status set_player_frozen_ticks(
        player_handle player,
        std::int32_t ticks) {
        (void)player;
        (void)ticks;
        return err(ec::not_supported, "set_player_frozen_ticks not available");
    }

    [[nodiscard]] virtual status extinguish_player(player_handle player) {
        return set_player_fire_ticks(player, 0);
    }

    [[nodiscard]] virtual status unfreeze_player(player_handle player) {
        return set_player_frozen_ticks(player, 0);
    }

    [[nodiscard]] virtual status get_player_no_gravity(
        player_handle player,
        std::int32_t& out_no_gravity) {
        (void)player;
        (void)out_no_gravity;
        return err(ec::not_supported, "get_player_no_gravity not available");
    }

    [[nodiscard]] virtual status set_player_no_gravity(
        player_handle player,
        std::int32_t no_gravity) {
        (void)player;
        (void)no_gravity;
        return err(ec::not_supported, "set_player_no_gravity not available");
    }

    [[nodiscard]] virtual status get_player_silent(
        player_handle player,
        std::int32_t& out_silent) {
        (void)player;
        (void)out_silent;
        return err(ec::not_supported, "get_player_silent not available");
    }

    [[nodiscard]] virtual status set_player_silent(
        player_handle player,
        std::int32_t silent) {
        (void)player;
        (void)silent;
        return err(ec::not_supported, "set_player_silent not available");
    }

    [[nodiscard]] virtual status get_player_glowing(
        player_handle player,
        std::int32_t& out_glowing) {
        (void)player;
        (void)out_glowing;
        return err(ec::not_supported, "get_player_glowing not available");
    }

    [[nodiscard]] virtual status set_player_glowing(
        player_handle player,
        std::int32_t glowing) {
        (void)player;
        (void)glowing;
        return err(ec::not_supported, "set_player_glowing not available");
    }

    [[nodiscard]] virtual status get_player_invisible(
        player_handle player,
        std::int32_t& out_invisible) {
        (void)player;
        (void)out_invisible;
        return err(ec::not_supported, "get_player_invisible not available");
    }

    [[nodiscard]] virtual status set_player_invisible(
        player_handle player,
        std::int32_t invisible) {
        (void)player;
        (void)invisible;
        return err(ec::not_supported, "set_player_invisible not available");
    }

    [[nodiscard]] virtual status get_player_portal_cooldown(
        player_handle player,
        std::int32_t& out_ticks) {
        (void)player;
        (void)out_ticks;
        return err(ec::not_supported, "get_player_portal_cooldown not available");
    }

    [[nodiscard]] virtual status set_player_portal_cooldown(
        player_handle player,
        std::int32_t ticks) {
        (void)player;
        (void)ticks;
        return err(ec::not_supported, "set_player_portal_cooldown not available");
    }

    [[nodiscard]] virtual status get_player_max_air(
        player_handle player,
        std::int32_t& out_max_air) {
        (void)player;
        (void)out_max_air;
        return err(ec::not_supported, "get_player_max_air not available");
    }

    [[nodiscard]] virtual status refill_player_air(player_handle player) {
        std::int32_t max_air = 0;
        auto st = get_player_max_air(player, max_air);
        if (!st) {
            return st;
        }
        return set_player_air(player, max_air);
    }

    /// Alive if health > 0 (composes get_player_health). out_alive is 0/1.
    [[nodiscard]] virtual status is_player_alive(
        player_handle player,
        std::int32_t& out_alive) {
        float health = 0.0f;
        float max_health = 0.0f;
        auto st = get_player_health(player, health, max_health);
        if (!st) {
            return st;
        }
        out_alive = health > 0.0f ? 1 : 0;
        return {};
    }
};

/// Select an ABI implementation for the running game.
[[nodiscard]] result<std::unique_ptr<minecraft_abi>> create_minecraft_abi(
    version minecraft,
    loader_kind loader);

} // namespace leaf
