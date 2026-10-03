#pragma once

#include "leaf/sdk/events.hpp"
#include "leaf/sdk/status.hpp"

#include "leaf/abi/leaf_abi_v1.h"

#include <cstdint>
#include <string_view>
#include <utility>

namespace leaf::sdk {

/// RAII subscription id returned by {@link api::subscribe}.
class subscription {
public:
    subscription() = default;

    subscription(const LeafApiV1* raw_api, std::uint64_t id) noexcept
        : api_(raw_api)
        , id_(id) {}

    subscription(const subscription&) = delete;
    subscription& operator=(const subscription&) = delete;

    subscription(subscription&& other) noexcept
        : api_(std::exchange(other.api_, nullptr))
        , id_(std::exchange(other.id_, 0)) {}

    subscription& operator=(subscription&& other) noexcept {
        if (this != &other) {
            reset();
            api_ = std::exchange(other.api_, nullptr);
            id_ = std::exchange(other.id_, 0);
        }
        return *this;
    }

    ~subscription() { reset(); }

    [[nodiscard]] bool active() const noexcept { return api_ != nullptr && id_ != 0; }

    [[nodiscard]] std::uint64_t id() const noexcept { return id_; }

    void reset() noexcept {
        if (api_ && id_ != 0 && api_->unsubscribe_event) {
            (void)api_->unsubscribe_event(id_);
        }
        api_ = nullptr;
        id_ = 0;
    }

    /// Detach without unsubscribing (engine owns lifetime across disable).
    std::uint64_t release() noexcept {
        api_ = nullptr;
        return std::exchange(id_, 0);
    }

private:
    const LeafApiV1* api_{nullptr};
    std::uint64_t id_{0};
};

/// C++23 façade over {@link LeafApiV1}. Never owns the engine.
class api {
public:
    explicit api(const LeafApiV1* raw) noexcept : raw_(raw) {}

    [[nodiscard]] const LeafApiV1* raw() const noexcept { return raw_; }

    [[nodiscard]] bool valid() const noexcept { return raw_ != nullptr; }

    [[nodiscard]] std::uint32_t abi_major() const noexcept {
        return raw_ ? raw_->abi_major : 0;
    }

    [[nodiscard]] std::uint32_t abi_minor() const noexcept {
        return raw_ ? raw_->abi_minor : 0;
    }

    void log(int level, std::string_view message) const {
        if (!raw_ || !raw_->log) {
            return;
        }
        // Leaf C ABI requires a NUL-terminated C string; callers must pass
        // literals or null-terminated buffers (string_view::data alone is OK
        // when the view covers a full C string).
        raw_->log(level, message.data());
    }

    void info(std::string_view message) const { log(LEAF_LOG_INFO, message); }
    void warn(std::string_view message) const { log(LEAF_LOG_WARN, message); }
    void error(std::string_view message) const { log(LEAF_LOG_ERROR, message); }

    [[nodiscard]] bool has_capability(LeafCapability cap) const noexcept {
        if (!raw_ || !raw_->has_capability) {
            return false;
        }
        return raw_->has_capability(static_cast<std::uint32_t>(cap)) != 0;
    }

    [[nodiscard]] LeafHandle get_server() const noexcept {
        if (!raw_ || !raw_->get_server) {
            return 0;
        }
        return raw_->get_server();
    }

    [[nodiscard]] status send_player_message(
        LeafHandle player,
        std::string_view message) const {
        if (!raw_ || !raw_->send_player_message) {
            return status::invalid_argument;
        }
        return from_abi(raw_->send_player_message(player, message.data()));
    }

    [[nodiscard]] status broadcast_message(std::string_view message) const {
        if (!raw_ || !raw_->broadcast_message) {
            return status::not_supported;
        }
        return from_abi(raw_->broadcast_message(message.data()));
    }

    [[nodiscard]] std::uint32_t player_count() const noexcept {
        if (!raw_ || !raw_->player_count) {
            return 0;
        }
        return raw_->player_count();
    }

    [[nodiscard]] status get_player_at(
        std::uint32_t index,
        LeafHandle& out_player) const {
        if (!raw_ || !raw_->get_player_at) {
            return status::not_supported;
        }
        return from_abi(raw_->get_player_at(index, &out_player));
    }

    [[nodiscard]] status get_player_name(
        LeafHandle player,
        char* out_buf,
        std::uint32_t out_buf_size) const {
        if (!raw_ || !raw_->get_player_name) {
            return status::not_supported;
        }
        return from_abi(raw_->get_player_name(player, out_buf, out_buf_size));
    }

