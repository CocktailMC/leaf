#include "leaf/bridge/bridge_host.hpp"

#include "leaf/abi/leaf_abi_v1.h"
#include "leaf/events/payloads.hpp"
#include "leaf/loader/host_target.hpp"
#include "leaf/minecraft/minecraft_abi.hpp"

#include <cstring>
#include <utility>

namespace leaf {
bridge_host& bridge_host::instance() {
    static bridge_host host;
    return host;
}

capability_set bridge_host::capabilities_for_(
    const version& mc,
    std::uint32_t /*loader*/) const {
    capability_set caps;
    if (mc.major > 1 || (mc.major == 1 && mc.minor > 20)
        || (mc.major == 1 && mc.minor == 20 && mc.patch >= 4)) {
        caps.enable(capability::data_components);
        caps.enable(capability::registry_modern);
    } else {
        caps.enable(capability::legacy_item_nbt);
        caps.enable(capability::registry_legacy);
    }
    if (mc.major > 1 || (mc.major == 1 && mc.minor >= 20)) {
        caps.enable(capability::custom_payload_v2);
    }
    caps.enable(capability::inventory_stack_v1);
    return caps;
}

status bridge_host::init(const LeafBridgeConfigV1& config) {
    std::scoped_lock lock(mutex_);
    if (ready_) {
        return err(ec::already_exists, "bridge host already initialized");
    }
    if (config.struct_size < sizeof(LeafBridgeConfigV1)) {
        return err(LEAF_ABI_VERSION_MISMATCH, "LeafBridgeConfigV1 too small");
    }
    if (!config.minecraft_version || !*config.minecraft_version) {
        return err(ec::invalid_argument, "minecraft_version required");
    }
    if (!config.leafmods_dir || !*config.leafmods_dir) {
        return err(ec::invalid_argument, "leafmods_dir required");
    }

    auto parsed = parse_version(config.minecraft_version);
    if (!parsed) {
        return err(parsed.error());
    }

    minecraft_version_text_ = config.minecraft_version;
    minecraft_version_ = *parsed;
    loader_ = config.loader;
    mapping_ = config.mapping;
    leafmods_dir_ = config.leafmods_dir;
    if (config.game_dir && *config.game_dir) {
        game_dir_ = config.game_dir;
    }

    // First-run UX: create leafmods/ if the path is missing.
    {
        std::error_code ec;
        if (!std::filesystem::exists(leafmods_dir_, ec)) {
            if (!std::filesystem::create_directories(leafmods_dir_, ec) && ec) {
                return err(ec::io_error,
                    "failed to create leafmods_dir: " + leafmods_dir_.string());
            }
        }
    }

    auto caps = capabilities_for_(minecraft_version_, loader_);
    engine_.emplace(caps);
    engine_->api().set_capabilities(caps);

    auto abi = create_minecraft_abi(
        minecraft_version_,
        static_cast<loader_kind>(loader_));
    if (!abi) {
        engine_.reset();
        return err(abi.error());
    }
    minecraft_ = std::move(*abi);
    engine_->api().attach_minecraft_abi(minecraft_.get());
    if (auto* stub = stub_minecraft()) {
        stub->set_send_hook(send_hook_);
    }

    if (config.auto_load_mods) {
        engine_bootstrap_options opt;
        opt.leafmods_dir = leafmods_dir_;
        opt.resolve.minecraft = minecraft_version_;
        opt.resolve.host_target = current_host_target();
        opt.capabilities = caps;

        if (auto st = engine_->bootstrap(opt); !st) {
            engine_.reset();
            return st;
        }
        if (auto st = engine_->enable_all(); !st) {
            (void)engine_->unload_all();
            engine_.reset();
            return st;
        }
    }

    ready_ = true;
    return ok();
}

status bridge_host::shutdown() {
    std::scoped_lock lock(mutex_);
    if (!ready_) {
        return ok();
    }
    if (engine_) {
        engine_->api().attach_minecraft_abi(nullptr);
        (void)engine_->disable_all();
        (void)engine_->unload_all();
        engine_.reset();
    }
    minecraft_.reset();
    ready_ = false;
    minecraft_version_text_.clear();
    return ok();
}

bool bridge_host::initialized() const noexcept {
    std::scoped_lock lock(mutex_);
    return ready_;
}

status bridge_host::pump_main(std::uint32_t budget) {
    std::scoped_lock lock(mutex_);
    if (!ready_ || !engine_) {
        return err(ec::mod_state_invalid, "bridge host not initialized");
    }
    auto& sched = engine_->schedule();
    if (!sched.main_thread_bound()) {
        sched.bind_main_thread();
    } else if (!sched.is_main_thread()) {
        return err(LEAF_SCHED_WRONG_THREAD, "pump_main must run on main thread");
    }
    auto st = sched.pump_main(budget == 0 ? 256u : budget);
    if (!st) {
        return st;
    }
    struct tick_payload {
        std::uint32_t pad{0};
    } p{};
    (void)engine_->events().publish_notification(event_ids::server_tick, p);
    return ok();
}

status bridge_host::fill_info(LeafBridgeInfoV1& out) {
    std::scoped_lock lock(mutex_);
    if (!ready_) {
        return err(ec::mod_state_invalid, "bridge host not initialized");
    }
    out.struct_size = static_cast<uint32_t>(sizeof(LeafBridgeInfoV1));
    out.abi_major = LEAF_ABI_VERSION_MAJOR;
    out.abi_minor = LEAF_ABI_VERSION_MINOR;
    out.engine_version = "0.1.0";
    out.loader = loader_;
    out.mapping = mapping_;
    out.minecraft_version = minecraft_version_text_.c_str();
    return ok();
}

std::uint32_t bridge_host::loaded_mod_count() const noexcept {
    std::scoped_lock lock(mutex_);
    if (!ready_ || !engine_) {
        return 0;
    }
    return static_cast<std::uint32_t>(engine_->mods().size());
}

status bridge_host::mod_id_at(
    std::uint32_t index,
    char* out_buf,
    std::uint32_t out_buf_size) const {
    std::scoped_lock lock(mutex_);
    if (!ready_ || !engine_) {
        return err(ec::mod_state_invalid, "bridge host not initialized");
    }
    if (!out_buf || out_buf_size == 0) {
        return err(ec::invalid_argument, "out_buf required");
    }
    if (index >= engine_->mods().size()) {
        return err(ec::not_found, "mod index out of range");
    }
    const auto& id = engine_->mods()[index]->id();
    if (id.size() + 1 > out_buf_size) {
        return err(ec::invalid_argument, "out_buf too small");
    }
    std::memcpy(out_buf, id.data(), id.size());
    out_buf[id.size()] = '\0';
    return ok();
}

status bridge_host::on_server_starting() {
    std::scoped_lock lock(mutex_);
    if (!ready_ || !engine_) {
        return err(ec::mod_state_invalid, "bridge host not initialized");
    }
    struct empty_payload {
        std::uint32_t pad{0};
    } p{};
    return engine_->events().publish_notification(
        event_ids::core_server_starting,
        p);
}

status bridge_host::on_server_started() {
    std::scoped_lock lock(mutex_);
    if (!ready_ || !engine_) {
        return err(ec::mod_state_invalid, "bridge host not initialized");
    }
    struct empty_payload {
        std::uint32_t pad{0};
    } p{};
    return engine_->events().publish_notification(
        event_ids::core_server_started,
        p);
}

status bridge_host::on_server_stopping() {
    std::scoped_lock lock(mutex_);
    if (!ready_ || !engine_) {
        return err(ec::mod_state_invalid, "bridge host not initialized");
    }
    return engine_->disable_all();
}

status bridge_host::emit_player_join(std::uint64_t player) {
    std::scoped_lock lock(mutex_);
    if (!ready_ || !engine_) {
        return err(ec::mod_state_invalid, "bridge host not initialized");
    }
    if (auto* stub = stub_minecraft()) {
        // Keep a prior leaf_bridge_register_player name if the live bridge set one.
        if (!stub->player_name(player_handle{player})) {
            stub->register_player(
                player_handle{player},
                "player#" + std::to_string(player));
        }
    }
    player_join_payload_v1 payload{.player_handle = player};
    return engine_->events().publish_notification(event_ids::player_join, payload);
}

status bridge_host::emit_player_leave(std::uint64_t player) {
    std::scoped_lock lock(mutex_);
    if (!ready_ || !engine_) {
        return err(ec::mod_state_invalid, "bridge host not initialized");
    }
    if (auto* stub = stub_minecraft()) {
        stub->unregister_player(player_handle{player});
    }
    player_leave_payload_v1 payload{.player_handle = player};
    return engine_->events().publish_notification(event_ids::player_leave, payload);
}

result<decision> bridge_host::emit_player_join_request(std::uint64_t player) {
    std::scoped_lock lock(mutex_);
    if (!ready_ || !engine_) {
        return err<decision>(ec::mod_state_invalid, "bridge host not initialized");
    }
    player_join_request_payload_v1 payload{.player_handle = player};
    auto r = engine_->events().publish_decision(event_ids::player_join_request, payload);
    if (!r) {
        return err<decision>(r.error());
    }
    return r->final_decision;
}

result<decision> bridge_host::emit_player_chat(
    std::uint64_t player,
    std::string_view message) {
    std::scoped_lock lock(mutex_);
    if (!ready_ || !engine_) {
        return err<decision>(ec::mod_state_invalid, "bridge host not initialized");
    }
    player_chat_payload_v1 payload{};
    payload.player_handle = player;
    const auto n = message.size() < (sizeof(payload.message) - 1)
        ? message.size()
        : (sizeof(payload.message) - 1);
    if (n > 0 && message.data() != nullptr) {
        std::memcpy(payload.message, message.data(), n);
    }
    payload.message[n] = '\0';
    auto r = engine_->events().publish_decision(event_ids::player_chat, payload);
    if (!r) {
        return err<decision>(r.error());
    }
    return r->final_decision;
}

status bridge_host::emit_player_death(std::uint64_t player) {
    std::scoped_lock lock(mutex_);
    if (!ready_ || !engine_) {
        return err(ec::mod_state_invalid, "bridge host not initialized");
    }
    player_death_payload_v1 payload{.player_handle = player};
    return engine_->events().publish_notification(event_ids::player_death, payload);
}

status bridge_host::emit_entity_spawn(
    std::uint64_t entity,
    std::uint32_t entity_type_id,
    std::int32_t x,
    std::int32_t y,
    std::int32_t z,
    std::uint32_t dimension) {
    std::scoped_lock lock(mutex_);
    if (!ready_ || !engine_) {
        return err(ec::mod_state_invalid, "bridge host not initialized");
    }
    entity_lifecycle_payload_v1 payload{
        .entity_handle = entity,
        .entity_type_id = entity_type_id,
        .x = x,
        .y = y,
        .z = z,
        .dimension = dimension,
    };
    return engine_->events().publish_notification(event_ids::entity_spawn, payload);
}

status bridge_host::emit_entity_remove(
    std::uint64_t entity,
    std::uint32_t entity_type_id,
    std::int32_t x,
    std::int32_t y,
    std::int32_t z,
    std::uint32_t dimension) {
    std::scoped_lock lock(mutex_);
    if (!ready_ || !engine_) {
        return err(ec::mod_state_invalid, "bridge host not initialized");
    }
    entity_lifecycle_payload_v1 payload{
        .entity_handle = entity,
        .entity_type_id = entity_type_id,
        .x = x,
        .y = y,
        .z = z,
        .dimension = dimension,
    };
    return engine_->events().publish_notification(event_ids::entity_remove, payload);
}

status bridge_host::emit_world_load(std::uint32_t dimension) {
    std::scoped_lock lock(mutex_);
    if (!ready_ || !engine_) {
        return err(ec::mod_state_invalid, "bridge host not initialized");
    }
    world_load_payload_v1 payload{.dimension = dimension};
    return engine_->events().publish_notification(event_ids::world_load, payload);
}

result<decision> bridge_host::emit_block_break(
    std::uint64_t player,
    std::int32_t x,
    std::int32_t y,
    std::int32_t z,
    std::uint32_t block_id) {
    std::scoped_lock lock(mutex_);
    if (!ready_ || !engine_) {
        return err<decision>(ec::mod_state_invalid, "bridge host not initialized");
    }
    block_change_payload_v1 payload{
        .player_handle = player,
        .x = x,
        .y = y,
        .z = z,
        .block_id = block_id,
    };
    auto r = engine_->events().publish_decision(event_ids::block_break, payload);
    if (!r) {
        return err<decision>(r.error());
    }
    return r->final_decision;
}

result<decision> bridge_host::emit_block_place(
    std::uint64_t player,
    std::int32_t x,
    std::int32_t y,
    std::int32_t z,
    std::uint32_t block_id) {
    std::scoped_lock lock(mutex_);
    if (!ready_ || !engine_) {
        return err<decision>(ec::mod_state_invalid, "bridge host not initialized");
    }
    block_change_payload_v1 payload{
        .player_handle = player,
        .x = x,
        .y = y,
        .z = z,
        .block_id = block_id,
    };
    auto r = engine_->events().publish_decision(event_ids::block_place, payload);
    if (!r) {
        return err<decision>(r.error());
    }
    return r->final_decision;
}

status bridge_host::reload_mods() {
    std::scoped_lock lock(mutex_);
    if (!ready_ || !engine_) {
        return err(ec::mod_state_invalid, "bridge host not initialized");
    }

    // Phase-1 reload: disable/unload everything, then bootstrap again.
    (void)engine_->disable_all();
    (void)engine_->unload_all();

    auto caps = capabilities_for_(minecraft_version_, loader_);
    engine_->api().set_capabilities(caps);

    engine_bootstrap_options opt;
    opt.leafmods_dir = leafmods_dir_;
    opt.resolve.minecraft = minecraft_version_;
    opt.resolve.host_target = current_host_target();
    opt.capabilities = caps;

    if (auto st = engine_->bootstrap(opt); !st) {
        return st;
    }
    return engine_->enable_all();
}

void bridge_host::set_send_player_message_hook(
    void (*fn)(std::uint64_t player_handle, const char* message)) {
    std::scoped_lock lock(mutex_);
    send_hook_ = fn;
    if (auto* stub = stub_minecraft()) {
        stub->set_send_hook(fn);
    }
}

void bridge_host::set_broadcast_message_hook(void (*fn)(const char* message)) {
    std::scoped_lock lock(mutex_);
    broadcast_hook_ = fn;
    if (auto* stub = stub_minecraft()) {
        stub->set_broadcast_hook(fn);
    }
}

void bridge_host::set_inventory_hooks(
    stub_minecraft_abi::inv_get_fn get_fn,
    stub_minecraft_abi::inv_set_fn set_fn) {
    std::scoped_lock lock(mutex_);
    inv_get_hook_ = get_fn;
    inv_set_hook_ = set_fn;
    if (auto* stub = stub_minecraft()) {
        stub->set_inventory_hooks(get_fn, set_fn);
    }
}

void bridge_host::set_inventory_stack_hooks(
    stub_minecraft_abi::inv_stack_get_fn get_fn,
    stub_minecraft_abi::inv_stack_set_fn set_fn) {
    std::scoped_lock lock(mutex_);
    inv_stack_get_hook_ = get_fn;
    inv_stack_set_hook_ = set_fn;
    if (auto* stub = stub_minecraft()) {
        stub->set_inventory_stack_hooks(get_fn, set_fn);
    }
}

void bridge_host::set_get_block_hook(stub_minecraft_abi::block_get_fn fn) {
    std::scoped_lock lock(mutex_);
    block_get_hook_ = fn;
    if (auto* stub = stub_minecraft()) {
        stub->set_get_block_hook(fn);
    }
}

void bridge_host::set_world_hooks(
    stub_minecraft_abi::block_get_fn get_fn,
    stub_minecraft_abi::block_set_fn set_fn) {
    std::scoped_lock lock(mutex_);
    block_get_hook_ = get_fn;
    block_set_hook_ = set_fn;
    if (auto* stub = stub_minecraft()) {
        stub->set_world_hooks(get_fn, set_fn);
    }
}

void bridge_host::set_player_pos_hooks(
    stub_minecraft_abi::player_pos_get_fn get_fn,
    stub_minecraft_abi::player_pos_set_fn set_fn) {
    std::scoped_lock lock(mutex_);
    player_pos_get_hook_ = get_fn;
    player_pos_set_hook_ = set_fn;
    if (auto* stub = stub_minecraft()) {
        stub->set_player_pos_hooks(get_fn, set_fn);
    }
}

void bridge_host::set_player_health_hooks(
    stub_minecraft_abi::player_health_get_fn get_fn,
    stub_minecraft_abi::player_health_set_fn set_fn) {
    std::scoped_lock lock(mutex_);
    player_health_get_hook_ = get_fn;
    player_health_set_hook_ = set_fn;
    if (auto* stub = stub_minecraft()) {
        stub->set_player_health_hooks(get_fn, set_fn);
    }
}

void bridge_host::set_player_food_hooks(
    stub_minecraft_abi::player_food_get_fn get_fn,
    stub_minecraft_abi::player_food_set_fn set_fn) {
    std::scoped_lock lock(mutex_);
    player_food_get_hook_ = get_fn;
    player_food_set_hook_ = set_fn;
    if (auto* stub = stub_minecraft()) {
        stub->set_player_food_hooks(get_fn, set_fn);
    }
}

void bridge_host::set_player_gamemode_hooks(
    stub_minecraft_abi::player_gamemode_get_fn get_fn,
    stub_minecraft_abi::player_gamemode_set_fn set_fn) {
    std::scoped_lock lock(mutex_);
    player_gamemode_get_hook_ = get_fn;
    player_gamemode_set_hook_ = set_fn;
    if (auto* stub = stub_minecraft()) {
        stub->set_player_gamemode_hooks(get_fn, set_fn);
    }
}

void bridge_host::set_player_xp_hooks(
    stub_minecraft_abi::player_xp_get_fn get_fn,
    stub_minecraft_abi::player_xp_set_fn set_fn) {
    std::scoped_lock lock(mutex_);
    player_xp_get_hook_ = get_fn;
    player_xp_set_hook_ = set_fn;
    if (auto* stub = stub_minecraft()) {
        stub->set_player_xp_hooks(get_fn, set_fn);
    }
}

void bridge_host::set_player_look_hooks(
    stub_minecraft_abi::player_look_get_fn get_fn,
    stub_minecraft_abi::player_look_set_fn set_fn) {
    std::scoped_lock lock(mutex_);
    player_look_get_hook_ = get_fn;
    player_look_set_hook_ = set_fn;
    if (auto* stub = stub_minecraft()) {
        stub->set_player_look_hooks(get_fn, set_fn);
    }
}

void bridge_host::set_play_sound_hook(stub_minecraft_abi::play_sound_fn fn) {
    std::scoped_lock lock(mutex_);
    play_sound_hook_ = fn;
    if (auto* stub = stub_minecraft()) {
        stub->set_play_sound_hook(fn);
    }
}

void bridge_host::set_actionbar_hook(stub_minecraft_abi::actionbar_fn fn) {
    std::scoped_lock lock(mutex_);
    actionbar_hook_ = fn;
    if (auto* stub = stub_minecraft()) {
        stub->set_actionbar_hook(fn);
    }
}

void bridge_host::set_title_hook(stub_minecraft_abi::title_fn fn) {
    std::scoped_lock lock(mutex_);
    title_hook_ = fn;
    if (auto* stub = stub_minecraft()) {
        stub->set_title_hook(fn);
    }
}

void bridge_host::set_kick_hook(stub_minecraft_abi::kick_fn fn) {
    std::scoped_lock lock(mutex_);
    kick_hook_ = fn;
    if (auto* stub = stub_minecraft()) {
        stub->set_kick_hook(fn);
    }
}

void bridge_host::set_give_item_hook(stub_minecraft_abi::give_item_fn fn) {
    std::scoped_lock lock(mutex_);
    give_item_hook_ = fn;
    if (auto* stub = stub_minecraft()) {
        stub->set_give_item_hook(fn);
    }
}

void bridge_host::set_effect_hooks(
    stub_minecraft_abi::apply_effect_fn apply_fn,
    stub_minecraft_abi::clear_effects_fn clear_fn) {
    std::scoped_lock lock(mutex_);
    apply_effect_hook_ = apply_fn;
    clear_effects_hook_ = clear_fn;
    if (auto* stub = stub_minecraft()) {
        stub->set_effect_hooks(apply_fn, clear_fn);
    }
}

void bridge_host::set_spawn_particle_hook(
    stub_minecraft_abi::spawn_particle_fn fn) {
    std::scoped_lock lock(mutex_);
    spawn_particle_hook_ = fn;
    if (auto* stub = stub_minecraft()) {
        stub->set_spawn_particle_hook(fn);
    }
}

void bridge_host::set_world_time_hooks(
    stub_minecraft_abi::world_time_get_fn get_fn,
    stub_minecraft_abi::world_time_set_fn set_fn) {
    std::scoped_lock lock(mutex_);
    world_time_get_hook_ = get_fn;
    world_time_set_hook_ = set_fn;
    if (auto* stub = stub_minecraft()) {
        stub->set_world_time_hooks(get_fn, set_fn);
    }
}

void bridge_host::set_player_velocity_hooks(
    stub_minecraft_abi::player_velocity_get_fn get_fn,
    stub_minecraft_abi::player_velocity_set_fn set_fn) {
    std::scoped_lock lock(mutex_);
    player_velocity_get_hook_ = get_fn;
    player_velocity_set_hook_ = set_fn;
    if (auto* stub = stub_minecraft()) {
        stub->set_player_velocity_hooks(get_fn, set_fn);
    }
}

void bridge_host::set_player_flags_hook(stub_minecraft_abi::player_flags_fn fn) {
    std::scoped_lock lock(mutex_);
    player_flags_hook_ = fn;
    if (auto* stub = stub_minecraft()) {
        stub->set_player_flags_hook(fn);
    }
}

void bridge_host::set_run_command_hook(stub_minecraft_abi::run_command_fn fn) {
    std::scoped_lock lock(mutex_);
    run_command_hook_ = fn;
    if (auto* stub = stub_minecraft()) {
        stub->set_run_command_hook(fn);
    }
}

void bridge_host::set_clear_inventory_hook(stub_minecraft_abi::clear_inventory_fn fn) {
    std::scoped_lock lock(mutex_);
    clear_inventory_hook_ = fn;
    if (auto* stub = stub_minecraft()) {
        stub->set_clear_inventory_hook(fn);
    }
}

void bridge_host::set_player_flight_hook(stub_minecraft_abi::player_flight_fn fn) {
    std::scoped_lock lock(mutex_);
    player_flight_hook_ = fn;
    if (auto* stub = stub_minecraft()) {
        stub->set_player_flight_hook(fn);
    }
}

void bridge_host::set_get_biome_hook(stub_minecraft_abi::get_biome_fn fn) {
    std::scoped_lock lock(mutex_);
    get_biome_hook_ = fn;
    if (auto* stub = stub_minecraft()) {
        stub->set_get_biome_hook(fn);
    }
}

void bridge_host::set_difficulty_hooks(
    stub_minecraft_abi::difficulty_get_fn get_fn,
    stub_minecraft_abi::difficulty_set_fn set_fn) {
    std::scoped_lock lock(mutex_);
    difficulty_get_hook_ = get_fn;
    difficulty_set_hook_ = set_fn;
    if (auto* stub = stub_minecraft()) {
        stub->set_difficulty_hooks(get_fn, set_fn);
    }
}

void bridge_host::set_weather_hooks(
    stub_minecraft_abi::weather_get_fn get_fn,
    stub_minecraft_abi::weather_set_fn set_fn) {
    std::scoped_lock lock(mutex_);
    weather_get_hook_ = get_fn;
    weather_set_hook_ = set_fn;
    if (auto* stub = stub_minecraft()) {
        stub->set_weather_hooks(get_fn, set_fn);
    }
}

void bridge_host::set_get_light_level_hook(
    stub_minecraft_abi::get_light_level_fn fn) {
    std::scoped_lock lock(mutex_);
    get_light_level_hook_ = fn;
    if (auto* stub = stub_minecraft()) {
        stub->set_get_light_level_hook(fn);
    }
}

void bridge_host::set_player_latency_hook(
    stub_minecraft_abi::player_latency_fn fn) {
    std::scoped_lock lock(mutex_);
    player_latency_hook_ = fn;
    if (auto* stub = stub_minecraft()) {
        stub->set_player_latency_hook(fn);
    }
}

void bridge_host::set_world_spawn_hooks(
    stub_minecraft_abi::world_spawn_get_fn get_fn,
    stub_minecraft_abi::world_spawn_set_fn set_fn) {
    std::scoped_lock lock(mutex_);
    world_spawn_get_hook_ = get_fn;
    world_spawn_set_hook_ = set_fn;
    if (auto* stub = stub_minecraft()) {
        stub->set_world_spawn_hooks(get_fn, set_fn);
    }
}

void bridge_host::set_player_op_hook(stub_minecraft_abi::player_op_fn fn) {
    std::scoped_lock lock(mutex_);
    player_op_hook_ = fn;
    if (auto* stub = stub_minecraft()) {
        stub->set_player_op_hook(fn);
    }
}

void bridge_host::set_player_uuid_hook(stub_minecraft_abi::player_uuid_fn fn) {
    std::scoped_lock lock(mutex_);
    player_uuid_hook_ = fn;
    if (auto* stub = stub_minecraft()) {
        stub->set_player_uuid_hook(fn);
    }
}

void bridge_host::set_player_permission_hook(
    stub_minecraft_abi::player_permission_fn fn) {
    std::scoped_lock lock(mutex_);
    player_permission_hook_ = fn;
    if (auto* stub = stub_minecraft()) {
        stub->set_player_permission_hook(fn);
    }
}

void bridge_host::set_find_player_uuid_hook(
    stub_minecraft_abi::find_player_uuid_fn fn) {
    std::scoped_lock lock(mutex_);
    find_player_uuid_hook_ = fn;
    if (auto* stub = stub_minecraft()) {
        stub->set_find_player_uuid_hook(fn);
    }
}

void bridge_host::set_find_player_name_hook(
    stub_minecraft_abi::find_player_name_fn fn) {
    std::scoped_lock lock(mutex_);
    find_player_name_hook_ = fn;
    if (auto* stub = stub_minecraft()) {
        stub->set_find_player_name_hook(fn);
    }
}

void bridge_host::set_block_registry_hooks(
    stub_minecraft_abi::get_block_registry_fn get_fn,
    stub_minecraft_abi::set_block_registry_fn set_fn) {
    std::scoped_lock lock(mutex_);
    get_block_registry_hook_ = get_fn;
    set_block_registry_hook_ = set_fn;
    if (auto* stub = stub_minecraft()) {
        stub->set_block_registry_hooks(get_fn, set_fn);
    }
}

void bridge_host::set_give_item_registry_hook(
    stub_minecraft_abi::give_item_registry_fn fn) {
    std::scoped_lock lock(mutex_);
    give_item_registry_hook_ = fn;
    if (auto* stub = stub_minecraft()) {
        stub->set_give_item_registry_hook(fn);
    }
}

void bridge_host::set_inventory_registry_hooks(
    stub_minecraft_abi::inv_registry_get_fn get_fn,
    stub_minecraft_abi::inv_registry_set_fn set_fn) {
    std::scoped_lock lock(mutex_);
    inv_registry_get_hook_ = get_fn;
    inv_registry_set_hook_ = set_fn;
    if (auto* stub = stub_minecraft()) {
        stub->set_inventory_registry_hooks(get_fn, set_fn);
    }
}

void bridge_host::set_teleport_hook(stub_minecraft_abi::teleport_fn fn) {
    std::scoped_lock lock(mutex_);
    teleport_hook_ = fn;
    if (auto* stub = stub_minecraft()) {
        stub->set_teleport_hook(fn);
    }
}

void bridge_host::set_selected_slot_hooks(
    stub_minecraft_abi::selected_slot_get_fn get_fn,
    stub_minecraft_abi::selected_slot_set_fn set_fn) {
    std::scoped_lock lock(mutex_);
    selected_slot_get_hook_ = get_fn;
    selected_slot_set_hook_ = set_fn;
    if (auto* stub = stub_minecraft()) {
        stub->set_selected_slot_hooks(get_fn, set_fn);
    }
}

void bridge_host::set_get_world_seed_hook(stub_minecraft_abi::get_world_seed_fn fn) {
    std::scoped_lock lock(mutex_);
    get_world_seed_hook_ = fn;
    if (auto* stub = stub_minecraft()) {
        stub->set_get_world_seed_hook(fn);
    }
}

void bridge_host::set_player_absorption_hooks(
    stub_minecraft_abi::player_absorption_get_fn get_fn,
    stub_minecraft_abi::player_absorption_set_fn set_fn) {
    std::scoped_lock lock(mutex_);
    player_absorption_get_hook_ = get_fn;
    player_absorption_set_hook_ = set_fn;
    if (auto* stub = stub_minecraft()) {
        stub->set_player_absorption_hooks(get_fn, set_fn);
    }
}

void bridge_host::set_player_invulnerable_hooks(
    stub_minecraft_abi::player_invulnerable_get_fn get_fn,
    stub_minecraft_abi::player_invulnerable_set_fn set_fn) {
    std::scoped_lock lock(mutex_);
    player_invulnerable_get_hook_ = get_fn;
    player_invulnerable_set_hook_ = set_fn;
    if (auto* stub = stub_minecraft()) {
        stub->set_player_invulnerable_hooks(get_fn, set_fn);
    }
}

void bridge_host::set_player_air_hooks(
    stub_minecraft_abi::player_air_get_fn get_fn,
    stub_minecraft_abi::player_air_set_fn set_fn) {
    std::scoped_lock lock(mutex_);
    player_air_get_hook_ = get_fn;
    player_air_set_hook_ = set_fn;
    if (auto* stub = stub_minecraft()) {
        stub->set_player_air_hooks(get_fn, set_fn);
    }
}

void bridge_host::set_player_fire_ticks_hooks(
    stub_minecraft_abi::player_fire_ticks_get_fn get_fn,
    stub_minecraft_abi::player_fire_ticks_set_fn set_fn) {
    std::scoped_lock lock(mutex_);
    player_fire_ticks_get_hook_ = get_fn;
    player_fire_ticks_set_hook_ = set_fn;
    if (auto* stub = stub_minecraft()) {
        stub->set_player_fire_ticks_hooks(get_fn, set_fn);
    }
}

void bridge_host::set_player_frozen_ticks_hooks(
    stub_minecraft_abi::player_frozen_ticks_get_fn get_fn,
    stub_minecraft_abi::player_frozen_ticks_set_fn set_fn) {
    std::scoped_lock lock(mutex_);
    player_frozen_ticks_get_hook_ = get_fn;
    player_frozen_ticks_set_hook_ = set_fn;
    if (auto* stub = stub_minecraft()) {
        stub->set_player_frozen_ticks_hooks(get_fn, set_fn);
    }
}

void bridge_host::set_player_no_gravity_hooks(
    stub_minecraft_abi::player_no_gravity_get_fn get_fn,
    stub_minecraft_abi::player_no_gravity_set_fn set_fn) {
    std::scoped_lock lock(mutex_);
    player_no_gravity_get_hook_ = get_fn;
    player_no_gravity_set_hook_ = set_fn;
    if (auto* stub = stub_minecraft()) {
        stub->set_player_no_gravity_hooks(get_fn, set_fn);
    }
}

void bridge_host::set_player_silent_hooks(
    stub_minecraft_abi::player_silent_get_fn get_fn,
    stub_minecraft_abi::player_silent_set_fn set_fn) {
    std::scoped_lock lock(mutex_);
    player_silent_get_hook_ = get_fn;
    player_silent_set_hook_ = set_fn;
    if (auto* stub = stub_minecraft()) {
        stub->set_player_silent_hooks(get_fn, set_fn);
    }
}

void bridge_host::set_player_glowing_hooks(
    stub_minecraft_abi::player_glowing_get_fn get_fn,
    stub_minecraft_abi::player_glowing_set_fn set_fn) {
    std::scoped_lock lock(mutex_);
    player_glowing_get_hook_ = get_fn;
    player_glowing_set_hook_ = set_fn;
    if (auto* stub = stub_minecraft()) {
        stub->set_player_glowing_hooks(get_fn, set_fn);
    }
}

void bridge_host::set_player_invisible_hooks(
    stub_minecraft_abi::player_invisible_get_fn get_fn,
    stub_minecraft_abi::player_invisible_set_fn set_fn) {
    std::scoped_lock lock(mutex_);
    player_invisible_get_hook_ = get_fn;
    player_invisible_set_hook_ = set_fn;
    if (auto* stub = stub_minecraft()) {
        stub->set_player_invisible_hooks(get_fn, set_fn);
    }
}

void bridge_host::set_player_portal_cooldown_hooks(
    stub_minecraft_abi::player_portal_cooldown_get_fn get_fn,
    stub_minecraft_abi::player_portal_cooldown_set_fn set_fn) {
    std::scoped_lock lock(mutex_);
    player_portal_cooldown_get_hook_ = get_fn;
    player_portal_cooldown_set_hook_ = set_fn;
    if (auto* stub = stub_minecraft()) {
        stub->set_player_portal_cooldown_hooks(get_fn, set_fn);
    }
}

void bridge_host::set_get_player_max_air_hook(
    stub_minecraft_abi::get_player_max_air_fn fn) {
    std::scoped_lock lock(mutex_);
    get_player_max_air_hook_ = fn;
    if (auto* stub = stub_minecraft()) {
        stub->set_get_player_max_air_hook(fn);
    }
}

status bridge_host::register_player(std::uint64_t player, std::string name) {
    std::scoped_lock lock(mutex_);
    if (!ready_) {
        return err(ec::mod_state_invalid, "bridge host not initialized");
    }
    if (auto* stub = stub_minecraft()) {
        stub->register_player(player_handle{player}, std::move(name));
        return ok();
    }
    return err(ec::not_supported, "minecraft abi does not support player registry");
}

status bridge_host::unregister_player(std::uint64_t player) {
    std::scoped_lock lock(mutex_);
    if (!ready_) {
        return err(ec::mod_state_invalid, "bridge host not initialized");
    }
    if (auto* stub = stub_minecraft()) {
        stub->unregister_player(player_handle{player});
        return ok();
    }
    return err(ec::not_supported, "minecraft abi does not support player registry");
}

} // namespace leaf

