#include "leaf/sdk/sdk.hpp"

#include "leaf/abi/leaf_event_v1.h"

#include <atomic>
#include <string_view>

namespace {

std::atomic<int> g_joins{0};
const LeafApiV1* g_api{nullptr};

class greeter_mod final : public leaf::sdk::mod {
public:
    [[nodiscard]] leaf::sdk::mod_info info() const noexcept override {
        return {
            .id = "greeter",
            .name = "Greeter",
            .version = "0.1.0",
        };
    }

    void on_load(leaf::sdk::api& a) override {
        g_api = a.raw();
        a.info("greeter on_load");
    }

    void on_enable(leaf::sdk::api& a) override {
        g_api = a.raw();
        a.info("greeter on_enable");
        // Touch additive slots so live wiring stays covered as LeafApi grows.
        std::uint32_t difficulty = 0;
        (void)a.get_difficulty(difficulty);
        std::uint32_t weather = 0;
        (void)a.get_weather(0, weather);
        std::int32_t sx = 0, sy = 0, sz = 0;
        (void)a.get_world_spawn(0, sx, sy, sz);
        const auto st = a.subscribe_method<&greeter_mod::on_player_join>(
            LEAF_EVENT_PLAYER_JOIN,
            this,
            join_sub_);
        if (!leaf::sdk::ok(st)) {
            a.warn("greeter failed to subscribe player_join");
        }
        const auto chat_st = a.subscribe_decision_method<&greeter_mod::on_player_chat>(
            LEAF_EVENT_PLAYER_CHAT,
            this,
            chat_sub_);
        if (!leaf::sdk::ok(chat_st)) {
            a.warn("greeter failed to subscribe player_chat");
        }
        const auto death_st = a.subscribe_method<&greeter_mod::on_player_death>(
            LEAF_EVENT_PLAYER_DEATH,
            this,
            death_sub_);
        if (!leaf::sdk::ok(death_st)) {
            a.warn("greeter failed to subscribe player_death");
        }
    }

    void on_disable(leaf::sdk::api& a) override {
        join_sub_.reset();
        chat_sub_.reset();
        death_sub_.reset();
        a.info("greeter on_disable");
    }