    [[nodiscard]] status get_inventory_slot(
        LeafHandle player,
        std::uint32_t slot,
        std::uint32_t& out_item_id,
        std::uint32_t& out_count) const {
        if (!raw_ || !raw_->get_inventory_slot) {
            return status::not_supported;
        }
        return from_abi(
            raw_->get_inventory_slot(player, slot, &out_item_id, &out_count));
    }

    [[nodiscard]] status set_inventory_slot(
        LeafHandle player,
        std::uint32_t slot,
        std::uint32_t item_id,
        std::uint32_t count) const {
        if (!raw_ || !raw_->set_inventory_slot) {
            return status::not_supported;
        }
        return from_abi(raw_->set_inventory_slot(player, slot, item_id, count));
    }

    [[nodiscard]] status get_inventory_stack(
        LeafHandle player,
        std::uint32_t slot,
        LeafItemStackV1& out) const {
        if (!raw_ || !raw_->get_inventory_stack) {
            return status::not_supported;
        }
        return from_abi(raw_->get_inventory_stack(player, slot, &out));
    }

    [[nodiscard]] status set_inventory_stack(
        LeafHandle player,
        std::uint32_t slot,
        const LeafItemStackV1& stack) const {
        if (!raw_ || !raw_->set_inventory_stack) {
            return status::not_supported;
        }
        return from_abi(raw_->set_inventory_stack(player, slot, &stack));
    }

    [[nodiscard]] status get_block(
        std::int32_t x,
        std::int32_t y,
        std::int32_t z,
        std::uint32_t& out_block_id) const {
        if (!raw_ || !raw_->get_block) {
            return status::not_supported;
        }
        return from_abi(raw_->get_block(x, y, z, &out_block_id));
    }

    [[nodiscard]] status set_block(
        std::int32_t x,
        std::int32_t y,
        std::int32_t z,
        std::uint32_t block_id) const {
        if (!raw_ || !raw_->set_block) {
            return status::not_supported;
        }
        return from_abi(raw_->set_block(x, y, z, block_id));
    }

    [[nodiscard]] status get_block_dim(
        std::uint32_t dimension,
        std::int32_t x,
        std::int32_t y,
        std::int32_t z,
        std::uint32_t& out_block_id) const {
        if (!raw_ || !raw_->get_block_dim) {
            return status::not_supported;
        }
        return from_abi(
            raw_->get_block_dim(dimension, x, y, z, &out_block_id));
    }

    [[nodiscard]] status set_block_dim(
        std::uint32_t dimension,
        std::int32_t x,
        std::int32_t y,
        std::int32_t z,
        std::uint32_t block_id) const {
        if (!raw_ || !raw_->set_block_dim) {
            return status::not_supported;
        }
        return from_abi(raw_->set_block_dim(dimension, x, y, z, block_id));
    }

    [[nodiscard]] status get_player_pos(
        LeafHandle player,
        std::int32_t& out_x,
        std::int32_t& out_y,
        std::int32_t& out_z,
        std::uint32_t& out_dimension) const {
        if (!raw_ || !raw_->get_player_pos) {
            return status::not_supported;
        }
        return from_abi(
            raw_->get_player_pos(player, &out_x, &out_y, &out_z, &out_dimension));
    }

    [[nodiscard]] status set_player_pos(
        LeafHandle player,
        std::int32_t x,
        std::int32_t y,
        std::int32_t z,
        std::uint32_t dimension) const {
        if (!raw_ || !raw_->set_player_pos) {
            return status::not_supported;
        }
        return from_abi(raw_->set_player_pos(player, x, y, z, dimension));
    }

    [[nodiscard]] status get_player_health(
        LeafHandle player,
        float& out_health,
        float& out_max_health) const {
        if (!raw_ || !raw_->get_player_health) {
            return status::not_supported;
        }
        return from_abi(
            raw_->get_player_health(player, &out_health, &out_max_health));
    }

    [[nodiscard]] status set_player_health(LeafHandle player, float health) const {
        if (!raw_ || !raw_->set_player_health) {
            return status::not_supported;
        }
        return from_abi(raw_->set_player_health(player, health));
    }

    [[nodiscard]] status get_player_food(
        LeafHandle player,
        std::int32_t& out_food,
        float& out_saturation) const {
        if (!raw_ || !raw_->get_player_food) {
            return status::not_supported;
        }
        return from_abi(raw_->get_player_food(player, &out_food, &out_saturation));
    }

    [[nodiscard]] status set_player_food(
        LeafHandle player,
        std::int32_t food,
        float saturation) const {
        if (!raw_ || !raw_->set_player_food) {
            return status::not_supported;
        }
        return from_abi(raw_->set_player_food(player, food, saturation));
    }