extern "C" {

LEAF_BRIDGE_API leaf_error_code leaf_bridge_init(const LeafBridgeConfigV1* config) {
    if (!config) {
        return LEAF_CORE_INVALID_ARGUMENT;
    }
    const auto st = leaf::bridge_host::instance().init(*config);
    return st ? LEAF_OK : st.error().code();
}

LEAF_BRIDGE_API leaf_error_code leaf_bridge_init_flat(
    const char* minecraft_version,
    const char* leafmods_dir,
    const char* game_dir,
    uint32_t loader,
    uint32_t mapping,
    uint32_t auto_load_mods) {
    LeafBridgeConfigV1 cfg{};
    cfg.struct_size = static_cast<uint32_t>(sizeof(LeafBridgeConfigV1));
    cfg.minecraft_version = minecraft_version;
    cfg.leafmods_dir = leafmods_dir;
    cfg.game_dir = game_dir;
    cfg.loader = loader;
    cfg.mapping = mapping;
    cfg.auto_load_mods = auto_load_mods;
    return leaf_bridge_init(&cfg);
}

LEAF_BRIDGE_API leaf_error_code leaf_bridge_shutdown(void) {
    const auto st = leaf::bridge_host::instance().shutdown();
    return st ? LEAF_OK : st.error().code();
}

LEAF_BRIDGE_API int leaf_bridge_is_initialized(void) {
    return leaf::bridge_host::instance().initialized() ? 1 : 0;
}

LEAF_BRIDGE_API leaf_error_code leaf_bridge_pump_main(uint32_t budget) {
    const auto st = leaf::bridge_host::instance().pump_main(budget);
    return st ? LEAF_OK : st.error().code();
}

LEAF_BRIDGE_API leaf_error_code leaf_bridge_get_info(LeafBridgeInfoV1* out_info) {
    if (!out_info) {
        return LEAF_CORE_INVALID_ARGUMENT;
    }
    const auto st = leaf::bridge_host::instance().fill_info(*out_info);
    return st ? LEAF_OK : st.error().code();
}

LEAF_BRIDGE_API uint32_t leaf_bridge_loaded_mod_count(void) {
    return leaf::bridge_host::instance().loaded_mod_count();
}

LEAF_BRIDGE_API leaf_error_code leaf_bridge_mod_id_at(
    uint32_t index,
    char* out_buf,
    uint32_t out_buf_size) {
    const auto st =
        leaf::bridge_host::instance().mod_id_at(index, out_buf, out_buf_size);
    return st ? LEAF_OK : st.error().code();
}

LEAF_BRIDGE_API leaf_error_code leaf_bridge_on_server_starting(void) {
    const auto st = leaf::bridge_host::instance().on_server_starting();
    return st ? LEAF_OK : st.error().code();
}

LEAF_BRIDGE_API leaf_error_code leaf_bridge_on_server_started(void) {
    const auto st = leaf::bridge_host::instance().on_server_started();
    return st ? LEAF_OK : st.error().code();
}

LEAF_BRIDGE_API leaf_error_code leaf_bridge_on_server_stopping(void) {
    const auto st = leaf::bridge_host::instance().on_server_stopping();
    return st ? LEAF_OK : st.error().code();
}

LEAF_BRIDGE_API leaf_error_code leaf_bridge_emit_player_join(uint64_t player_handle) {
    const auto st = leaf::bridge_host::instance().emit_player_join(player_handle);
    return st ? LEAF_OK : st.error().code();
}

LEAF_BRIDGE_API leaf_error_code leaf_bridge_emit_player_leave(uint64_t player_handle) {
    const auto st = leaf::bridge_host::instance().emit_player_leave(player_handle);
    return st ? LEAF_OK : st.error().code();
}

LEAF_BRIDGE_API leaf_error_code leaf_bridge_emit_player_join_request(
    uint64_t player_handle,
    uint32_t* out_decision) {
    if (!out_decision) {
        return LEAF_CORE_INVALID_ARGUMENT;
    }
    auto r = leaf::bridge_host::instance().emit_player_join_request(player_handle);
    if (!r) {
        return r.error().code();
    }
    *out_decision = static_cast<uint32_t>(*r);
    return LEAF_OK;
}

LEAF_BRIDGE_API leaf_error_code leaf_bridge_emit_player_chat(
    uint64_t player_handle,
    const char* message,
    uint32_t* out_decision) {
    if (!out_decision) {
        return LEAF_CORE_INVALID_ARGUMENT;
    }
    auto r = leaf::bridge_host::instance().emit_player_chat(
        player_handle,
        message ? message : "");
    if (!r) {
        return r.error().code();
    }
    *out_decision = static_cast<uint32_t>(*r);
    return LEAF_OK;
}

LEAF_BRIDGE_API leaf_error_code leaf_bridge_emit_player_death(
    uint64_t player_handle) {
    const auto st = leaf::bridge_host::instance().emit_player_death(player_handle);
    return st ? LEAF_OK : st.error().code();
}

LEAF_BRIDGE_API leaf_error_code leaf_bridge_emit_entity_spawn(
    uint64_t entity_handle,
    uint32_t entity_type_id,
    int32_t x,
    int32_t y,
    int32_t z,
    uint32_t dimension) {
    const auto st = leaf::bridge_host::instance().emit_entity_spawn(
        entity_handle, entity_type_id, x, y, z, dimension);
    return st ? LEAF_OK : st.error().code();
}

LEAF_BRIDGE_API leaf_error_code leaf_bridge_emit_entity_remove(
    uint64_t entity_handle,
    uint32_t entity_type_id,
    int32_t x,
    int32_t y,
    int32_t z,
    uint32_t dimension) {
    const auto st = leaf::bridge_host::instance().emit_entity_remove(
        entity_handle, entity_type_id, x, y, z, dimension);
    return st ? LEAF_OK : st.error().code();
}

LEAF_BRIDGE_API leaf_error_code leaf_bridge_emit_world_load(uint32_t dimension) {
    const auto st = leaf::bridge_host::instance().emit_world_load(dimension);
    return st ? LEAF_OK : st.error().code();
}

LEAF_BRIDGE_API leaf_error_code leaf_bridge_emit_block_break(
    uint64_t player_handle,
    int32_t x,
    int32_t y,
    int32_t z,
    uint32_t block_id,
    uint32_t* out_decision) {
    if (!out_decision) {
        return LEAF_CORE_INVALID_ARGUMENT;
    }
    auto r = leaf::bridge_host::instance().emit_block_break(
        player_handle, x, y, z, block_id);
    if (!r) {
        return r.error().code();
    }
    *out_decision = static_cast<uint32_t>(*r);
    return LEAF_OK;
}

LEAF_BRIDGE_API leaf_error_code leaf_bridge_emit_block_place(
    uint64_t player_handle,
    int32_t x,
    int32_t y,
    int32_t z,
    uint32_t block_id,
    uint32_t* out_decision) {
    if (!out_decision) {
        return LEAF_CORE_INVALID_ARGUMENT;
    }
    auto r = leaf::bridge_host::instance().emit_block_place(
        player_handle, x, y, z, block_id);
    if (!r) {
        return r.error().code();
    }
    *out_decision = static_cast<uint32_t>(*r);
    return LEAF_OK;
}

LEAF_BRIDGE_API leaf_error_code leaf_bridge_reload_mods(void) {
    const auto st = leaf::bridge_host::instance().reload_mods();
    return st ? LEAF_OK : st.error().code();
}

LEAF_BRIDGE_API leaf_error_code leaf_bridge_set_minecraft_hooks(
    const LeafMinecraftHooksV1* hooks) {
    if (!hooks) {
        leaf::bridge_host::instance().set_send_player_message_hook(nullptr);
        leaf::bridge_host::instance().set_broadcast_message_hook(nullptr);
        leaf::bridge_host::instance().set_inventory_hooks(nullptr, nullptr);
        leaf::bridge_host::instance().set_get_block_hook(nullptr);
        return LEAF_OK;
    }
    if (hooks->struct_size < sizeof(LeafMinecraftHooksV1)) {
        return LEAF_ABI_VERSION_MISMATCH;
    }
    leaf::bridge_host::instance().set_send_player_message_hook(
        hooks->send_player_message);
    leaf::bridge_host::instance().set_broadcast_message_hook(
        hooks->broadcast_message);
    leaf::bridge_host::instance().set_inventory_hooks(
        hooks->get_inventory_slot,
        hooks->set_inventory_slot);
    leaf::bridge_host::instance().set_get_block_hook(hooks->get_block);
    return LEAF_OK;
}

LEAF_BRIDGE_API void leaf_bridge_set_send_player_message_hook(
    void (*fn)(uint64_t player_handle, const char* message)) {
    leaf::bridge_host::instance().set_send_player_message_hook(fn);
}

LEAF_BRIDGE_API void leaf_bridge_set_broadcast_message_hook(
    void (*fn)(const char* message)) {
    leaf::bridge_host::instance().set_broadcast_message_hook(fn);
}

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
        uint32_t count)) {
    leaf::bridge_host::instance().set_inventory_hooks(get_fn, set_fn);
}

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
        int32_t damage)) {
    leaf::bridge_host::instance().set_inventory_stack_hooks(get_fn, set_fn);
}

