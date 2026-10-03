#include "leaf/abi/leaf_bridge_v1.h"
#include "leaf/abi/leaf_abi_v1.h"
#include "leaf/abi/leaf_error_v1.h"
#include "leaf/abi/leaf_event_v1.h"
#include "leaf/bridge/bridge_host.hpp"
#include "leaf/events/event_id.hpp"
#include "test_harness.hpp"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <string>
#include <thread>

void test_bridge_host_lifecycle() {
    // Ensure clean process state.
    (void)leaf_bridge_shutdown();

    const auto leafmods = std::filesystem::temp_directory_path() / "leafmc_bridge_leafmods";
    std::filesystem::create_directories(leafmods);

    LeafBridgeConfigV1 cfg{};
    cfg.struct_size = static_cast<uint32_t>(sizeof(LeafBridgeConfigV1));
    cfg.minecraft_version = "1.21.1";
    cfg.leafmods_dir = leafmods.c_str();
    cfg.game_dir = nullptr;
    cfg.loader = LEAF_LOADER_FABRIC;
    cfg.mapping = LEAF_MAPPING_INTERMEDIARY;
    cfg.auto_load_mods = 1; // empty dir is fine

    LEAF_CHECK(leaf_bridge_init(&cfg) == LEAF_OK);
    LEAF_CHECK(leaf_bridge_is_initialized() == 1);

    LeafBridgeInfoV1 info{};
    info.struct_size = static_cast<uint32_t>(sizeof(LeafBridgeInfoV1));
    LEAF_CHECK(leaf_bridge_get_info(&info) == LEAF_OK);
    LEAF_CHECK(info.loader == LEAF_LOADER_FABRIC);
    LEAF_CHECK(std::string{info.minecraft_version} == "1.21.1");

    std::atomic<int> joins{0};
    auto* engine = leaf::bridge_host::instance().engine();
    LEAF_CHECK(engine != nullptr);

    auto sub = engine->events().subscribe_notification(
        leaf::event_ids::player_join,
        [&](leaf::event_packet_view view) {
            ++joins;
            LEAF_CHECK(view.id() == leaf::event_ids::player_join);
        },
        leaf::event_priority::normal,
        42);
    LEAF_CHECK(sub.has_value());

    LEAF_CHECK(leaf_bridge_pump_main(64) == LEAF_OK);
    LEAF_CHECK(leaf_bridge_on_server_starting() == LEAF_OK);
    LEAF_CHECK(leaf_bridge_on_server_started() == LEAF_OK);

    std::atomic<int> ticks{0};
    auto tick_sub = engine->events().subscribe_notification(
        leaf::event_ids::server_tick,
        [&](leaf::event_packet_view /*view*/) { ++ticks; },
        leaf::event_priority::normal,
        42);
    LEAF_CHECK(tick_sub.has_value());
    LEAF_CHECK(leaf_bridge_pump_main(64) == LEAF_OK);
    LEAF_CHECK(ticks.load() == 1);

    LEAF_CHECK(leaf_bridge_emit_player_join(99) == LEAF_OK);
    LEAF_CHECK(joins.load() == 1);

    uint32_t decision = 255;
    LEAF_CHECK(leaf_bridge_emit_player_join_request(7, &decision) == LEAF_OK);
    LEAF_CHECK(decision == LEAF_DECISION_PASS);

    std::atomic<int> chats{0};
    auto chat_sub = engine->events().subscribe_decision(
        leaf::event_ids::player_chat,
        [&](leaf::event_packet_view view) {
            ++chats;
            LEAF_CHECK(view.id() == leaf::event_ids::player_chat);
            return leaf::decision::pass;
        },
        leaf::event_priority::normal,
        42);
    LEAF_CHECK(chat_sub.has_value());
    decision = 255;
    LEAF_CHECK(leaf_bridge_emit_player_chat(99, "hello leaf", &decision) == LEAF_OK);
    LEAF_CHECK(decision == LEAF_DECISION_PASS);
    LEAF_CHECK(chats.load() == 1);

    auto deny_sub = engine->events().subscribe_decision(
        leaf::event_ids::player_chat,
        [&](leaf::event_packet_view /*view*/) {
            return leaf::decision::deny;
        },
        leaf::event_priority::normal,
        43);
    LEAF_CHECK(deny_sub.has_value());
    decision = 255;
    LEAF_CHECK(leaf_bridge_emit_player_chat(99, "nope", &decision) == LEAF_OK);
    LEAF_CHECK(decision == LEAF_DECISION_DENY);

    decision = 255;
    LEAF_CHECK(leaf_bridge_emit_block_break(99, 1, 2, 3, 7, &decision) == LEAF_OK);
    LEAF_CHECK(decision == LEAF_DECISION_PASS);
    decision = 255;
    LEAF_CHECK(leaf_bridge_emit_block_place(99, 1, 2, 3, 8, &decision) == LEAF_OK);
    LEAF_CHECK(decision == LEAF_DECISION_PASS);

    LEAF_CHECK(leaf_bridge_emit_player_death(99) == LEAF_OK);

    LEAF_CHECK(leaf_bridge_emit_entity_spawn(55, 12, 1, 64, 2, 0) == LEAF_OK);
    LEAF_CHECK(leaf_bridge_emit_entity_remove(55, 12, 1, 64, 2, 0) == LEAF_OK);
    LEAF_CHECK(leaf_bridge_emit_world_load(0) == LEAF_OK);
    LEAF_CHECK(leaf_bridge_emit_world_load(1) == LEAF_OK);

    {
        static int delayed = 0;
        delayed = 0;
        LEAF_CHECK(engine != nullptr);
        const auto& api = engine->api().abi();
        LEAF_CHECK(api.post_main != nullptr);
        LEAF_CHECK(api.delay_main != nullptr);
        uint64_t task = 0;
        LEAF_CHECK(api.post_main(
                       [](void*) { delayed += 10; },
                       nullptr)
            == LEAF_STATUS_OK);
        LEAF_CHECK(api.delay_main(
                       [](void*) { delayed += 1; },
                       nullptr,
                       1,
                       &task)
            == LEAF_STATUS_OK);
        LEAF_CHECK(task != 0);
        LEAF_CHECK(leaf_bridge_pump_main(64) == LEAF_OK);
        LEAF_CHECK(delayed == 10); // delay_ms=1 may still be pending
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        LEAF_CHECK(leaf_bridge_pump_main(64) == LEAF_OK);
        LEAF_CHECK(delayed == 11);
    }

    LEAF_CHECK(leaf_bridge_on_server_stopping() == LEAF_OK);
    LEAF_CHECK(leaf_bridge_shutdown() == LEAF_OK);
    LEAF_CHECK(leaf_bridge_is_initialized() == 0);

    // Double init after shutdown should work.
    LEAF_CHECK(leaf_bridge_init(&cfg) == LEAF_OK);
    LEAF_CHECK(leaf_bridge_shutdown() == LEAF_OK);
}

void test_bridge_rejects_bad_config() {
    (void)leaf_bridge_shutdown();
    LeafBridgeConfigV1 cfg{};
    cfg.struct_size = static_cast<uint32_t>(sizeof(LeafBridgeConfigV1));
    cfg.minecraft_version = nullptr;
    cfg.leafmods_dir = "/tmp";
    LEAF_CHECK(leaf_bridge_init(&cfg) == LEAF_CORE_INVALID_ARGUMENT);
}

void test_bridge_loads_greeter_all_loaders();
void test_bridge_creates_missing_leafmods_dir();
void test_bridge_reload_mods();

void test_bridge_bundle() {
    test_bridge_host_lifecycle();
    test_bridge_rejects_bad_config();
    test_bridge_loads_greeter_all_loaders();
    test_bridge_creates_missing_leafmods_dir();
    test_bridge_reload_mods();
}