    [[nodiscard]] status get_player_gamemode(
        LeafHandle player,
        std::uint32_t& out_mode) const {
        if (!raw_ || !raw_->get_player_gamemode) {
            return status::not_supported;
        }
        return from_abi(raw_->get_player_gamemode(player, &out_mode));
    }

    [[nodiscard]] status set_player_gamemode(
        LeafHandle player,
        std::uint32_t mode) const {
        if (!raw_ || !raw_->set_player_gamemode) {
            return status::not_supported;
        }
        return from_abi(raw_->set_player_gamemode(player, mode));
    }

    [[nodiscard]] status get_player_xp(
        LeafHandle player,
        std::int32_t& out_level,
        float& out_progress) const {
        if (!raw_ || !raw_->get_player_xp) {
            return status::not_supported;
        }
        return from_abi(raw_->get_player_xp(player, &out_level, &out_progress));
    }

    [[nodiscard]] status set_player_xp_level(
        LeafHandle player,
        std::int32_t level) const {
        if (!raw_ || !raw_->set_player_xp_level) {
            return status::not_supported;
        }
        return from_abi(raw_->set_player_xp_level(player, level));
    }

    [[nodiscard]] status get_player_look(
        LeafHandle player,
        float& out_yaw,
        float& out_pitch) const {
        if (!raw_ || !raw_->get_player_look) {
            return status::not_supported;
        }
        return from_abi(raw_->get_player_look(player, &out_yaw, &out_pitch));
    }

    [[nodiscard]] status set_player_look(
        LeafHandle player,
        float yaw,
        float pitch) const {
        if (!raw_ || !raw_->set_player_look) {
            return status::not_supported;
        }
        return from_abi(raw_->set_player_look(player, yaw, pitch));
    }

    [[nodiscard]] status play_sound(
        LeafHandle player,
        std::string_view sound_id,
        float volume,
        float pitch,
        std::int32_t x,
        std::int32_t y,
        std::int32_t z,
        std::uint32_t dimension) const {
        if (!raw_ || !raw_->play_sound || !sound_id.data()) {
            return status::not_supported;
        }
        return from_abi(raw_->play_sound(
            player, sound_id.data(), volume, pitch, x, y, z, dimension));
    }

    [[nodiscard]] status send_actionbar(
        LeafHandle player,
        std::string_view message) const {
        if (!raw_ || !raw_->send_actionbar || !message.data()) {
            return status::not_supported;
        }
        return from_abi(raw_->send_actionbar(player, message.data()));
    }

    [[nodiscard]] status send_title(
        LeafHandle player,
        std::string_view title,
        std::string_view subtitle,
        std::int32_t fade_in_ticks,
        std::int32_t stay_ticks,
        std::int32_t fade_out_ticks) const {
        if (!raw_ || !raw_->send_title) {
            return status::not_supported;
        }
        return from_abi(raw_->send_title(
            player,
            title.data() ? title.data() : "",
            subtitle.data() ? subtitle.data() : "",
            fade_in_ticks,
            stay_ticks,
            fade_out_ticks));
    }

    [[nodiscard]] status kick_player(
        LeafHandle player,
        std::string_view reason) const {
        if (!raw_ || !raw_->kick_player) {
            return status::not_supported;
        }
        return from_abi(raw_->kick_player(
            player, reason.data() ? reason.data() : ""));
    }

    [[nodiscard]] status give_item(
        LeafHandle player,
        std::uint32_t item_id,
        std::uint32_t count,
        std::int32_t damage) const {
        if (!raw_ || !raw_->give_item) {
            return status::not_supported;
        }
        return from_abi(raw_->give_item(player, item_id, count, damage));
    }

    [[nodiscard]] status apply_effect(
        LeafHandle player,
        std::string_view effect_id,
        std::int32_t duration_ticks,
        std::int32_t amplifier,
        std::uint32_t flags) const {
        if (!raw_ || !raw_->apply_effect || !effect_id.data()) {
            return status::not_supported;
        }
        return from_abi(raw_->apply_effect(
            player, effect_id.data(), duration_ticks, amplifier, flags));
    }

    [[nodiscard]] status clear_effects(LeafHandle player) const {
        if (!raw_ || !raw_->clear_effects) {
            return status::not_supported;
        }
        return from_abi(raw_->clear_effects(player));
    }

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
        double speed) const {
        if (!raw_ || !raw_->spawn_particle || !particle_id.data()) {
            return status::not_supported;
        }
        return from_abi(raw_->spawn_particle(
            particle_id.data(),
            x,
            y,
            z,
            dimension,
            count,
            dx,
            dy,
            dz,
            speed));
    }