LEAF_BRIDGE_API void leaf_bridge_set_get_block_hook(
    int (*fn)(
        uint32_t dimension,
        int32_t x,
        int32_t y,
        int32_t z,
        uint32_t* out_block_id)) {
    leaf::bridge_host::instance().set_get_block_hook(fn);
}

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
        uint32_t block_id)) {
    leaf::bridge_host::instance().set_world_hooks(get_fn, set_fn);
}

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
        uint32_t dimension)) {
    leaf::bridge_host::instance().set_player_pos_hooks(get_fn, set_fn);
}

LEAF_BRIDGE_API void leaf_bridge_set_player_health_hooks(
    int (*get_fn)(
        uint64_t player_handle,
        float* out_health,
        float* out_max_health),
    int (*set_fn)(uint64_t player_handle, float health)) {
    leaf::bridge_host::instance().set_player_health_hooks(get_fn, set_fn);
}

LEAF_BRIDGE_API void leaf_bridge_set_player_food_hooks(
    int (*get_fn)(
        uint64_t player_handle,
        int32_t* out_food,
        float* out_saturation),
    int (*set_fn)(uint64_t player_handle, int32_t food, float saturation)) {
    leaf::bridge_host::instance().set_player_food_hooks(get_fn, set_fn);
}

LEAF_BRIDGE_API void leaf_bridge_set_player_gamemode_hooks(
    int (*get_fn)(uint64_t player_handle, uint32_t* out_mode),
    int (*set_fn)(uint64_t player_handle, uint32_t mode)) {
    leaf::bridge_host::instance().set_player_gamemode_hooks(get_fn, set_fn);
}

