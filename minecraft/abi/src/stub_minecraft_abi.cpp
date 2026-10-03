#include "leaf/minecraft/stub_minecraft_abi.hpp"

#include "leaf/core/error.hpp"

#include <cctype>
#include <cstdio>
#include <iterator>
#include <utility>

namespace leaf {

namespace {

[[nodiscard]] std::uint64_t pack_block(std::int32_t x, std::int32_t y, std::int32_t z) noexcept {
    // Compact key for stub world (not world-coordinate-safe; tests only).
    const auto ux = static_cast<std::uint64_t>(static_cast<std::uint32_t>(x));
    const auto uy = static_cast<std::uint64_t>(static_cast<std::uint16_t>(y));
    const auto uz = static_cast<std::uint64_t>(static_cast<std::uint32_t>(z));
    return (ux << 32) | (uy << 16) | (uz & 0xffffu);
}

[[nodiscard]] std::uint64_t pack_block_dim(
    std::uint32_t dimension,
    std::int32_t x,
    std::int32_t y,
    std::int32_t z) noexcept {
    return (static_cast<std::uint64_t>(dimension & 0xfu) << 60) | pack_block(x, y, z);
}

} // namespace

stub_minecraft_abi::stub_minecraft_abi(
    version mc,
    loader_kind loader,
    capability_set caps)
    : version_(mc)
    , loader_(loader)
    , caps_(std::move(caps)) {}

version stub_minecraft_abi::minecraft_version() const noexcept {
    return version_;
}

loader_kind stub_minecraft_abi::loader() const noexcept {
    return loader_;
}

capability_set stub_minecraft_abi::capabilities() const noexcept {
    return caps_;
}

result<server_handle> stub_minecraft_abi::get_server() {
    return server_;
}

status stub_minecraft_abi::send_player_message(
    player_handle player,
    std::string_view message) {
    if (!player.valid()) {
        return err(ec::invalid_handle, "null player handle");
    }
    const auto it = players_.find(player.raw());
    const std::string name = it == players_.end() ? ("player#" + std::to_string(player.raw()))
                                                  : it->second;
    message_log_.push_back(name + ": " + std::string{message});
    if (send_hook_ && message.data() != nullptr) {
        send_hook_(player.raw(), message.data());
    }
    return ok();
}

status stub_minecraft_abi::broadcast_message(std::string_view message) {
    message_log_.push_back(std::string{"*broadcast*: "} + std::string{message});
    if (broadcast_hook_ && message.data() != nullptr) {
        broadcast_hook_(message.data());
        return ok();
    }
    if (send_hook_ && message.data() != nullptr) {
        for (const auto& [raw, name] : players_) {
            (void)name;
            send_hook_(raw, message.data());
        }
    }
    return ok();
}

std::uint32_t stub_minecraft_abi::player_count() const noexcept {
    return static_cast<std::uint32_t>(players_.size());
}

status stub_minecraft_abi::get_player_at(
    std::uint32_t index,
    player_handle& out_player) {
    if (index >= players_.size()) {
        return err(ec::not_found, "player index out of range");
    }
    auto it = players_.begin();
    std::advance(it, static_cast<std::ptrdiff_t>(index));
    out_player = player_handle{it->first};
    return ok();
}

result<std::string> stub_minecraft_abi::player_name(player_handle player) {
    if (!player.valid()) {
        return err<std::string>(ec::invalid_handle, "null player handle");
    }
    const auto it = players_.find(player.raw());
    if (it == players_.end()) {
        return err<std::string>(ec::not_found, "unknown player handle");
    }
    return it->second;
}

status stub_minecraft_abi::get_inventory_slot(
    player_handle player,
    std::uint32_t slot,
    std::uint32_t& out_item_id,
    std::uint32_t& out_count) {
    LeafItemStackV1 stack{};
    auto st = get_inventory_stack(player, slot, stack);
    if (!st) {
        return st;
    }
    out_item_id = stack.item_id;
    out_count = stack.count;
    return ok();
}

status stub_minecraft_abi::set_inventory_slot(
    player_handle player,
    std::uint32_t slot,
    std::uint32_t item_id,
    std::uint32_t count) {
    LeafItemStackV1 stack{};
    stack.struct_size = static_cast<std::uint32_t>(sizeof(LeafItemStackV1));
    stack.item_id = item_id;
    stack.count = count;
    stack.damage = -1;
    return set_inventory_stack(player, slot, stack);
}

status stub_minecraft_abi::get_inventory_stack(
    player_handle player,
    std::uint32_t slot,
    LeafItemStackV1& out) {
    if (!player.valid()) {
        return err(ec::invalid_handle, "null player handle");
    }
    if (slot > 40) {
        return err(ec::invalid_argument, "inventory slot out of range");
    }
    if (inv_stack_get_hook_) {
        std::uint32_t id = 0;
        std::uint32_t count = 0;
        std::int32_t damage = -1;
        if (inv_stack_get_hook_(player.raw(), slot, &id, &count, &damage) != 0) {
            return err(ec::not_found, "inventory stack hook failed");
        }
        out.struct_size = static_cast<std::uint32_t>(sizeof(LeafItemStackV1));
        out.item_id = id;
        out.count = count;
        out.damage = damage;
        return ok();
    }
    if (inv_get_hook_) {
        std::uint32_t id = 0;
        std::uint32_t count = 0;
        if (inv_get_hook_(player.raw(), slot, &id, &count) != 0) {
            return err(ec::not_found, "inventory hook failed");
        }
        out.struct_size = static_cast<std::uint32_t>(sizeof(LeafItemStackV1));
        out.item_id = id;
        out.count = count;
        out.damage = -1;
        return ok();
    }
    out.struct_size = static_cast<std::uint32_t>(sizeof(LeafItemStackV1));
    const auto pit = inventories_.find(player.raw());
    if (pit == inventories_.end()) {
        out.item_id = 0;
        out.count = 0;
        out.damage = -1;
        return ok();
    }
    const auto sit = pit->second.find(slot);
    if (sit == pit->second.end()) {
        out.item_id = 0;
        out.count = 0;
        out.damage = -1;
        return ok();
    }
    out.item_id = std::get<0>(sit->second);
    out.count = std::get<1>(sit->second);
    out.damage = std::get<2>(sit->second);
    return ok();
}

status stub_minecraft_abi::set_inventory_stack(
    player_handle player,
    std::uint32_t slot,
    const LeafItemStackV1& stack) {
    if (!player.valid()) {
        return err(ec::invalid_handle, "null player handle");
    }
    if (slot > 40) {
        return err(ec::invalid_argument, "inventory slot out of range");
    }
    if (inv_stack_set_hook_) {
        if (inv_stack_set_hook_(
                player.raw(),
                slot,
                stack.item_id,
                stack.count,
                stack.damage)
            != 0) {
            return err(ec::not_supported, "inventory stack set hook failed");
        }
        return ok();
    }
    if (inv_set_hook_) {
        if (inv_set_hook_(player.raw(), slot, stack.item_id, stack.count) != 0) {
            return err(ec::not_supported, "inventory set hook failed");
        }
        return ok();
    }
    if (stack.item_id == 0 || stack.count == 0) {
        auto& inv = inventories_[player.raw()];
        inv.erase(slot);
        return ok();
    }
    inventories_[player.raw()][slot] = {
        stack.item_id,
        stack.count,
        stack.damage,
    };
    return ok();
}

status stub_minecraft_abi::get_block(
    std::int32_t x,
    std::int32_t y,
    std::int32_t z,
    std::uint32_t& out_block_id) {
    return get_block_dim(0, x, y, z, out_block_id);
}

status stub_minecraft_abi::set_block(
    std::int32_t x,
    std::int32_t y,
    std::int32_t z,
    std::uint32_t block_id) {
    return set_block_dim(0, x, y, z, block_id);
}

status stub_minecraft_abi::get_block_dim(
    std::uint32_t dimension,
    std::int32_t x,
    std::int32_t y,
    std::int32_t z,
    std::uint32_t& out_block_id) {
    if (block_get_hook_) {
        std::uint32_t id = 0;
        if (block_get_hook_(dimension, x, y, z, &id) != 0) {
            return err(ec::not_found, "block hook failed");
        }
        out_block_id = id;
        return ok();
    }
    if (dimension != 0) {
        out_block_id = 0;
        return ok();
    }
    const auto key = pack_block(x, y, z);
    const auto it = blocks_.find(key);
    out_block_id = it == blocks_.end() ? 0u : it->second;
    return ok();
}

status stub_minecraft_abi::set_block_dim(
    std::uint32_t dimension,
    std::int32_t x,
    std::int32_t y,
    std::int32_t z,
    std::uint32_t block_id) {
    if (block_set_hook_) {
        if (block_set_hook_(dimension, x, y, z, block_id) != 0) {
            return err(ec::not_supported, "block set hook failed");
        }
        return ok();
    }
    if (dimension != 0) {
        return err(ec::not_supported, "stub only stores overworld blocks");
    }
    set_block_for_test(x, y, z, block_id);
    return ok();
}

status stub_minecraft_abi::get_player_pos(
    player_handle player,
    std::int32_t& out_x,
    std::int32_t& out_y,
    std::int32_t& out_z,
    std::uint32_t& out_dimension) {
    if (!player.valid()) {
        return err(ec::invalid_handle, "null player handle");
    }
    if (player_pos_get_hook_) {
        std::int32_t x = 0;
        std::int32_t y = 0;
        std::int32_t z = 0;
        std::uint32_t dim = 0;
        if (player_pos_get_hook_(player.raw(), &x, &y, &z, &dim) != 0) {
            return err(ec::not_found, "player pos hook failed");
        }
        out_x = x;
        out_y = y;
        out_z = z;
        out_dimension = dim;
        return ok();
    }
    if (players_.find(player.raw()) == players_.end()) {
        return err(ec::not_found, "unknown player handle");
    }
    const auto it = positions_.find(player.raw());
    if (it == positions_.end()) {
        out_x = 0;
        out_y = 64;
        out_z = 0;
        out_dimension = 0;
        return ok();
    }
    out_x = std::get<0>(it->second);
    out_y = std::get<1>(it->second);
    out_z = std::get<2>(it->second);
    out_dimension = std::get<3>(it->second);
    return ok();
}

status stub_minecraft_abi::set_player_pos(
    player_handle player,
    std::int32_t x,
    std::int32_t y,
    std::int32_t z,
    std::uint32_t dimension) {
    if (!player.valid()) {
        return err(ec::invalid_handle, "null player handle");
    }
    if (dimension > 2) {
        return err(ec::invalid_argument, "dimension out of range");
    }
    if (player_pos_set_hook_) {
        if (player_pos_set_hook_(player.raw(), x, y, z, dimension) != 0) {
            return err(ec::not_supported, "player pos set hook failed");
        }
        return ok();
    }
    if (players_.find(player.raw()) == players_.end()) {
        return err(ec::not_found, "unknown player handle");
    }
    positions_[player.raw()] = {x, y, z, dimension};
    return ok();
}

status stub_minecraft_abi::get_player_health(
    player_handle player,
    float& out_health,
    float& out_max_health) {
    if (!player.valid()) {
        return err(ec::invalid_handle, "null player handle");
    }
    if (player_health_get_hook_) {
        float health = 0.f;
        float max_health = 0.f;
        if (player_health_get_hook_(player.raw(), &health, &max_health) != 0) {
            return err(ec::not_found, "player health hook failed");
        }
        out_health = health;
        out_max_health = max_health;
        return ok();
    }
    if (players_.find(player.raw()) == players_.end()) {
        return err(ec::not_found, "unknown player handle");
    }
    const auto it = health_.find(player.raw());
    if (it == health_.end()) {
        out_health = 20.f;
        out_max_health = 20.f;
        return ok();
    }
    out_health = it->second.first;
    out_max_health = it->second.second;
    return ok();
}

status stub_minecraft_abi::set_player_health(player_handle player, float health) {
    if (!player.valid()) {
        return err(ec::invalid_handle, "null player handle");
    }
    if (player_health_set_hook_) {
        if (player_health_set_hook_(player.raw(), health) != 0) {
            return err(ec::not_supported, "player health set hook failed");
        }
        return ok();
    }
    if (players_.find(player.raw()) == players_.end()) {
        return err(ec::not_found, "unknown player handle");
    }
    float max_health = 20.f;
    auto it = health_.find(player.raw());
    if (it != health_.end()) {
        max_health = it->second.second;
    }
    if (health < 0.f) {
        health = 0.f;
    }
    if (health > max_health) {
        health = max_health;
    }
    health_[player.raw()] = {health, max_health};
    return ok();
}

status stub_minecraft_abi::get_player_food(
    player_handle player,
    std::int32_t& out_food,
    float& out_saturation) {
    if (!player.valid()) {
        return err(ec::invalid_handle, "null player handle");
    }
    if (player_food_get_hook_) {
        std::int32_t food = 0;
        float sat = 0.f;
        if (player_food_get_hook_(player.raw(), &food, &sat) != 0) {
            return err(ec::not_found, "player food hook failed");
        }
        out_food = food;
        out_saturation = sat;
        return ok();
    }
    if (players_.find(player.raw()) == players_.end()) {
        return err(ec::not_found, "unknown player handle");
    }
    const auto it = food_.find(player.raw());
    if (it == food_.end()) {
        out_food = 20;
        out_saturation = 5.f;
        return ok();
    }
    out_food = it->second.first;
    out_saturation = it->second.second;
    return ok();
}

status stub_minecraft_abi::set_player_food(
    player_handle player,
    std::int32_t food,
    float saturation) {
    if (!player.valid()) {
        return err(ec::invalid_handle, "null player handle");
    }
    if (player_food_set_hook_) {
        if (player_food_set_hook_(player.raw(), food, saturation) != 0) {
            return err(ec::not_supported, "player food set hook failed");
        }
        return ok();
    }
    if (players_.find(player.raw()) == players_.end()) {
        return err(ec::not_found, "unknown player handle");
    }
    if (food < 0) {
        food = 0;
    }
    if (food > 20) {
        food = 20;
    }
    if (saturation < 0.f) {
        saturation = 0.f;
    }
    food_[player.raw()] = {food, saturation};
    return ok();
}

status stub_minecraft_abi::get_player_gamemode(
    player_handle player,
    std::uint32_t& out_mode) {
    if (!player.valid()) {
        return err(ec::invalid_handle, "null player handle");
    }
    if (player_gamemode_get_hook_) {
        std::uint32_t mode = 0;
        if (player_gamemode_get_hook_(player.raw(), &mode) != 0) {
            return err(ec::not_found, "player gamemode hook failed");
        }
        out_mode = mode;
        return ok();
    }
    if (players_.find(player.raw()) == players_.end()) {
        return err(ec::not_found, "unknown player handle");
    }
    const auto it = gamemodes_.find(player.raw());
    out_mode = it == gamemodes_.end() ? 0u : it->second;
    return ok();
}

status stub_minecraft_abi::set_player_gamemode(
    player_handle player,
    std::uint32_t mode) {
    if (!player.valid()) {
        return err(ec::invalid_handle, "null player handle");
    }
    if (mode > 3) {
        return err(ec::invalid_argument, "gamemode out of range");
    }
    if (player_gamemode_set_hook_) {
        if (player_gamemode_set_hook_(player.raw(), mode) != 0) {
            return err(ec::not_supported, "player gamemode set hook failed");
        }
        return ok();
    }
    if (players_.find(player.raw()) == players_.end()) {
        return err(ec::not_found, "unknown player handle");
    }
    gamemodes_[player.raw()] = mode;
    return ok();
}

status stub_minecraft_abi::get_player_xp(
    player_handle player,
    std::int32_t& out_level,
    float& out_progress) {
    if (!player.valid()) {
        return err(ec::invalid_handle, "null player handle");
    }
    if (player_xp_get_hook_) {
        std::int32_t level = 0;
        float progress = 0.f;
        if (player_xp_get_hook_(player.raw(), &level, &progress) != 0) {
            return err(ec::not_found, "player xp hook failed");
        }
        out_level = level;
        out_progress = progress;
        return ok();
    }
    if (players_.find(player.raw()) == players_.end()) {
        return err(ec::not_found, "unknown player handle");
    }
    const auto it = xp_.find(player.raw());
    if (it == xp_.end()) {
        out_level = 0;
        out_progress = 0.f;
        return ok();
    }
    out_level = it->second.first;
    out_progress = it->second.second;
    return ok();
}

status stub_minecraft_abi::set_player_xp_level(
    player_handle player,
    std::int32_t level) {
    if (!player.valid()) {
        return err(ec::invalid_handle, "null player handle");
    }
    if (level < 0) {
        return err(ec::invalid_argument, "xp level negative");
    }
    if (player_xp_set_hook_) {
        if (player_xp_set_hook_(player.raw(), level) != 0) {
            return err(ec::not_supported, "player xp set hook failed");
        }
        return ok();
    }
    if (players_.find(player.raw()) == players_.end()) {
        return err(ec::not_found, "unknown player handle");
    }
    xp_[player.raw()] = {level, 0.f};
    return ok();
}

status stub_minecraft_abi::get_player_look(
    player_handle player,
    float& out_yaw,
    float& out_pitch) {
    if (!player.valid()) {
        return err(ec::invalid_handle, "null player handle");
    }
    if (player_look_get_hook_) {
        float yaw = 0.f;
        float pitch = 0.f;
        if (player_look_get_hook_(player.raw(), &yaw, &pitch) != 0) {
            return err(ec::not_found, "player look hook failed");
        }
        out_yaw = yaw;
        out_pitch = pitch;
        return ok();
    }
    if (players_.find(player.raw()) == players_.end()) {
        return err(ec::not_found, "unknown player handle");
    }
    const auto it = looks_.find(player.raw());
    if (it == looks_.end()) {
        out_yaw = 0.f;
        out_pitch = 0.f;
        return ok();
    }
    out_yaw = it->second.first;
    out_pitch = it->second.second;
    return ok();
}

status stub_minecraft_abi::set_player_look(
    player_handle player,
    float yaw,
    float pitch) {
    if (!player.valid()) {
        return err(ec::invalid_handle, "null player handle");
    }
    if (player_look_set_hook_) {
        if (player_look_set_hook_(player.raw(), yaw, pitch) != 0) {
            return err(ec::not_supported, "player look set hook failed");
        }
        return ok();
    }
    if (players_.find(player.raw()) == players_.end()) {
        return err(ec::not_found, "unknown player handle");
    }
    looks_[player.raw()] = {yaw, pitch};
    return ok();
}

status stub_minecraft_abi::play_sound(
    player_handle player,
    std::string_view sound_id,
    float volume,
    float pitch,
    std::int32_t x,
    std::int32_t y,
    std::int32_t z,
    std::uint32_t dimension) {
    if (sound_id.empty()) {
        return err(ec::invalid_argument, "sound_id required");
    }
    if (play_sound_hook_) {
        const std::string id{sound_id};
        if (play_sound_hook_(
                player.raw(),
                id.c_str(),
                volume,
                pitch,
                x,
                y,
                z,
                dimension)
            != 0) {
            return err(ec::not_supported, "play_sound hook failed");
        }
        return ok();
    }
    sound_log_.push_back(
        std::string{sound_id} + "@" + std::to_string(player.raw()) + ":"
        + std::to_string(x) + "," + std::to_string(y) + "," + std::to_string(z)
        + " d" + std::to_string(dimension) + " v" + std::to_string(volume)
        + " p" + std::to_string(pitch));
    return ok();
}

status stub_minecraft_abi::send_actionbar(
    player_handle player,
    std::string_view message) {
    if (!player.valid()) {
        return err(ec::invalid_handle, "null player handle");
    }
    if (message.empty()) {
        return err(ec::invalid_argument, "message required");
    }
    if (actionbar_hook_) {
        const std::string text{message};
        if (actionbar_hook_(player.raw(), text.c_str()) != 0) {
            return err(ec::not_supported, "actionbar hook failed");
        }
        return ok();
    }
    if (players_.find(player.raw()) == players_.end()) {
        return err(ec::not_found, "unknown player handle");
    }
    actionbar_log_.push_back(std::string{message});
    return ok();
}

status stub_minecraft_abi::send_title(
    player_handle player,
    std::string_view title,
    std::string_view subtitle,
    std::int32_t fade_in_ticks,
    std::int32_t stay_ticks,
    std::int32_t fade_out_ticks) {
    if (!player.valid()) {
        return err(ec::invalid_handle, "null player handle");
    }
    if (title_hook_) {
        const std::string t{title};
        const std::string s{subtitle};
        if (title_hook_(
                player.raw(),
                t.c_str(),
                s.c_str(),
                fade_in_ticks,
                stay_ticks,
                fade_out_ticks)
            != 0) {
            return err(ec::not_supported, "title hook failed");
        }
        return ok();
    }
    if (players_.find(player.raw()) == players_.end()) {
        return err(ec::not_found, "unknown player handle");
    }
    title_log_.push_back(
        std::string{title} + "|" + std::string{subtitle} + "@"
        + std::to_string(fade_in_ticks) + "," + std::to_string(stay_ticks) + ","
        + std::to_string(fade_out_ticks));
    return ok();
}

status stub_minecraft_abi::kick_player(
    player_handle player,
    std::string_view reason) {
    if (!player.valid()) {
        return err(ec::invalid_handle, "null player handle");
    }
    if (kick_hook_) {
        const std::string text{reason};
        if (kick_hook_(player.raw(), text.c_str()) != 0) {
            return err(ec::not_supported, "kick hook failed");
        }
        return ok();
    }
    if (players_.find(player.raw()) == players_.end()) {
        return err(ec::not_found, "unknown player handle");
    }
    kick_log_.push_back(std::string{reason});
    unregister_player(player);
    return ok();
}

status stub_minecraft_abi::give_item(
    player_handle player,
    std::uint32_t item_id,
    std::uint32_t count,
    std::int32_t damage) {
    if (!player.valid()) {
        return err(ec::invalid_handle, "null player handle");
    }
    if (item_id == 0 || count == 0) {
        return err(ec::invalid_argument, "item_id and count required");
    }
    if (give_item_hook_) {
        if (give_item_hook_(player.raw(), item_id, count, damage) != 0) {
            return err(ec::not_supported, "give_item hook failed");
        }
        return ok();
    }
    if (players_.find(player.raw()) == players_.end()) {
        return err(ec::not_found, "unknown player handle");
    }
    constexpr std::uint32_t kMaxStack = 64;
    std::uint32_t remaining = count;
    // Merge into existing matching stacks first.
    for (std::uint32_t slot = 0; slot <= 40 && remaining > 0; ++slot) {
        LeafItemStackV1 cur{};
        auto st = get_inventory_stack(player, slot, cur);
        if (!st) {
            return st;
        }
        if (cur.item_id != item_id || cur.damage != damage || cur.count >= kMaxStack) {
            continue;
        }
        const auto space = kMaxStack - cur.count;
        const auto add = remaining < space ? remaining : space;
        cur.count += add;
        remaining -= add;
        st = set_inventory_stack(player, slot, cur);
        if (!st) {
            return st;
        }
    }
    // Then fill empty slots.
    for (std::uint32_t slot = 0; slot <= 40 && remaining > 0; ++slot) {
        LeafItemStackV1 cur{};
        auto st = get_inventory_stack(player, slot, cur);
        if (!st) {
            return st;
        }
        if (cur.item_id != 0 && cur.count != 0) {
            continue;
        }
        const auto put = remaining < kMaxStack ? remaining : kMaxStack;
        LeafItemStackV1 next{};
        next.struct_size = static_cast<std::uint32_t>(sizeof(LeafItemStackV1));
        next.item_id = item_id;
        next.count = put;
        next.damage = damage;
        remaining -= put;
        st = set_inventory_stack(player, slot, next);
        if (!st) {
            return st;
        }
    }
    if (remaining > 0) {
        return err(ec::not_supported, "inventory full");
    }
    return ok();
}

status stub_minecraft_abi::apply_effect(
    player_handle player,
    std::string_view effect_id,
    std::int32_t duration_ticks,
    std::int32_t amplifier,
    std::uint32_t flags) {
    if (!player.valid()) {
        return err(ec::invalid_handle, "null player handle");
    }
    if (effect_id.empty()) {
        return err(ec::invalid_argument, "effect_id required");
    }
    if (apply_effect_hook_) {
        const std::string id{effect_id};
        if (apply_effect_hook_(
                player.raw(), id.c_str(), duration_ticks, amplifier, flags)
            != 0) {
            return err(ec::not_supported, "apply_effect hook failed");
        }
        return ok();
    }
    if (players_.find(player.raw()) == players_.end()) {
        return err(ec::not_found, "unknown player handle");
    }
    effect_log_.push_back(
        std::string{effect_id} + "@" + std::to_string(player.raw()) + ":"
        + std::to_string(duration_ticks) + "x" + std::to_string(amplifier)
        + " f" + std::to_string(flags));
    effects_[player.raw()].insert(std::string{effect_id});
    return ok();
}

status stub_minecraft_abi::clear_effects(player_handle player) {
    if (!player.valid()) {
        return err(ec::invalid_handle, "null player handle");
    }
    if (clear_effects_hook_) {
        if (clear_effects_hook_(player.raw()) != 0) {
            return err(ec::not_supported, "clear_effects hook failed");
        }
        return ok();
    }
    if (players_.find(player.raw()) == players_.end()) {
        return err(ec::not_found, "unknown player handle");
    }
    effects_.erase(player.raw());
    effect_log_.push_back("clear@" + std::to_string(player.raw()));
    return ok();
}

status stub_minecraft_abi::spawn_particle(
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
    if (particle_id.empty()) {
        return err(ec::invalid_argument, "particle_id required");
    }
    if (spawn_particle_hook_) {
        const std::string id{particle_id};
        if (spawn_particle_hook_(
                id.c_str(), x, y, z, dimension, count, dx, dy, dz, speed)
            != 0) {
            return err(ec::not_supported, "spawn_particle hook failed");
        }
        return ok();
    }
    particle_log_.push_back(
        std::string{particle_id} + "@" + std::to_string(x) + ","
        + std::to_string(y) + "," + std::to_string(z) + " d"
        + std::to_string(dimension) + " n" + std::to_string(count));
    return ok();
}

status stub_minecraft_abi::get_world_time(
    std::uint32_t dimension,
    std::int64_t& out_time) {
    if (world_time_get_hook_) {
        if (world_time_get_hook_(dimension, &out_time) != 0) {
            return err(ec::not_supported, "world time get hook failed");
        }
        return ok();
    }
    out_time = world_times_[dimension];
    return ok();
}

status stub_minecraft_abi::set_world_time(
    std::uint32_t dimension,
    std::int64_t time) {
    if (world_time_set_hook_) {
        if (world_time_set_hook_(dimension, time) != 0) {
            return err(ec::not_supported, "world time set hook failed");
        }
        return ok();
    }
    world_times_[dimension] = time;
    return ok();
}

status stub_minecraft_abi::get_player_velocity(
    player_handle player,
    double& out_vx,
    double& out_vy,
    double& out_vz) {
    if (!player.valid()) {
        return err(ec::invalid_handle, "null player handle");
    }
    if (player_velocity_get_hook_) {
        if (player_velocity_get_hook_(player.raw(), &out_vx, &out_vy, &out_vz)
            != 0) {
            return err(ec::not_supported, "player velocity get hook failed");
        }
        return ok();
    }
    if (players_.find(player.raw()) == players_.end()) {
        return err(ec::not_found, "unknown player handle");
    }
    const auto it = velocities_.find(player.raw());
    if (it == velocities_.end()) {
        out_vx = out_vy = out_vz = 0.0;
        return ok();
    }
    out_vx = std::get<0>(it->second);
    out_vy = std::get<1>(it->second);
    out_vz = std::get<2>(it->second);
    return ok();
}

status stub_minecraft_abi::set_player_velocity(
    player_handle player,
    double vx,
    double vy,
    double vz) {
    if (!player.valid()) {
        return err(ec::invalid_handle, "null player handle");
    }
    if (player_velocity_set_hook_) {
        if (player_velocity_set_hook_(player.raw(), vx, vy, vz) != 0) {
            return err(ec::not_supported, "player velocity set hook failed");
        }
        return ok();
    }
    if (players_.find(player.raw()) == players_.end()) {
        return err(ec::not_found, "unknown player handle");
    }
    velocities_[player.raw()] = {vx, vy, vz};
    return ok();
}

status stub_minecraft_abi::get_player_flags(
    player_handle player,
    std::uint32_t& out_flags) {
    if (!player.valid()) {
        return err(ec::invalid_handle, "null player handle");
    }
    if (player_flags_hook_) {
        if (player_flags_hook_(player.raw(), &out_flags) != 0) {
            return err(ec::not_supported, "player flags hook failed");
        }
        return ok();
    }
    if (players_.find(player.raw()) == players_.end()) {
        return err(ec::not_found, "unknown player handle");
    }
    const auto it = flags_.find(player.raw());
    out_flags = it == flags_.end() ? 0u : it->second;
    return ok();
}

status stub_minecraft_abi::run_command(
    player_handle player,
    std::string_view command) {
    if (command.empty()) {
        return err(ec::invalid_argument, "command required");
    }
    if (run_command_hook_) {
        const std::string text{command};
        if (run_command_hook_(player.raw(), text.c_str()) != 0) {
            return err(ec::not_supported, "run_command hook failed");
        }
        return ok();
    }
    command_log_.push_back(
        std::to_string(player.raw()) + ":" + std::string{command});
    return ok();
}

status stub_minecraft_abi::clear_inventory(player_handle player) {
    if (!player.valid()) {
        return err(ec::invalid_handle, "null player handle");
    }
    if (clear_inventory_hook_) {
        if (clear_inventory_hook_(player.raw()) != 0) {
            return err(ec::not_supported, "clear_inventory hook failed");
        }
        return ok();
    }
    if (players_.find(player.raw()) == players_.end()) {
        return err(ec::not_found, "unknown player handle");
    }
    inventories_.erase(player.raw());
    return ok();
}

status stub_minecraft_abi::broadcast_actionbar(std::string_view message) {
    for (const auto& [raw, name] : players_) {
        (void)name;
        auto st = send_actionbar(player_handle{raw}, message);
        if (!st) {
            return st;
        }
    }
    return ok();
}

status stub_minecraft_abi::broadcast_title(
    std::string_view title,
    std::string_view subtitle,
    std::int32_t fade_in_ticks,
    std::int32_t stay_ticks,
    std::int32_t fade_out_ticks) {
    for (const auto& [raw, name] : players_) {
        (void)name;
        auto st = send_title(
            player_handle{raw},
            title,
            subtitle,
            fade_in_ticks,
            stay_ticks,
            fade_out_ticks);
        if (!st) {
            return st;
        }
    }
    return ok();
}

status stub_minecraft_abi::set_player_flight(
    player_handle player,
    std::int32_t allow_flight,
    std::int32_t flying) {
    if (!player.valid()) {
        return err(ec::invalid_handle, "null player handle");
    }
    if (player_flight_hook_) {
        if (player_flight_hook_(player.raw(), allow_flight, flying) != 0) {
            return err(ec::not_supported, "set_player_flight hook failed");
        }
        return ok();
    }
    if (players_.find(player.raw()) == players_.end()) {
        return err(ec::not_found, "unknown player handle");
    }
    auto& flags = flags_[player.raw()];
    if (allow_flight >= 0) {
        if (allow_flight) {
            flags |= LEAF_PLAYER_FLAG_ALLOW_FLIGHT;
        } else {
            flags &= ~LEAF_PLAYER_FLAG_ALLOW_FLIGHT;
            flags &= ~LEAF_PLAYER_FLAG_FLYING;
        }
    }
    if (flying >= 0) {
        if (flying) {
            flags |= LEAF_PLAYER_FLAG_FLYING;
            flags |= LEAF_PLAYER_FLAG_ALLOW_FLIGHT;
        } else {
            flags &= ~LEAF_PLAYER_FLAG_FLYING;
        }
    }
    return ok();
}

status stub_minecraft_abi::get_biome(
    std::uint32_t dimension,
    std::int32_t x,
    std::int32_t y,
    std::int32_t z,
    std::string& out_id) {
    if (get_biome_hook_) {
        char buf[256]{};
        if (get_biome_hook_(dimension, x, y, z, buf, sizeof(buf)) != 0) {
            return err(ec::not_supported, "get_biome hook failed");
        }
        out_id.assign(buf);
        return ok();
    }
    (void)dimension;
    (void)x;
    (void)y;
    (void)z;
    out_id = "minecraft:plains";
    return ok();
}

status stub_minecraft_abi::get_difficulty(std::uint32_t& out_difficulty) {
    if (difficulty_get_hook_) {
        if (difficulty_get_hook_(&out_difficulty) != 0) {
            return err(ec::not_supported, "get_difficulty hook failed");
        }
        return ok();
    }
    out_difficulty = difficulty_;
    return ok();
}

status stub_minecraft_abi::set_difficulty(std::uint32_t difficulty) {
    if (difficulty > 3) {
        return err(ec::invalid_argument, "difficulty must be 0..3");
    }
    if (difficulty_set_hook_) {
        if (difficulty_set_hook_(difficulty) != 0) {
            return err(ec::not_supported, "set_difficulty hook failed");
        }
        return ok();
    }
    difficulty_ = difficulty;
    return ok();
}

status stub_minecraft_abi::get_weather(
    std::uint32_t dimension,
    std::uint32_t& out_weather) {
    if (weather_get_hook_) {
        if (weather_get_hook_(dimension, &out_weather) != 0) {
            return err(ec::not_supported, "get_weather hook failed");
        }
        return ok();
    }
    const auto it = weather_.find(dimension);
    out_weather = it == weather_.end() ? LEAF_WEATHER_CLEAR : it->second;
    return ok();
}

status stub_minecraft_abi::set_weather(
    std::uint32_t dimension,
    std::uint32_t weather,
    std::int32_t duration_ticks) {
    if (weather > LEAF_WEATHER_THUNDER) {
        return err(ec::invalid_argument, "weather must be 0..2");
    }
    if (weather_set_hook_) {
        if (weather_set_hook_(dimension, weather, duration_ticks) != 0) {
            return err(ec::not_supported, "set_weather hook failed");
        }
        return ok();
    }
    (void)duration_ticks;
    weather_[dimension] = weather;
    return ok();
}

status stub_minecraft_abi::get_light_level(
    std::uint32_t dimension,
    std::int32_t x,
    std::int32_t y,
    std::int32_t z,
    std::uint32_t& out_block_light,
    std::uint32_t& out_sky_light) {
    if (get_light_level_hook_) {
        if (get_light_level_hook_(
                dimension, x, y, z, &out_block_light, &out_sky_light)
            != 0) {
            return err(ec::not_supported, "get_light_level hook failed");
        }
        return ok();
    }
    (void)dimension;
    (void)x;
    (void)y;
    (void)z;
    out_block_light = 0;
    out_sky_light = 15;
    return ok();
}

status stub_minecraft_abi::get_player_latency(
    player_handle player,
    std::int32_t& out_ms) {
    if (!player.valid()) {
        return err(ec::invalid_handle, "null player handle");
    }
    if (player_latency_hook_) {
        if (player_latency_hook_(player.raw(), &out_ms) != 0) {
            return err(ec::not_supported, "player latency hook failed");
        }
        return ok();
    }
    if (players_.find(player.raw()) == players_.end()) {
        return err(ec::not_found, "unknown player handle");
    }
    const auto it = latencies_.find(player.raw());
    out_ms = it == latencies_.end() ? 0 : it->second;
    return ok();
}

status stub_minecraft_abi::get_world_spawn(
    std::uint32_t dimension,
    std::int32_t& out_x,
    std::int32_t& out_y,
    std::int32_t& out_z) {
    if (world_spawn_get_hook_) {
        if (world_spawn_get_hook_(dimension, &out_x, &out_y, &out_z) != 0) {
            return err(ec::not_supported, "get_world_spawn hook failed");
        }
        return ok();
    }
    const auto it = spawns_.find(dimension);
    if (it == spawns_.end()) {
        out_x = 0;
        out_y = 64;
        out_z = 0;
        return ok();
    }
    out_x = std::get<0>(it->second);
    out_y = std::get<1>(it->second);
    out_z = std::get<2>(it->second);
    return ok();
}

status stub_minecraft_abi::set_world_spawn(
    std::uint32_t dimension,
    std::int32_t x,
    std::int32_t y,
    std::int32_t z) {
    if (world_spawn_set_hook_) {
        if (world_spawn_set_hook_(dimension, x, y, z) != 0) {
            return err(ec::not_supported, "set_world_spawn hook failed");
        }
        return ok();
    }
    spawns_[dimension] = {x, y, z};
    return ok();
}

status stub_minecraft_abi::is_player_op(
    player_handle player,
    std::int32_t& out_op) {
    if (!player.valid()) {
        return err(ec::invalid_handle, "null player handle");
    }
    if (player_op_hook_) {
        if (player_op_hook_(player.raw(), &out_op) != 0) {
            return err(ec::not_supported, "is_player_op hook failed");
        }
        return ok();
    }
    if (players_.find(player.raw()) == players_.end()) {
        return err(ec::not_found, "unknown player handle");
    }
    const auto it = ops_.find(player.raw());
    out_op = it == ops_.end() ? 0 : it->second;
    return ok();
}

status stub_minecraft_abi::get_player_uuid(
    player_handle player,
    std::string& out_uuid) {
    if (!player.valid()) {
        return err(ec::invalid_handle, "null player handle");
    }
    if (player_uuid_hook_) {
        char buf[64]{};
        if (player_uuid_hook_(player.raw(), buf, sizeof(buf)) != 0) {
            return err(ec::not_supported, "get_player_uuid hook failed");
        }
        out_uuid.assign(buf);
        return ok();
    }
    if (players_.find(player.raw()) == players_.end()) {
        return err(ec::not_found, "unknown player handle");
    }
    const auto it = uuids_.find(player.raw());
    if (it != uuids_.end()) {
        out_uuid = it->second;
        return ok();
    }
    char buf[64]{};
    std::snprintf(
        buf,
        sizeof(buf),
        "00000000-0000-4000-8000-%012llx",
        static_cast<unsigned long long>(player.raw()));
    out_uuid.assign(buf);
    return ok();
}

status stub_minecraft_abi::get_player_permission_level(
    player_handle player,
    std::int32_t& out_level) {
    if (!player.valid()) {
        return err(ec::invalid_handle, "null player handle");
    }
    if (player_permission_hook_) {
        if (player_permission_hook_(player.raw(), &out_level) != 0) {
            return err(ec::not_supported, "get_player_permission_level hook failed");
        }
        return ok();
    }
    if (players_.find(player.raw()) == players_.end()) {
        return err(ec::not_found, "unknown player handle");
    }
    const auto it = permissions_.find(player.raw());
    if (it != permissions_.end()) {
        out_level = it->second;
        return ok();
    }
    const auto op_it = ops_.find(player.raw());
    out_level = (op_it != ops_.end() && op_it->second != 0) ? 4 : 0;
    return ok();
}

status stub_minecraft_abi::find_player_by_uuid(
    std::string_view uuid,
    player_handle& out_player) {
    if (uuid.empty()) {
        return err(ec::invalid_argument, "empty uuid");
    }
    if (find_player_uuid_hook_) {
        std::string copy{uuid};
        std::uint64_t handle = 0;
        if (find_player_uuid_hook_(copy.c_str(), &handle) != 0 || handle == 0) {
            return err(ec::not_found, "player uuid not online");
        }
        out_player = player_handle{handle};
        return ok();
    }
    for (const auto& [handle, stored] : uuids_) {
        if (stored.size() == uuid.size()) {
            bool match = true;
            for (std::size_t i = 0; i < uuid.size(); ++i) {
                const auto a = static_cast<unsigned char>(stored[i]);
                const auto b = static_cast<unsigned char>(uuid[i]);
                if (std::tolower(a) != std::tolower(b)) {
                    match = false;
                    break;
                }
            }
            if (match) {
                out_player = player_handle{handle};
                return ok();
            }
        }
    }
    // Match synthetic stub UUID for players without an explicit uuid entry.
    for (const auto& [handle, name] : players_) {
        (void)name;
        char buf[64]{};
        std::snprintf(
            buf,
            sizeof(buf),
            "00000000-0000-4000-8000-%012llx",
            static_cast<unsigned long long>(handle));
        if (uuid.size() == std::char_traits<char>::length(buf)) {
            bool match = true;
            for (std::size_t i = 0; i < uuid.size(); ++i) {
                const auto a = static_cast<unsigned char>(buf[i]);
                const auto b = static_cast<unsigned char>(uuid[i]);
                if (std::tolower(a) != std::tolower(b)) {
                    match = false;
                    break;
                }
            }
            if (match) {
                out_player = player_handle{handle};
                return ok();
            }
        }
    }
    return err(ec::not_found, "player uuid not online");
}

status stub_minecraft_abi::find_player_by_name(
    std::string_view name,
    player_handle& out_player) {
    if (name.empty()) {
        return err(ec::invalid_argument, "empty player name");
    }
    if (find_player_name_hook_) {
        std::string copy{name};
        std::uint64_t handle = 0;
        if (find_player_name_hook_(copy.c_str(), &handle) != 0 || handle == 0) {
            return err(ec::not_found, "player name not online");
        }
        out_player = player_handle{handle};
        return ok();
    }
    for (const auto& [handle, stored] : players_) {
        if (stored.size() != name.size()) {
            continue;
        }
        bool match = true;
        for (std::size_t i = 0; i < name.size(); ++i) {
            const auto a = static_cast<unsigned char>(stored[i]);
            const auto b = static_cast<unsigned char>(name[i]);
            if (std::tolower(a) != std::tolower(b)) {
                match = false;
                break;
            }
        }
        if (match) {
            out_player = player_handle{handle};
            return ok();
        }
    }
    return err(ec::not_found, "player name not online");
}

status stub_minecraft_abi::get_block_registry_id(
    std::uint32_t dimension,
    std::int32_t x,
    std::int32_t y,
    std::int32_t z,
    std::string& out_id) {
    if (get_block_registry_hook_) {
        char buf[256]{};
        if (get_block_registry_hook_(dimension, x, y, z, buf, sizeof(buf)) != 0) {
            return err(ec::not_supported, "get_block_registry_id hook failed");
        }
        out_id.assign(buf);
        return ok();
    }
    const auto key = pack_block_dim(dimension, x, y, z);
    const auto it = block_registry_ids_.find(key);
    if (it != block_registry_ids_.end()) {
        out_id = it->second;
        return ok();
    }
    out_id = "minecraft:air";
    return ok();
}

status stub_minecraft_abi::set_block_registry_id(
    std::uint32_t dimension,
    std::int32_t x,
    std::int32_t y,
    std::int32_t z,
    std::string_view block_id) {
    if (block_id.empty()) {
        return err(ec::invalid_argument, "empty block id");
    }
    if (set_block_registry_hook_) {
        std::string copy{block_id};
        if (set_block_registry_hook_(dimension, x, y, z, copy.c_str()) != 0) {
            return err(ec::not_supported, "set_block_registry_id hook failed");
        }
        return ok();
    }
    const auto key = pack_block_dim(dimension, x, y, z);
    if (block_id == "minecraft:air" || block_id == "air") {
        block_registry_ids_.erase(key);
        if (dimension == 0) {
            blocks_.erase(pack_block(x, y, z));
        }
        return ok();
    }
    block_registry_ids_[key] = std::string{block_id};
    if (dimension == 0) {
        blocks_[pack_block(x, y, z)] = 1; // non-air placeholder numeric id
    }
    return ok();
}

status stub_minecraft_abi::give_item_registry_id(
    player_handle player,
    std::string_view item_id,
    std::uint32_t count,
    std::int32_t damage) {
    if (!player.valid()) {
        return err(ec::invalid_handle, "null player handle");
    }
    if (item_id.empty() || count == 0) {
        return err(ec::invalid_argument, "item_id and count required");
    }
    if (give_item_registry_hook_) {
        std::string copy{item_id};
        if (give_item_registry_hook_(player.raw(), copy.c_str(), count, damage) != 0) {
            return err(ec::not_supported, "give_item_registry_id hook failed");
        }
        return ok();
    }
    // Stub: map registry string to a stable non-zero numeric id.
    std::uint32_t numeric = 1u;
    for (unsigned char c : item_id) {
        numeric = numeric * 16777619u ^ static_cast<std::uint32_t>(c);
    }
    if (numeric == 0) {
        numeric = 1;
    }
    return give_item(player, numeric, count, damage);
}

status stub_minecraft_abi::get_inventory_item_registry_id(
    player_handle player,
    std::uint32_t slot,
    std::string& out_id,
    std::uint32_t& out_count,
    std::int32_t& out_damage) {
    if (!player.valid()) {
        return err(ec::invalid_handle, "null player handle");
    }
    if (slot > 40) {
        return err(ec::invalid_argument, "inventory slot out of range");
    }
    if (inv_registry_get_hook_) {
        char buf[256]{};
        std::uint32_t count = 0;
        std::int32_t damage = -1;
        if (inv_registry_get_hook_(
                player.raw(), slot, buf, sizeof(buf), &count, &damage)
            != 0) {
            return err(ec::not_supported, "get_inventory_item_registry_id hook failed");
        }
        out_id.assign(buf);
        out_count = count;
        out_damage = damage;
        return ok();
    }
    LeafItemStackV1 stack{};
    auto st = get_inventory_stack(player, slot, stack);
    if (!st) {
        return st;
    }
    out_count = stack.count;
    out_damage = stack.damage;
    const auto key = (player.raw() << 8) | slot;
    const auto it = inv_registry_ids_.find(key);
    if (it != inv_registry_ids_.end()) {
        out_id = it->second;
    } else if (stack.item_id == 0 || stack.count == 0) {
        out_id = "minecraft:air";
    } else {
        out_id = "minecraft:unknown";
    }
    return ok();
}

status stub_minecraft_abi::set_inventory_item_registry_id(
    player_handle player,
    std::uint32_t slot,
    std::string_view item_id,
    std::uint32_t count,
    std::int32_t damage) {
    if (!player.valid()) {
        return err(ec::invalid_handle, "null player handle");
    }
    if (slot > 40) {
        return err(ec::invalid_argument, "inventory slot out of range");
    }
    if (inv_registry_set_hook_) {
        std::string copy{item_id};
        if (inv_registry_set_hook_(
                player.raw(), slot, copy.c_str(), count, damage)
            != 0) {
            return err(ec::not_supported, "set_inventory_item_registry_id hook failed");
        }
        return ok();
    }
    const auto key = (player.raw() << 8) | slot;
    if (item_id.empty() || item_id == "minecraft:air" || item_id == "air" || count == 0) {
        inv_registry_ids_.erase(key);
        LeafItemStackV1 empty{};
        empty.struct_size = static_cast<std::uint32_t>(sizeof(LeafItemStackV1));
        empty.item_id = 0;
        empty.count = 0;
        empty.damage = -1;
        return set_inventory_stack(player, slot, empty);
    }
    inv_registry_ids_[key] = std::string{item_id};
    std::uint32_t numeric = 1u;
    for (unsigned char c : item_id) {
        numeric = numeric * 16777619u ^ static_cast<std::uint32_t>(c);
    }
    if (numeric == 0) {
        numeric = 1;
    }
    LeafItemStackV1 stack{};
    stack.struct_size = static_cast<std::uint32_t>(sizeof(LeafItemStackV1));
    stack.item_id = numeric;
    stack.count = count;
    stack.damage = damage;
    return set_inventory_stack(player, slot, stack);
}

status stub_minecraft_abi::teleport_player(
    player_handle player,
    std::int32_t x,
    std::int32_t y,
    std::int32_t z,
    std::uint32_t dimension,
    float yaw,
    float pitch) {
    if (!player.valid()) {
        return err(ec::invalid_handle, "null player handle");
    }
    if (teleport_hook_) {
        if (teleport_hook_(player.raw(), x, y, z, dimension, yaw, pitch) != 0) {
            return err(ec::not_supported, "teleport_player hook failed");
        }
        return ok();
    }
    auto st = set_player_pos(player, x, y, z, dimension);
    if (!st) {
        return st;
    }
    return set_player_look(player, yaw, pitch);
}

status stub_minecraft_abi::get_selected_slot(
    player_handle player,
    std::uint32_t& out_slot) {
    if (!player.valid()) {
        return err(ec::invalid_handle, "null player handle");
    }
    if (selected_slot_get_hook_) {
        if (selected_slot_get_hook_(player.raw(), &out_slot) != 0) {
            return err(ec::not_supported, "get_selected_slot hook failed");
        }
        return ok();
    }
    if (players_.find(player.raw()) == players_.end()) {
        return err(ec::not_found, "unknown player handle");
    }
    const auto it = selected_slots_.find(player.raw());
    out_slot = it == selected_slots_.end() ? 0u : it->second;
    return ok();
}

status stub_minecraft_abi::set_selected_slot(
    player_handle player,
    std::uint32_t slot) {
    if (!player.valid()) {
        return err(ec::invalid_handle, "null player handle");
    }
    if (slot > 8) {
        return err(ec::invalid_argument, "selected slot must be 0..8");
    }
    if (selected_slot_set_hook_) {
        if (selected_slot_set_hook_(player.raw(), slot) != 0) {
            return err(ec::not_supported, "set_selected_slot hook failed");
        }
        return ok();
    }
    if (players_.find(player.raw()) == players_.end()) {
        return err(ec::not_found, "unknown player handle");
    }
    selected_slots_[player.raw()] = slot;
    return ok();
}

status stub_minecraft_abi::get_world_seed(
    std::uint32_t dimension,
    std::int64_t& out_seed) {
    if (get_world_seed_hook_) {
        if (get_world_seed_hook_(dimension, &out_seed) != 0) {
            return err(ec::not_supported, "get_world_seed hook failed");
        }
        return ok();
    }
    const auto it = world_seeds_.find(dimension);
    out_seed = it == world_seeds_.end() ? 0 : it->second;
    return ok();
}

status stub_minecraft_abi::get_player_absorption(
    player_handle player,
    float& out_absorption) {
    if (!player.valid()) {
        return err(ec::invalid_handle, "null player handle");
    }
    if (player_absorption_get_hook_) {
        if (player_absorption_get_hook_(player.raw(), &out_absorption) != 0) {
            return err(ec::not_supported, "get_player_absorption hook failed");
        }
        return ok();
    }
    if (players_.find(player.raw()) == players_.end()) {
        return err(ec::not_found, "unknown player handle");
    }
    const auto it = absorption_.find(player.raw());
    out_absorption = it == absorption_.end() ? 0.0f : it->second;
    return ok();
}

status stub_minecraft_abi::set_player_absorption(
    player_handle player,
    float absorption) {
    if (!player.valid()) {
        return err(ec::invalid_handle, "null player handle");
    }
    if (absorption < 0.0f) {
        return err(ec::invalid_argument, "absorption must be >= 0");
    }
    if (player_absorption_set_hook_) {
        if (player_absorption_set_hook_(player.raw(), absorption) != 0) {
            return err(ec::not_supported, "set_player_absorption hook failed");
        }
        return ok();
    }
    if (players_.find(player.raw()) == players_.end()) {
        return err(ec::not_found, "unknown player handle");
    }
    absorption_[player.raw()] = absorption;
    return ok();
}

status stub_minecraft_abi::get_player_invulnerable(
    player_handle player,
    std::int32_t& out_invulnerable) {
    if (!player.valid()) {
        return err(ec::invalid_handle, "null player handle");
    }
    if (player_invulnerable_get_hook_) {
        if (player_invulnerable_get_hook_(player.raw(), &out_invulnerable) != 0) {
            return err(ec::not_supported, "get_player_invulnerable hook failed");
        }
        return ok();
    }
    if (players_.find(player.raw()) == players_.end()) {
        return err(ec::not_found, "unknown player handle");
    }
    const auto it = invulnerable_.find(player.raw());
    out_invulnerable = it == invulnerable_.end() ? 0 : it->second;
    return ok();
}

status stub_minecraft_abi::set_player_invulnerable(
    player_handle player,
    std::int32_t invulnerable) {
    if (!player.valid()) {
        return err(ec::invalid_handle, "null player handle");
    }
    if (player_invulnerable_set_hook_) {
        if (player_invulnerable_set_hook_(player.raw(), invulnerable) != 0) {
            return err(ec::not_supported, "set_player_invulnerable hook failed");
        }
        return ok();
    }
    if (players_.find(player.raw()) == players_.end()) {
        return err(ec::not_found, "unknown player handle");
    }
    invulnerable_[player.raw()] = invulnerable != 0 ? 1 : 0;
    return ok();
}

status stub_minecraft_abi::get_player_air(
    player_handle player,
    std::int32_t& out_air) {
    if (!player.valid()) {
        return err(ec::invalid_handle, "null player handle");
    }
    if (player_air_get_hook_) {
        if (player_air_get_hook_(player.raw(), &out_air) != 0) {
            return err(ec::not_supported, "get_player_air hook failed");
        }
        return ok();
    }
    if (players_.find(player.raw()) == players_.end()) {
        return err(ec::not_found, "unknown player handle");
    }
    const auto it = air_.find(player.raw());
    out_air = it == air_.end() ? 300 : it->second;
    return ok();
}

status stub_minecraft_abi::set_player_air(
    player_handle player,
    std::int32_t air) {
    if (!player.valid()) {
        return err(ec::invalid_handle, "null player handle");
    }
    if (player_air_set_hook_) {
        if (player_air_set_hook_(player.raw(), air) != 0) {
            return err(ec::not_supported, "set_player_air hook failed");
        }
        return ok();
    }
    if (players_.find(player.raw()) == players_.end()) {
        return err(ec::not_found, "unknown player handle");
    }
    air_[player.raw()] = air;
    return ok();
}

status stub_minecraft_abi::get_player_fire_ticks(
    player_handle player,
    std::int32_t& out_ticks) {
    if (!player.valid()) {
        return err(ec::invalid_handle, "null player handle");
    }
    if (player_fire_ticks_get_hook_) {
        if (player_fire_ticks_get_hook_(player.raw(), &out_ticks) != 0) {
            return err(ec::not_supported, "get_player_fire_ticks hook failed");
        }
        return ok();
    }
    if (players_.find(player.raw()) == players_.end()) {
        return err(ec::not_found, "unknown player handle");
    }
    const auto it = fire_ticks_.find(player.raw());
    out_ticks = it == fire_ticks_.end() ? 0 : it->second;
    return ok();
}

status stub_minecraft_abi::set_player_fire_ticks(
    player_handle player,
    std::int32_t ticks) {
    if (!player.valid()) {
        return err(ec::invalid_handle, "null player handle");
    }
    if (player_fire_ticks_set_hook_) {
        if (player_fire_ticks_set_hook_(player.raw(), ticks) != 0) {
            return err(ec::not_supported, "set_player_fire_ticks hook failed");
        }
        return ok();
    }
    if (players_.find(player.raw()) == players_.end()) {
        return err(ec::not_found, "unknown player handle");
    }
    fire_ticks_[player.raw()] = ticks;
    return ok();
}

status stub_minecraft_abi::get_player_frozen_ticks(
    player_handle player,
    std::int32_t& out_ticks) {
    if (!player.valid()) {
        return err(ec::invalid_handle, "null player handle");
    }
    if (player_frozen_ticks_get_hook_) {
        if (player_frozen_ticks_get_hook_(player.raw(), &out_ticks) != 0) {
            return err(ec::not_supported, "get_player_frozen_ticks hook failed");
        }
        return ok();
    }
    if (players_.find(player.raw()) == players_.end()) {
        return err(ec::not_found, "unknown player handle");
    }
    const auto it = frozen_ticks_.find(player.raw());
    out_ticks = it == frozen_ticks_.end() ? 0 : it->second;
    return ok();
}

status stub_minecraft_abi::set_player_frozen_ticks(
    player_handle player,
    std::int32_t ticks) {
    if (!player.valid()) {
        return err(ec::invalid_handle, "null player handle");
    }
    if (player_frozen_ticks_set_hook_) {
        if (player_frozen_ticks_set_hook_(player.raw(), ticks) != 0) {
            return err(ec::not_supported, "set_player_frozen_ticks hook failed");
        }
        return ok();
    }
    if (players_.find(player.raw()) == players_.end()) {
        return err(ec::not_found, "unknown player handle");
    }
    frozen_ticks_[player.raw()] = ticks;
    return ok();
}

status stub_minecraft_abi::get_player_no_gravity(
    player_handle player,
    std::int32_t& out_no_gravity) {
    if (!player.valid()) {
        return err(ec::invalid_handle, "null player handle");
    }
    if (player_no_gravity_get_hook_) {
        if (player_no_gravity_get_hook_(player.raw(), &out_no_gravity) != 0) {
            return err(ec::not_supported, "get_player_no_gravity hook failed");
        }
        return ok();
    }
    if (players_.find(player.raw()) == players_.end()) {
        return err(ec::not_found, "unknown player handle");
    }
    const auto it = no_gravity_.find(player.raw());
    out_no_gravity = it == no_gravity_.end() ? 0 : it->second;
    return ok();
}

status stub_minecraft_abi::set_player_no_gravity(
    player_handle player,
    std::int32_t no_gravity) {
    if (!player.valid()) {
        return err(ec::invalid_handle, "null player handle");
    }
    if (player_no_gravity_set_hook_) {
        if (player_no_gravity_set_hook_(player.raw(), no_gravity) != 0) {
            return err(ec::not_supported, "set_player_no_gravity hook failed");
        }
        return ok();
    }
    if (players_.find(player.raw()) == players_.end()) {
        return err(ec::not_found, "unknown player handle");
    }
    no_gravity_[player.raw()] = no_gravity != 0 ? 1 : 0;
    return ok();
}

status stub_minecraft_abi::get_player_silent(
    player_handle player,
    std::int32_t& out_silent) {
    if (!player.valid()) {
        return err(ec::invalid_handle, "null player handle");
    }
    if (player_silent_get_hook_) {
        if (player_silent_get_hook_(player.raw(), &out_silent) != 0) {
            return err(ec::not_supported, "get_player_silent hook failed");
        }
        return ok();
    }
    if (players_.find(player.raw()) == players_.end()) {
        return err(ec::not_found, "unknown player handle");
    }
    const auto it = silent_.find(player.raw());
    out_silent = it == silent_.end() ? 0 : it->second;
    return ok();
}

status stub_minecraft_abi::set_player_silent(
    player_handle player,
    std::int32_t silent) {
    if (!player.valid()) {
        return err(ec::invalid_handle, "null player handle");
    }
    if (player_silent_set_hook_) {
        if (player_silent_set_hook_(player.raw(), silent) != 0) {
            return err(ec::not_supported, "set_player_silent hook failed");
        }
        return ok();
    }
    if (players_.find(player.raw()) == players_.end()) {
        return err(ec::not_found, "unknown player handle");
    }
    silent_[player.raw()] = silent != 0 ? 1 : 0;
    return ok();
}

status stub_minecraft_abi::get_player_glowing(
    player_handle player,
    std::int32_t& out_glowing) {
    if (!player.valid()) {
        return err(ec::invalid_handle, "null player handle");
    }
    if (player_glowing_get_hook_) {
        if (player_glowing_get_hook_(player.raw(), &out_glowing) != 0) {
            return err(ec::not_supported, "get_player_glowing hook failed");
        }
        return ok();
    }
    if (players_.find(player.raw()) == players_.end()) {
        return err(ec::not_found, "unknown player handle");
    }
    const auto it = glowing_.find(player.raw());
    out_glowing = it == glowing_.end() ? 0 : it->second;
    return ok();
}

status stub_minecraft_abi::set_player_glowing(
    player_handle player,
    std::int32_t glowing) {
    if (!player.valid()) {
        return err(ec::invalid_handle, "null player handle");
    }
    if (player_glowing_set_hook_) {
        if (player_glowing_set_hook_(player.raw(), glowing) != 0) {
            return err(ec::not_supported, "set_player_glowing hook failed");
        }
        return ok();
    }
    if (players_.find(player.raw()) == players_.end()) {
        return err(ec::not_found, "unknown player handle");
    }
    glowing_[player.raw()] = glowing != 0 ? 1 : 0;
    return ok();
}

status stub_minecraft_abi::get_player_invisible(
    player_handle player,
    std::int32_t& out_invisible) {
    if (!player.valid()) {
        return err(ec::invalid_handle, "null player handle");
    }
    if (player_invisible_get_hook_) {
        if (player_invisible_get_hook_(player.raw(), &out_invisible) != 0) {
            return err(ec::not_supported, "get_player_invisible hook failed");
        }
        return ok();
    }
    if (players_.find(player.raw()) == players_.end()) {
        return err(ec::not_found, "unknown player handle");
    }
    const auto it = invisible_.find(player.raw());
    out_invisible = it == invisible_.end() ? 0 : it->second;
    return ok();
}

status stub_minecraft_abi::set_player_invisible(
    player_handle player,
    std::int32_t invisible) {
    if (!player.valid()) {
        return err(ec::invalid_handle, "null player handle");
    }
    if (player_invisible_set_hook_) {
        if (player_invisible_set_hook_(player.raw(), invisible) != 0) {
            return err(ec::not_supported, "set_player_invisible hook failed");
        }
        return ok();
    }
    if (players_.find(player.raw()) == players_.end()) {
        return err(ec::not_found, "unknown player handle");
    }
    invisible_[player.raw()] = invisible != 0 ? 1 : 0;
    return ok();
}

status stub_minecraft_abi::get_player_portal_cooldown(
    player_handle player,
    std::int32_t& out_ticks) {
    if (!player.valid()) {
        return err(ec::invalid_handle, "null player handle");
    }
    if (player_portal_cooldown_get_hook_) {
        if (player_portal_cooldown_get_hook_(player.raw(), &out_ticks) != 0) {
            return err(ec::not_supported, "get_player_portal_cooldown hook failed");
        }
        return ok();
    }
    if (players_.find(player.raw()) == players_.end()) {
        return err(ec::not_found, "unknown player handle");
    }
    const auto it = portal_cooldown_.find(player.raw());
    out_ticks = it == portal_cooldown_.end() ? 0 : it->second;
    return ok();
}

status stub_minecraft_abi::set_player_portal_cooldown(
    player_handle player,
    std::int32_t ticks) {
    if (!player.valid()) {
        return err(ec::invalid_handle, "null player handle");
    }
    if (player_portal_cooldown_set_hook_) {
        if (player_portal_cooldown_set_hook_(player.raw(), ticks) != 0) {
            return err(ec::not_supported, "set_player_portal_cooldown hook failed");
        }
        return ok();
    }
    if (players_.find(player.raw()) == players_.end()) {
        return err(ec::not_found, "unknown player handle");
    }
    portal_cooldown_[player.raw()] = ticks;
    return ok();
}

status stub_minecraft_abi::get_player_max_air(
    player_handle player,
    std::int32_t& out_max_air) {
    if (!player.valid()) {
        return err(ec::invalid_handle, "null player handle");
    }
    if (get_player_max_air_hook_) {
        if (get_player_max_air_hook_(player.raw(), &out_max_air) != 0) {
            return err(ec::not_supported, "get_player_max_air hook failed");
        }
        return ok();
    }
    if (players_.find(player.raw()) == players_.end()) {
        return err(ec::not_found, "unknown player handle");
    }
    const auto it = max_air_.find(player.raw());
    out_max_air = it == max_air_.end() ? 300 : it->second;
    return ok();
}

void stub_minecraft_abi::register_player(player_handle player, std::string name) {
    players_[player.raw()] = std::move(name);
}

void stub_minecraft_abi::unregister_player(player_handle player) {
    players_.erase(player.raw());
    inventories_.erase(player.raw());
    positions_.erase(player.raw());
    health_.erase(player.raw());
    absorption_.erase(player.raw());
    invulnerable_.erase(player.raw());
    air_.erase(player.raw());
    fire_ticks_.erase(player.raw());
    frozen_ticks_.erase(player.raw());
    no_gravity_.erase(player.raw());
    silent_.erase(player.raw());
    glowing_.erase(player.raw());
    invisible_.erase(player.raw());
    portal_cooldown_.erase(player.raw());
    max_air_.erase(player.raw());
    food_.erase(player.raw());
    gamemodes_.erase(player.raw());
    xp_.erase(player.raw());
    looks_.erase(player.raw());
    velocities_.erase(player.raw());
    flags_.erase(player.raw());
    effects_.erase(player.raw());
    latencies_.erase(player.raw());
    ops_.erase(player.raw());
    uuids_.erase(player.raw());
    permissions_.erase(player.raw());
    for (auto it = inv_registry_ids_.begin(); it != inv_registry_ids_.end();) {
        if ((it->first >> 8) == player.raw()) {
            it = inv_registry_ids_.erase(it);
        } else {
            ++it;
        }
    }
    selected_slots_.erase(player.raw());
}

void stub_minecraft_abi::set_block_for_test(
    std::int32_t x,
    std::int32_t y,
    std::int32_t z,
    std::uint32_t block_id) {
    const auto key = pack_block(x, y, z);
    if (block_id == 0) {
        blocks_.erase(key);
    } else {
        blocks_[key] = block_id;
    }
}

namespace {

[[nodiscard]] capability_set default_caps_for(version mc) {
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

} // namespace

result<std::unique_ptr<minecraft_abi>> create_minecraft_abi(
    version minecraft,
    loader_kind loader) {
    // Phase-1: always stub. Real 1_20_1 / 1_21_1 backends replace this later.
    auto stub = std::make_unique<stub_minecraft_abi>(
        minecraft,
        loader,
        default_caps_for(minecraft));
    return std::unique_ptr<minecraft_abi>{std::move(stub)};
}

} // namespace leaf