    [[nodiscard]] status get_world_time(
        std::uint32_t dimension,
        std::int64_t& out_time) const {
        if (!raw_ || !raw_->get_world_time) {
            return status::not_supported;
        }
        return from_abi(raw_->get_world_time(dimension, &out_time));
    }

    [[nodiscard]] status set_world_time(
        std::uint32_t dimension,
        std::int64_t time) const {
        if (!raw_ || !raw_->set_world_time) {
            return status::not_supported;
        }
        return from_abi(raw_->set_world_time(dimension, time));
    }

    [[nodiscard]] status get_player_velocity(
        LeafHandle player,
        double& out_vx,
        double& out_vy,
        double& out_vz) const {
        if (!raw_ || !raw_->get_player_velocity) {
            return status::not_supported;
        }
        return from_abi(raw_->get_player_velocity(player, &out_vx, &out_vy, &out_vz));
    }

    [[nodiscard]] status set_player_velocity(
        LeafHandle player,
        double vx,
        double vy,
        double vz) const {
        if (!raw_ || !raw_->set_player_velocity) {
            return status::not_supported;
        }
        return from_abi(raw_->set_player_velocity(player, vx, vy, vz));
    }

    [[nodiscard]] status get_player_flags(
        LeafHandle player,
        std::uint32_t& out_flags) const {
        if (!raw_ || !raw_->get_player_flags) {
            return status::not_supported;
        }
        return from_abi(raw_->get_player_flags(player, &out_flags));
    }

    [[nodiscard]] status run_command(
        LeafHandle player,
        std::string_view command) const {
        if (!raw_ || !raw_->run_command || !command.data()) {
            return status::not_supported;
        }
        return from_abi(raw_->run_command(player, command.data()));
    }

    [[nodiscard]] status clear_inventory(LeafHandle player) const {
        if (!raw_ || !raw_->clear_inventory) {
            return status::not_supported;
        }
        return from_abi(raw_->clear_inventory(player));
    }

    [[nodiscard]] status broadcast_actionbar(std::string_view message) const {
        if (!raw_ || !raw_->broadcast_actionbar || !message.data()) {
            return status::not_supported;
        }
        return from_abi(raw_->broadcast_actionbar(message.data()));
    }

    [[nodiscard]] status broadcast_title(
        std::string_view title,
        std::string_view subtitle,
        std::int32_t fade_in_ticks,
        std::int32_t stay_ticks,
        std::int32_t fade_out_ticks) const {
        if (!raw_ || !raw_->broadcast_title || !title.data()) {
            return status::not_supported;
        }
        return from_abi(raw_->broadcast_title(
            title.data(),
            subtitle.data() ? subtitle.data() : "",
            fade_in_ticks,
            stay_ticks,
            fade_out_ticks));
    }

    [[nodiscard]] status set_player_flight(
        LeafHandle player,
        std::int32_t allow_flight,
        std::int32_t flying) const {
        if (!raw_ || !raw_->set_player_flight) {
            return status::not_supported;
        }
        return from_abi(raw_->set_player_flight(player, allow_flight, flying));
    }

    [[nodiscard]] status get_biome(
        std::uint32_t dimension,
        std::int32_t x,
        std::int32_t y,
        std::int32_t z,
        char* out_buf,
        std::uint32_t out_buf_size) const {
        if (!raw_ || !raw_->get_biome || !out_buf || out_buf_size == 0) {
            return status::not_supported;
        }
        return from_abi(
            raw_->get_biome(dimension, x, y, z, out_buf, out_buf_size));
    }

    [[nodiscard]] status get_difficulty(std::uint32_t& out_difficulty) const {
        if (!raw_ || !raw_->get_difficulty) {
            return status::not_supported;
        }
        return from_abi(raw_->get_difficulty(&out_difficulty));
    }

    [[nodiscard]] status set_difficulty(std::uint32_t difficulty) const {
        if (!raw_ || !raw_->set_difficulty) {
            return status::not_supported;
        }
        return from_abi(raw_->set_difficulty(difficulty));
    }

    [[nodiscard]] status get_weather(
        std::uint32_t dimension,
        std::uint32_t& out_weather) const {
        if (!raw_ || !raw_->get_weather) {
            return status::not_supported;
        }
        return from_abi(raw_->get_weather(dimension, &out_weather));
    }

    [[nodiscard]] status set_weather(
        std::uint32_t dimension,
        std::uint32_t weather,
        std::int32_t duration_ticks) const {
        if (!raw_ || !raw_->set_weather) {
            return status::not_supported;
        }
        return from_abi(raw_->set_weather(dimension, weather, duration_ticks));
    }