LEAF_BRIDGE_API void leaf_bridge_set_player_xp_hooks(
    int (*get_fn)(
        uint64_t player_handle,
        int32_t* out_level,
        float* out_progress),
    int (*set_fn)(uint64_t player_handle, int32_t level)) {
    leaf::bridge_host::instance().set_player_xp_hooks(get_fn, set_fn);
}

LEAF_BRIDGE_API void leaf_bridge_set_player_look_hooks(
    int (*get_fn)(uint64_t player_handle, float* out_yaw, float* out_pitch),
    int (*set_fn)(uint64_t player_handle, float yaw, float pitch)) {
    leaf::bridge_host::instance().set_player_look_hooks(get_fn, set_fn);
}

LEAF_BRIDGE_API void leaf_bridge_set_play_sound_hook(
    int (*fn)(
        uint64_t player_handle,
        const char* sound_id,
        float volume,
        float pitch,
        int32_t x,
        int32_t y,
        int32_t z,
        uint32_t dimension)) {
    leaf::bridge_host::instance().set_play_sound_hook(fn);
}

LEAF_BRIDGE_API void leaf_bridge_set_actionbar_hook(
    int (*fn)(uint64_t player_handle, const char* message)) {
    leaf::bridge_host::instance().set_actionbar_hook(fn);
}