    void on_unload(leaf::sdk::api& a) override {
        g_api = nullptr;
        a.info("greeter on_unload");
    }

private:
    void on_player_join(leaf::sdk::event_view view) {
        const auto ev = leaf::sdk::as_player_join(view);
        ++g_joins;
        if (!g_api) {
            return;
        }
        leaf::sdk::api a{g_api};
        char name[64]{};
        if (leaf::sdk::ok(a.get_player_name(ev.player, name, sizeof(name)))) {
            a.info(name);
            LeafHandle by_name = 0;
            (void)a.find_player_by_name(name, by_name);
        }
        // Keep message as a durable C string for the ABI boundary.
        static constexpr char kWelcome[] = "Welcome to LEAFMC!";
        const auto st = a.send_player_message(ev.player, kWelcome);
        if (!leaf::sdk::ok(st)) {
            a.warn("greeter welcome message failed");
        }
        if (a.player_count() > 0) {
            static constexpr char kBroadcast[] = "[LEAF] A player joined.";
            (void)a.broadcast_message(kBroadcast);
        }
        // Delayed follow-up on main thread (requires pump_main / server tick).
        static LeafHandle delayed_player = 0;
        delayed_player = ev.player;
        std::uint64_t task = 0;
        (void)a.delay_main(
            [](void*) {
                if (!g_api || delayed_player == 0) {
                    return;
                }
                leaf::sdk::api delayed{g_api};
                static constexpr char kLater[] = "[LEAF] Still glad you're here.";
                (void)delayed.send_player_message(delayed_player, kLater);
            },
            nullptr,
            50,
            task);
        static constexpr char kJoinSound[] = "minecraft:entity.player.levelup";
        (void)a.play_sound(ev.player, kJoinSound, 0.7f, 1.0f, 0, 0, 0, 0);
        static constexpr char kBar[] = "LEAFMC online";
        (void)a.send_actionbar(ev.player, kBar);
        static constexpr char kTitle[] = "LEAFMC";
        static constexpr char kSub[] = "Welcome";
        (void)a.send_title(ev.player, kTitle, kSub, 10, 40, 10);
        (void)a.broadcast_actionbar(kBar);
        std::int32_t latency = 0;
        (void)a.get_player_latency(ev.player, latency);
        char uuid[48]{};
        (void)a.get_player_uuid(ev.player, uuid, sizeof(uuid));
        if (uuid[0] != '\0') {
            a.info(uuid);
            LeafHandle roundtrip = 0;
            (void)a.find_player_by_uuid(uuid, roundtrip);
        }
        std::int32_t perm = 0;
        (void)a.get_player_permission_level(ev.player, perm);
        std::uint32_t hotbar = 0;
        if (leaf::sdk::ok(a.get_selected_slot(ev.player, hotbar))) {
            (void)a.set_selected_slot(ev.player, hotbar);
        }
        char held[64]{};
        std::uint32_t held_count = 0;
        std::int32_t held_damage = 0;
        (void)a.get_held_item_registry_id(
            ev.player, held, sizeof(held), held_count, held_damage);
        char offhand[64]{};
        std::uint32_t off_count = 0;
        std::int32_t off_damage = 0;
        (void)a.get_equipment_item_registry_id(
            ev.player,
            LEAF_EQUIP_OFFHAND,
            offhand,
            sizeof(offhand),
            off_count,
            off_damage);
        std::int64_t seed = 0;
        (void)a.get_world_seed(0, seed);
        (void)a.heal_player(ev.player);
        std::int32_t alive = 0;
        (void)a.is_player_alive(ev.player, alive);
        float absorption = 0.0f;
        (void)a.get_player_absorption(ev.player, absorption);
        std::int32_t invuln = 0;
        (void)a.get_player_invulnerable(ev.player, invuln);
        std::int32_t air = 0;
        (void)a.get_player_air(ev.player, air);
        std::int32_t fire = 0;
        (void)a.get_player_fire_ticks(ev.player, fire);
        std::int32_t frozen = 0;
        (void)a.get_player_frozen_ticks(ev.player, frozen);
        (void)a.extinguish_player(ev.player);
        (void)a.unfreeze_player(ev.player);
        std::int32_t no_grav = 0;
        (void)a.get_player_no_gravity(ev.player, no_grav);
        (void)a.set_player_no_gravity(ev.player, 0);
        std::int32_t silent = 0;
        (void)a.get_player_silent(ev.player, silent);
        (void)a.set_player_silent(ev.player, 0);
        std::int32_t glowing = 0;
        (void)a.get_player_glowing(ev.player, glowing);
        (void)a.set_player_glowing(ev.player, 0);
        std::int32_t invisible = 0;
        (void)a.get_player_invisible(ev.player, invisible);
        (void)a.set_player_invisible(ev.player, 0);
        std::int32_t portal_cd = 0;
        (void)a.get_player_portal_cooldown(ev.player, portal_cd);
        (void)a.set_player_portal_cooldown(ev.player, 0);
        std::int32_t max_air = 0;
        (void)a.get_player_max_air(ev.player, max_air);
        (void)a.refill_player_air(ev.player);
        std::int32_t px = 0, py = 0, pz = 0;
        std::uint32_t dim = 0;
        if (leaf::sdk::ok(a.get_player_pos(ev.player, px, py, pz, dim))) {
            char biome[64]{};
            (void)a.get_biome(dim, px, py, pz, biome, sizeof(biome));
            std::uint32_t block_light = 0, sky_light = 0;
            (void)a.get_light_level(dim, px, py, pz, block_light, sky_light);
            char block_id[64]{};
            (void)a.get_block_registry_id(dim, px, py - 1, pz, block_id, sizeof(block_id));
        }
    }

    int on_player_chat(leaf::sdk::event_view view) {
        const auto ev = leaf::sdk::as_player_chat(view);
        if (!g_api || !ev.message) {
            return LEAF_DECISION_PASS;
        }
        leaf::sdk::api a{g_api};
        a.info(ev.message);
        // Demo: messages containing "[blocked]" are denied by greeter.
        if (std::string_view{ev.message}.find("[blocked]") != std::string_view::npos) {
            return LEAF_DECISION_DENY;
        }
        return LEAF_DECISION_PASS;
    }

    void on_player_death(leaf::sdk::event_view view) {
        const auto ev = leaf::sdk::as_player_death(view);
        if (!g_api) {
            return;
        }
        leaf::sdk::api a{g_api};
        char name[64]{};
        if (leaf::sdk::ok(a.get_player_name(ev.player, name, sizeof(name)))) {
            a.info(name);
        }
        static constexpr char kDeath[] = "[LEAF] You died. Respawning is up to the game.";
        (void)a.send_player_message(ev.player, kDeath);
    }

    leaf::sdk::subscription join_sub_;
    leaf::sdk::subscription chat_sub_;
    leaf::sdk::subscription death_sub_;
};

} // namespace

extern "C" {

#if defined(_WIN32)
#  define GREETER_EXPORT __declspec(dllexport)
#else
#  define GREETER_EXPORT __attribute__((visibility("default")))
#endif

GREETER_EXPORT int leaf_test_greeter_join_count(void) {
    return g_joins.load();
}

} // extern "C"

LEAF_DEFINE_MOD(greeter_mod)