    [[nodiscard]] status get_light_level(
        std::uint32_t dimension,
        std::int32_t x,
        std::int32_t y,
        std::int32_t z,
        std::uint32_t& out_block_light,
        std::uint32_t& out_sky_light) const {
        if (!raw_ || !raw_->get_light_level) {
            return status::not_supported;
        }
        return from_abi(raw_->get_light_level(
            dimension, x, y, z, &out_block_light, &out_sky_light));
    }

    [[nodiscard]] status get_player_latency(
        LeafHandle player,
        std::int32_t& out_ms) const {
        if (!raw_ || !raw_->get_player_latency) {
            return status::not_supported;
        }
        return from_abi(raw_->get_player_latency(player, &out_ms));
    }

    [[nodiscard]] status get_world_spawn(
        std::uint32_t dimension,
        std::int32_t& out_x,
        std::int32_t& out_y,
        std::int32_t& out_z) const {
        if (!raw_ || !raw_->get_world_spawn) {
            return status::not_supported;
        }
        return from_abi(raw_->get_world_spawn(dimension, &out_x, &out_y, &out_z));
    }

    [[nodiscard]] status set_world_spawn(
        std::uint32_t dimension,
        std::int32_t x,
        std::int32_t y,
        std::int32_t z) const {
        if (!raw_ || !raw_->set_world_spawn) {
            return status::not_supported;
        }
        return from_abi(raw_->set_world_spawn(dimension, x, y, z));
    }

    [[nodiscard]] status is_player_op(LeafHandle player, std::int32_t& out_op) const {
        if (!raw_ || !raw_->is_player_op) {
            return status::not_supported;
        }
        return from_abi(raw_->is_player_op(player, &out_op));
    }

    [[nodiscard]] status get_player_uuid(
        LeafHandle player,
        char* out_buf,
        std::uint32_t out_buf_size) const {
        if (!raw_ || !raw_->get_player_uuid || !out_buf || out_buf_size == 0) {
            return status::not_supported;
        }
        return from_abi(raw_->get_player_uuid(player, out_buf, out_buf_size));
    }

    [[nodiscard]] status get_player_permission_level(
        LeafHandle player,
        std::int32_t& out_level) const {
        if (!raw_ || !raw_->get_player_permission_level) {
            return status::not_supported;
        }
        return from_abi(raw_->get_player_permission_level(player, &out_level));
    }

    [[nodiscard]] status find_player_by_uuid(
        const char* uuid,
        LeafHandle& out_player) const {
        if (!raw_ || !raw_->find_player_by_uuid || !uuid) {
            return status::not_supported;
        }
        return from_abi(raw_->find_player_by_uuid(uuid, &out_player));
    }

    [[nodiscard]] status find_player_by_name(
        const char* name,
        LeafHandle& out_player) const {
        if (!raw_ || !raw_->find_player_by_name || !name) {
            return status::not_supported;
        }
        return from_abi(raw_->find_player_by_name(name, &out_player));
    }

    [[nodiscard]] status get_block_registry_id(
        std::uint32_t dimension,
        std::int32_t x,
        std::int32_t y,
        std::int32_t z,
        char* out_buf,
        std::uint32_t out_buf_size) const {
        if (!raw_ || !raw_->get_block_registry_id || !out_buf || out_buf_size == 0) {
            return status::not_supported;
        }
        return from_abi(raw_->get_block_registry_id(
            dimension, x, y, z, out_buf, out_buf_size));
    }

    [[nodiscard]] status set_block_registry_id(
        std::uint32_t dimension,
        std::int32_t x,
        std::int32_t y,
        std::int32_t z,
        const char* block_id) const {
        if (!raw_ || !raw_->set_block_registry_id || !block_id) {
            return status::not_supported;
        }
        return from_abi(raw_->set_block_registry_id(dimension, x, y, z, block_id));
    }

    [[nodiscard]] status give_item_registry_id(
        LeafHandle player,
        const char* item_id,
        std::uint32_t count,
        std::int32_t damage = -1) const {
        if (!raw_ || !raw_->give_item_registry_id || !item_id) {
            return status::not_supported;
        }
        return from_abi(raw_->give_item_registry_id(player, item_id, count, damage));
    }

    [[nodiscard]] status get_inventory_item_registry_id(
        LeafHandle player,
        std::uint32_t slot,
        char* out_buf,
        std::uint32_t out_buf_size,
        std::uint32_t& out_count,
        std::int32_t& out_damage) const {
        if (!raw_ || !raw_->get_inventory_item_registry_id || !out_buf
            || out_buf_size == 0) {
            return status::not_supported;
        }
        return from_abi(raw_->get_inventory_item_registry_id(
            player, slot, out_buf, out_buf_size, &out_count, &out_damage));
    }