LEAF_BRIDGE_API void leaf_bridge_set_title_hook(
    int (*fn)(
        uint64_t player_handle,
        const char* title,
        const char* subtitle,
        int32_t fade_in_ticks,
        int32_t stay_ticks,
        int32_t fade_out_ticks)) {
    leaf::bridge_host::instance().set_title_hook(fn);
}

LEAF_BRIDGE_API void leaf_bridge_set_kick_player_hook(
    int (*fn)(uint64_t player_handle, const char* reason)) {
    leaf::bridge_host::instance().set_kick_hook(fn);
}

LEAF_BRIDGE_API void leaf_bridge_set_give_item_hook(
    int (*fn)(
        uint64_t player_handle,
        uint32_t item_id,
        uint32_t count,
        int32_t damage)) {
    leaf::bridge_host::instance().set_give_item_hook(fn);
}

LEAF_BRIDGE_API void leaf_bridge_set_effect_hooks(
    int (*apply_fn)(
        uint64_t player_handle,
        const char* effect_id,
        int32_t duration_ticks,
        int32_t amplifier,
        uint32_t flags),
    int (*clear_fn)(uint64_t player_handle)) {
    leaf::bridge_host::instance().set_effect_hooks(apply_fn, clear_fn);
}

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
        double speed)) {
    leaf::bridge_host::instance().set_spawn_particle_hook(fn);
}

LEAF_BRIDGE_API void leaf_bridge_set_world_time_hooks(
    int (*get_fn)(uint32_t dimension, int64_t* out_time),
    int (*set_fn)(uint32_t dimension, int64_t time)) {
    leaf::bridge_host::instance().set_world_time_hooks(get_fn, set_fn);
}

LEAF_BRIDGE_API void leaf_bridge_set_player_velocity_hooks(
    int (*get_fn)(uint64_t player_handle, double* out_vx, double* out_vy, double* out_vz),
    int (*set_fn)(uint64_t player_handle, double vx, double vy, double vz)) {
    leaf::bridge_host::instance().set_player_velocity_hooks(get_fn, set_fn);
}

LEAF_BRIDGE_API void leaf_bridge_set_player_flags_hook(
    int (*fn)(uint64_t player_handle, uint32_t* out_flags)) {
    leaf::bridge_host::instance().set_player_flags_hook(fn);
}

LEAF_BRIDGE_API void leaf_bridge_set_run_command_hook(
    int (*fn)(uint64_t player_handle, const char* command)) {
    leaf::bridge_host::instance().set_run_command_hook(fn);
}

LEAF_BRIDGE_API void leaf_bridge_set_clear_inventory_hook(
    int (*fn)(uint64_t player_handle)) {
    leaf::bridge_host::instance().set_clear_inventory_hook(fn);
}