    [[nodiscard]] status set_inventory_item_registry_id(
        LeafHandle player,
        std::uint32_t slot,
        const char* item_id,
        std::uint32_t count,
        std::int32_t damage = -1) const {
        if (!raw_ || !raw_->set_inventory_item_registry_id || !item_id) {
            return status::not_supported;
        }
        return from_abi(raw_->set_inventory_item_registry_id(
            player, slot, item_id, count, damage));
    }

    [[nodiscard]] status teleport_player(
        LeafHandle player,
        std::int32_t x,
        std::int32_t y,
        std::int32_t z,
        std::uint32_t dimension,
        float yaw,
        float pitch) const {
        if (!raw_ || !raw_->teleport_player) {
            return status::not_supported;
        }
        return from_abi(raw_->teleport_player(
            player, x, y, z, dimension, yaw, pitch));
    }

    [[nodiscard]] status get_selected_slot(
        LeafHandle player,
        std::uint32_t& out_slot) const {
        if (!raw_ || !raw_->get_selected_slot) {
            return status::not_supported;
        }
        return from_abi(raw_->get_selected_slot(player, &out_slot));
    }

    [[nodiscard]] status set_selected_slot(
        LeafHandle player,
        std::uint32_t slot) const {
        if (!raw_ || !raw_->set_selected_slot) {
            return status::not_supported;
        }
        return from_abi(raw_->set_selected_slot(player, slot));
    }

    [[nodiscard]] status get_held_item_registry_id(
        LeafHandle player,
        char* out_buf,
        std::uint32_t out_buf_size,
        std::uint32_t& out_count,
        std::int32_t& out_damage) const {
        if (!raw_ || !raw_->get_held_item_registry_id || !out_buf
            || out_buf_size == 0) {
            return status::not_supported;
        }
        return from_abi(raw_->get_held_item_registry_id(
            player, out_buf, out_buf_size, &out_count, &out_damage));
    }

    [[nodiscard]] status set_held_item_registry_id(
        LeafHandle player,
        const char* item_id,
        std::uint32_t count,
        std::int32_t damage) const {
        if (!raw_ || !raw_->set_held_item_registry_id || !item_id) {
            return status::not_supported;
        }
        return from_abi(raw_->set_held_item_registry_id(
            player, item_id, count, damage));
    }

    [[nodiscard]] status get_equipment_item_registry_id(
        LeafHandle player,
        std::uint32_t equip_slot,
        char* out_buf,
        std::uint32_t out_buf_size,
        std::uint32_t& out_count,
        std::int32_t& out_damage) const {
        if (!raw_ || !raw_->get_equipment_item_registry_id || !out_buf
            || out_buf_size == 0) {
            return status::not_supported;
        }
        return from_abi(raw_->get_equipment_item_registry_id(
            player, equip_slot, out_buf, out_buf_size, &out_count, &out_damage));
    }

    [[nodiscard]] status set_equipment_item_registry_id(
        LeafHandle player,
        std::uint32_t equip_slot,
        const char* item_id,
        std::uint32_t count,
        std::int32_t damage = -1) const {
        if (!raw_ || !raw_->set_equipment_item_registry_id || !item_id) {
            return status::not_supported;
        }
        return from_abi(raw_->set_equipment_item_registry_id(
            player, equip_slot, item_id, count, damage));
    }

    [[nodiscard]] status get_world_seed(
        std::uint32_t dimension,
        std::int64_t& out_seed) const {
        if (!raw_ || !raw_->get_world_seed) {
            return status::not_supported;
        }
        return from_abi(raw_->get_world_seed(dimension, &out_seed));
    }

    [[nodiscard]] status heal_player(LeafHandle player) const {
        if (!raw_ || !raw_->heal_player) {
            return status::not_supported;
        }
        return from_abi(raw_->heal_player(player));
    }

    [[nodiscard]] status get_player_absorption(
        LeafHandle player,
        float& out_absorption) const {
        if (!raw_ || !raw_->get_player_absorption) {
            return status::not_supported;
        }
        return from_abi(raw_->get_player_absorption(player, &out_absorption));
    }

    [[nodiscard]] status set_player_absorption(
        LeafHandle player,
        float absorption) const {
        if (!raw_ || !raw_->set_player_absorption) {
            return status::not_supported;
        }
        return from_abi(raw_->set_player_absorption(player, absorption));
    }

    [[nodiscard]] status get_player_invulnerable(
        LeafHandle player,
        std::int32_t& out_invulnerable) const {
        if (!raw_ || !raw_->get_player_invulnerable) {
            return status::not_supported;
        }
        return from_abi(raw_->get_player_invulnerable(player, &out_invulnerable));
    }

    [[nodiscard]] status set_player_invulnerable(
        LeafHandle player,
        std::int32_t invulnerable) const {
        if (!raw_ || !raw_->set_player_invulnerable) {
            return status::not_supported;
        }
        return from_abi(raw_->set_player_invulnerable(player, invulnerable));
    }

    [[nodiscard]] status get_player_air(
        LeafHandle player,
        std::int32_t& out_air) const {
        if (!raw_ || !raw_->get_player_air) {
            return status::not_supported;
        }
        return from_abi(raw_->get_player_air(player, &out_air));
    }

    [[nodiscard]] status set_player_air(
        LeafHandle player,
        std::int32_t air) const {
        if (!raw_ || !raw_->set_player_air) {
            return status::not_supported;
        }
        return from_abi(raw_->set_player_air(player, air));
    }

    [[nodiscard]] status get_player_fire_ticks(
        LeafHandle player,
        std::int32_t& out_ticks) const {
        if (!raw_ || !raw_->get_player_fire_ticks) {
            return status::not_supported;
        }
        return from_abi(raw_->get_player_fire_ticks(player, &out_ticks));
    }

    [[nodiscard]] status set_player_fire_ticks(
        LeafHandle player,
        std::int32_t ticks) const {
        if (!raw_ || !raw_->set_player_fire_ticks) {
            return status::not_supported;
        }
        return from_abi(raw_->set_player_fire_ticks(player, ticks));
    }

    [[nodiscard]] status get_player_frozen_ticks(
        LeafHandle player,
        std::int32_t& out_ticks) const {
        if (!raw_ || !raw_->get_player_frozen_ticks) {
            return status::not_supported;
        }
        return from_abi(raw_->get_player_frozen_ticks(player, &out_ticks));
    }

    [[nodiscard]] status set_player_frozen_ticks(
        LeafHandle player,
        std::int32_t ticks) const {
        if (!raw_ || !raw_->set_player_frozen_ticks) {
            return status::not_supported;
        }
        return from_abi(raw_->set_player_frozen_ticks(player, ticks));
    }

    [[nodiscard]] status extinguish_player(LeafHandle player) const {
        if (!raw_ || !raw_->extinguish_player) {
            return status::not_supported;
        }
        return from_abi(raw_->extinguish_player(player));
    }

    [[nodiscard]] status unfreeze_player(LeafHandle player) const {
        if (!raw_ || !raw_->unfreeze_player) {
            return status::not_supported;
        }
        return from_abi(raw_->unfreeze_player(player));
    }

    [[nodiscard]] status get_player_no_gravity(
        LeafHandle player,
        std::int32_t& out_no_gravity) const {
        if (!raw_ || !raw_->get_player_no_gravity) {
            return status::not_supported;
        }
        return from_abi(raw_->get_player_no_gravity(player, &out_no_gravity));
    }

    [[nodiscard]] status set_player_no_gravity(
        LeafHandle player,
        std::int32_t no_gravity) const {
        if (!raw_ || !raw_->set_player_no_gravity) {
            return status::not_supported;
        }
        return from_abi(raw_->set_player_no_gravity(player, no_gravity));
    }

    [[nodiscard]] status get_player_silent(
        LeafHandle player,
        std::int32_t& out_silent) const {
        if (!raw_ || !raw_->get_player_silent) {
            return status::not_supported;
        }
        return from_abi(raw_->get_player_silent(player, &out_silent));
    }

    [[nodiscard]] status set_player_silent(
        LeafHandle player,
        std::int32_t silent) const {
        if (!raw_ || !raw_->set_player_silent) {
            return status::not_supported;
        }
        return from_abi(raw_->set_player_silent(player, silent));
    }

    [[nodiscard]] status get_player_glowing(
        LeafHandle player,
        std::int32_t& out_glowing) const {
        if (!raw_ || !raw_->get_player_glowing) {
            return status::not_supported;
        }
        return from_abi(raw_->get_player_glowing(player, &out_glowing));
    }

    [[nodiscard]] status set_player_glowing(
        LeafHandle player,
        std::int32_t glowing) const {
        if (!raw_ || !raw_->set_player_glowing) {
            return status::not_supported;
        }
        return from_abi(raw_->set_player_glowing(player, glowing));
    }

    [[nodiscard]] status get_player_invisible(
        LeafHandle player,
        std::int32_t& out_invisible) const {
        if (!raw_ || !raw_->get_player_invisible) {
            return status::not_supported;
        }
        return from_abi(raw_->get_player_invisible(player, &out_invisible));
    }