LEAF_BRIDGE_API void leaf_bridge_set_player_flight_hook(
    int (*fn)(uint64_t player_handle, int32_t allow_flight, int32_t flying)) {
    leaf::bridge_host::instance().set_player_flight_hook(fn);
}

LEAF_BRIDGE_API void leaf_bridge_set_get_biome_hook(
    int (*fn)(
        uint32_t dimension,
        int32_t x,
        int32_t y,
        int32_t z,
        char* out_buf,
        uint32_t out_buf_size)) {
    leaf::bridge_host::instance().set_get_biome_hook(fn);
}

LEAF_BRIDGE_API void leaf_bridge_set_difficulty_hooks(
    int (*get_fn)(uint32_t* out_difficulty),
    int (*set_fn)(uint32_t difficulty)) {
    leaf::bridge_host::instance().set_difficulty_hooks(get_fn, set_fn);
}

LEAF_BRIDGE_API void leaf_bridge_set_weather_hooks(
    int (*get_fn)(uint32_t dimension, uint32_t* out_weather),
    int (*set_fn)(uint32_t dimension, uint32_t weather, int32_t duration_ticks)) {
    leaf::bridge_host::instance().set_weather_hooks(get_fn, set_fn);
}

LEAF_BRIDGE_API void leaf_bridge_set_get_light_level_hook(
    int (*fn)(
        uint32_t dimension,
        int32_t x,
        int32_t y,
        int32_t z,
        uint32_t* out_block_light,
        uint32_t* out_sky_light)) {
    leaf::bridge_host::instance().set_get_light_level_hook(fn);
}

LEAF_BRIDGE_API void leaf_bridge_set_player_latency_hook(
    int (*fn)(uint64_t player_handle, int32_t* out_ms)) {
    leaf::bridge_host::instance().set_player_latency_hook(fn);
}