    [[nodiscard]] status set_player_invisible(
        LeafHandle player,
        std::int32_t invisible) const {
        if (!raw_ || !raw_->set_player_invisible) {
            return status::not_supported;
        }
        return from_abi(raw_->set_player_invisible(player, invisible));
    }

    [[nodiscard]] status get_player_portal_cooldown(
        LeafHandle player,
        std::int32_t& out_ticks) const {
        if (!raw_ || !raw_->get_player_portal_cooldown) {
            return status::not_supported;
        }
        return from_abi(raw_->get_player_portal_cooldown(player, &out_ticks));
    }

    [[nodiscard]] status set_player_portal_cooldown(
        LeafHandle player,
        std::int32_t ticks) const {
        if (!raw_ || !raw_->set_player_portal_cooldown) {
            return status::not_supported;
        }
        return from_abi(raw_->set_player_portal_cooldown(player, ticks));
    }

    [[nodiscard]] status get_player_max_air(
        LeafHandle player,
        std::int32_t& out_max_air) const {
        if (!raw_ || !raw_->get_player_max_air) {
            return status::not_supported;
        }
        return from_abi(raw_->get_player_max_air(player, &out_max_air));
    }

    [[nodiscard]] status refill_player_air(LeafHandle player) const {
        if (!raw_ || !raw_->refill_player_air) {
            return status::not_supported;
        }
        return from_abi(raw_->refill_player_air(player));
    }

    [[nodiscard]] status is_player_alive(
        LeafHandle player,
        std::int32_t& out_alive) const {
        if (!raw_ || !raw_->is_player_alive) {
            return status::not_supported;
        }
        return from_abi(raw_->is_player_alive(player, &out_alive));
    }

    [[nodiscard]] status subscribe(
        std::uint32_t event_id,
        LeafEventCallback callback,
        void* user_data,
        subscription& out) const {
        if (!raw_ || !raw_->subscribe_event || !callback) {
            return status::invalid_argument;
        }
        std::uint64_t id = 0;
        const auto st = from_abi(
            raw_->subscribe_event(event_id, callback, user_data, &id));
        if (ok(st)) {
            out = subscription{raw_, id};
        }
        return st;
    }

    /// Member-function trampoline: `Method` is `&T::on_event(event_view)`.
    template <auto Method, typename T>
    [[nodiscard]] status subscribe_method(
        std::uint32_t event_id,
        T* self,
        subscription& out) const {
        return subscribe(event_id, &member_trampoline<Method, T>, self, out);
    }

    /// Decision listener: `Method` is `&T::on_event(event_view) -> int`
    /// returning `LEAF_DECISION_*`.
    template <auto Method, typename T>
    [[nodiscard]] status subscribe_decision_method(
        std::uint32_t event_id,
        T* self,
        subscription& out) const {
        if (!raw_ || !raw_->subscribe_decision || !self) {
            return status::invalid_argument;
        }
        std::uint64_t id = 0;
        const auto st = from_abi(raw_->subscribe_decision(
            event_id,
            &decision_trampoline<Method, T>,
            self,
            &id));
        if (ok(st)) {
            out = subscription{raw_, id};
        }
        return st;
    }

    [[nodiscard]] status post_main(LeafTaskCallback callback, void* user_data) const {
        if (!raw_ || !raw_->post_main || !callback) {
            return status::invalid_argument;
        }
        return from_abi(raw_->post_main(callback, user_data));
    }

    [[nodiscard]] status delay_main(
        LeafTaskCallback callback,
        void* user_data,
        std::uint32_t delay_ms,
        std::uint64_t& out_task_id) const {
        if (!raw_ || !raw_->delay_main || !callback) {
            return status::invalid_argument;
        }
        return from_abi(raw_->delay_main(callback, user_data, delay_ms, &out_task_id));
    }

    [[nodiscard]] status cancel_task(std::uint64_t task_id) const {
        if (!raw_ || !raw_->cancel_task) {
            return status::not_supported;
        }
        return from_abi(raw_->cancel_task(task_id));
    }

private:
    template <auto Method, typename T>
    static void member_trampoline(const void* event, void* user_data) {
        auto* self = static_cast<T*>(user_data);
        (self->*Method)(event_view{event});
    }

    template <auto Method, typename T>
    static int decision_trampoline(const void* event, void* user_data) {
        auto* self = static_cast<T*>(user_data);
        return (self->*Method)(event_view{event});
    }

    const LeafApiV1* raw_{nullptr};
};

} // namespace leaf::sdk