LEAF_BRIDGE_API void leaf_bridge_set_world_spawn_hooks(
    int (*get_fn)(
        uint32_t dimension,
        int32_t* out_x,
        int32_t* out_y,
        int32_t* out_z),
    int (*set_fn)(uint32_t dimension, int32_t x, int32_t y, int32_t z)) {
    leaf::bridge_host::instance().set_world_spawn_hooks(get_fn, set_fn);
}

LEAF_BRIDGE_API void leaf_bridge_set_player_op_hook(
    int (*fn)(uint64_t player_handle, int32_t* out_op)) {
    leaf::bridge_host::instance().set_player_op_hook(fn);
}

LEAF_BRIDGE_API void leaf_bridge_set_player_uuid_hook(
    int (*fn)(uint64_t player_handle, char* out_buf, uint32_t out_buf_size)) {
    leaf::bridge_host::instance().set_player_uuid_hook(fn);
}

LEAF_BRIDGE_API void leaf_bridge_set_player_permission_hook(
    int (*fn)(uint64_t player_handle, int32_t* out_level)) {
    leaf::bridge_host::instance().set_player_permission_hook(fn);
}

LEAF_BRIDGE_API void leaf_bridge_set_find_player_uuid_hook(
    int (*fn)(const char* uuid, uint64_t* out_player)) {
    leaf::bridge_host::instance().set_find_player_uuid_hook(fn);
}

LEAF_BRIDGE_API void leaf_bridge_set_find_player_name_hook(
    int (*fn)(const char* name, uint64_t* out_player)) {
    leaf::bridge_host::instance().set_find_player_name_hook(fn);
}

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
        const char* block_id)) {
    leaf::bridge_host::instance().set_block_registry_hooks(get_fn, set_fn);
}

LEAF_BRIDGE_API void leaf_bridge_set_give_item_registry_hook(
    int (*fn)(
        uint64_t player_handle,
        const char* item_id,
        uint32_t count,
        int32_t damage)) {
    leaf::bridge_host::instance().set_give_item_registry_hook(fn);
}

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
        int32_t damage)) {
    leaf::bridge_host::instance().set_inventory_registry_hooks(get_fn, set_fn);
}

LEAF_BRIDGE_API void leaf_bridge_set_teleport_hook(
    int (*fn)(
        uint64_t player_handle,
        int32_t x,
        int32_t y,
        int32_t z,
        uint32_t dimension,
        float yaw,
        float pitch)) {
    leaf::bridge_host::instance().set_teleport_hook(fn);
}

LEAF_BRIDGE_API void leaf_bridge_set_selected_slot_hooks(
    int (*get_fn)(uint64_t player_handle, uint32_t* out_slot),
    int (*set_fn)(uint64_t player_handle, uint32_t slot)) {
    leaf::bridge_host::instance().set_selected_slot_hooks(get_fn, set_fn);
}

LEAF_BRIDGE_API void leaf_bridge_set_get_world_seed_hook(
    int (*fn)(uint32_t dimension, int64_t* out_seed)) {
    leaf::bridge_host::instance().set_get_world_seed_hook(fn);
}

LEAF_BRIDGE_API void leaf_bridge_set_player_absorption_hooks(
    int (*get_fn)(uint64_t player_handle, float* out_absorption),
    int (*set_fn)(uint64_t player_handle, float absorption)) {
    leaf::bridge_host::instance().set_player_absorption_hooks(get_fn, set_fn);
}

LEAF_BRIDGE_API void leaf_bridge_set_player_invulnerable_hooks(
    int (*get_fn)(uint64_t player_handle, int32_t* out_invulnerable),
    int (*set_fn)(uint64_t player_handle, int32_t invulnerable)) {
    leaf::bridge_host::instance().set_player_invulnerable_hooks(get_fn, set_fn);
}

LEAF_BRIDGE_API void leaf_bridge_set_player_air_hooks(
    int (*get_fn)(uint64_t player_handle, int32_t* out_air),
    int (*set_fn)(uint64_t player_handle, int32_t air)) {
    leaf::bridge_host::instance().set_player_air_hooks(get_fn, set_fn);
}

LEAF_BRIDGE_API void leaf_bridge_set_player_fire_ticks_hooks(
    int (*get_fn)(uint64_t player_handle, int32_t* out_ticks),
    int (*set_fn)(uint64_t player_handle, int32_t ticks)) {
    leaf::bridge_host::instance().set_player_fire_ticks_hooks(get_fn, set_fn);
}

LEAF_BRIDGE_API void leaf_bridge_set_player_frozen_ticks_hooks(
    int (*get_fn)(uint64_t player_handle, int32_t* out_ticks),
    int (*set_fn)(uint64_t player_handle, int32_t ticks)) {
    leaf::bridge_host::instance().set_player_frozen_ticks_hooks(get_fn, set_fn);
}

LEAF_BRIDGE_API void leaf_bridge_set_player_no_gravity_hooks(
    int (*get_fn)(uint64_t player_handle, int32_t* out_no_gravity),
    int (*set_fn)(uint64_t player_handle, int32_t no_gravity)) {
    leaf::bridge_host::instance().set_player_no_gravity_hooks(get_fn, set_fn);
}

LEAF_BRIDGE_API void leaf_bridge_set_player_silent_hooks(
    int (*get_fn)(uint64_t player_handle, int32_t* out_silent),
    int (*set_fn)(uint64_t player_handle, int32_t silent)) {
    leaf::bridge_host::instance().set_player_silent_hooks(get_fn, set_fn);
}

LEAF_BRIDGE_API void leaf_bridge_set_player_glowing_hooks(
    int (*get_fn)(uint64_t player_handle, int32_t* out_glowing),
    int (*set_fn)(uint64_t player_handle, int32_t glowing)) {
    leaf::bridge_host::instance().set_player_glowing_hooks(get_fn, set_fn);
}

LEAF_BRIDGE_API void leaf_bridge_set_player_invisible_hooks(
    int (*get_fn)(uint64_t player_handle, int32_t* out_invisible),
    int (*set_fn)(uint64_t player_handle, int32_t invisible)) {
    leaf::bridge_host::instance().set_player_invisible_hooks(get_fn, set_fn);
}

LEAF_BRIDGE_API void leaf_bridge_set_player_portal_cooldown_hooks(
    int (*get_fn)(uint64_t player_handle, int32_t* out_ticks),
    int (*set_fn)(uint64_t player_handle, int32_t ticks)) {
    leaf::bridge_host::instance().set_player_portal_cooldown_hooks(get_fn, set_fn);
}

LEAF_BRIDGE_API void leaf_bridge_set_get_player_max_air_hook(
    int (*fn)(uint64_t player_handle, int32_t* out_max_air)) {
    leaf::bridge_host::instance().set_get_player_max_air_hook(fn);
}

LEAF_BRIDGE_API leaf_error_code leaf_bridge_register_player(
    uint64_t player_handle,
    const char* name) {
    const auto st = leaf::bridge_host::instance().register_player(
        player_handle,
        name ? name : "");
    return st ? LEAF_OK : st.error().code();
}

LEAF_BRIDGE_API leaf_error_code leaf_bridge_unregister_player(
    uint64_t player_handle) {
    const auto st = leaf::bridge_host::instance().unregister_player(player_handle);
    return st ? LEAF_OK : st.error().code();
}

} // extern "C"
