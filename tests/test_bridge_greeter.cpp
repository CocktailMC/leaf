#include "leaf/abi/leaf_bridge_v1.h"
#include "leaf/abi/leaf_error_v1.h"
#include "leaf/bridge/bridge_host.hpp"
#include "leaf/minecraft/stub_minecraft_abi.hpp"
#include "test_harness.hpp"

#include <filesystem>
#include <string>

#ifndef LEAF_TEST_GREETER_LEAFMODS_DIR
#  error "LEAF_TEST_GREETER_LEAFMODS_DIR must be defined"
#endif

namespace {

void expect_greeter_loaded() {
    LEAF_CHECK(leaf_bridge_is_initialized() == 1);
    LEAF_CHECK(leaf_bridge_loaded_mod_count() == 1);

    char id[64]{};
    LEAF_CHECK(leaf_bridge_mod_id_at(0, id, sizeof(id)) == LEAF_OK);
    LEAF_CHECK(std::string{id} == "greeter");
}

void run_loader(std::uint32_t loader, std::uint32_t mapping) {
    (void)leaf_bridge_shutdown();

    const std::filesystem::path leafmods{LEAF_TEST_GREETER_LEAFMODS_DIR};
    LEAF_CHECK(std::filesystem::is_directory(leafmods / "greeter.leafmod"));

    const auto leafmods_str = leafmods.string();
    LeafBridgeConfigV1 cfg{};
    cfg.struct_size = static_cast<uint32_t>(sizeof(LeafBridgeConfigV1));
    cfg.minecraft_version = "1.21.1";
    cfg.leafmods_dir = leafmods_str.c_str();
    cfg.game_dir = nullptr;
    cfg.loader = loader;
    cfg.mapping = mapping;
    cfg.auto_load_mods = 1;

    LEAF_CHECK(leaf_bridge_init(&cfg) == LEAF_OK);
    expect_greeter_loaded();

    LEAF_CHECK(leaf_bridge_pump_main(32) == LEAF_OK);
    LEAF_CHECK(leaf_bridge_on_server_starting() == LEAF_OK);
    LEAF_CHECK(leaf_bridge_on_server_started() == LEAF_OK);
    LEAF_CHECK(leaf_bridge_emit_player_join(4242) == LEAF_OK);

#if defined(__linux__)
    {
        auto* stub = leaf::bridge_host::instance().stub_minecraft();
        LEAF_CHECK(stub != nullptr);
        LEAF_CHECK(stub->message_log_size() >= 1);
        bool saw_welcome = false;
        bool saw_broadcast = false;
        for (std::size_t i = 0; i < stub->message_log_size(); ++i) {
            const auto& line = stub->message_at(i);
            if (line.find("Welcome to LEAFMC!") != std::string::npos) {
                saw_welcome = true;
            }
            if (line.find("*broadcast*") != std::string::npos) {
                saw_broadcast = true;
            }
        }
        LEAF_CHECK(saw_welcome);
        LEAF_CHECK(saw_broadcast);
    }
#endif

    LEAF_CHECK(leaf_bridge_on_server_stopping() == LEAF_OK);
    LEAF_CHECK(leaf_bridge_shutdown() == LEAF_OK);
}

} // namespace

void test_bridge_loads_greeter_all_loaders() {
    run_loader(LEAF_LOADER_FABRIC, LEAF_MAPPING_INTERMEDIARY);
    run_loader(LEAF_LOADER_FORGE, LEAF_MAPPING_SRG);
    run_loader(LEAF_LOADER_NEOFORGE, LEAF_MAPPING_MOJMAP);
}

void test_bridge_creates_missing_leafmods_dir() {
    (void)leaf_bridge_shutdown();
    const auto dir = std::filesystem::temp_directory_path()
        / "leafmc_autocreate_leafmods";
    std::filesystem::remove_all(dir);
    LEAF_CHECK(!std::filesystem::exists(dir));

    const auto dir_str = dir.string();
    LeafBridgeConfigV1 cfg{};
    cfg.struct_size = static_cast<uint32_t>(sizeof(LeafBridgeConfigV1));
    cfg.minecraft_version = "1.21.1";
    cfg.leafmods_dir = dir_str.c_str();
    cfg.loader = LEAF_LOADER_FABRIC;
    cfg.mapping = LEAF_MAPPING_INTERMEDIARY;
    cfg.auto_load_mods = 1;

    LEAF_CHECK(leaf_bridge_init(&cfg) == LEAF_OK);
    LEAF_CHECK(std::filesystem::is_directory(dir));
    LEAF_CHECK(leaf_bridge_loaded_mod_count() == 0);
    LEAF_CHECK(leaf_bridge_shutdown() == LEAF_OK);
}

void test_bridge_reload_mods() {
    (void)leaf_bridge_shutdown();
    const std::filesystem::path leafmods{LEAF_TEST_GREETER_LEAFMODS_DIR};
    const auto leafmods_str = leafmods.string();

    LeafBridgeConfigV1 cfg{};
    cfg.struct_size = static_cast<uint32_t>(sizeof(LeafBridgeConfigV1));
    cfg.minecraft_version = "1.21.1";
    cfg.leafmods_dir = leafmods_str.c_str();
    cfg.loader = LEAF_LOADER_FABRIC;
    cfg.mapping = LEAF_MAPPING_INTERMEDIARY;
    cfg.auto_load_mods = 1;

    LEAF_CHECK(leaf_bridge_init(&cfg) == LEAF_OK);
    LEAF_CHECK(leaf_bridge_loaded_mod_count() == 1);
    LEAF_CHECK(leaf_bridge_reload_mods() == LEAF_OK);
    LEAF_CHECK(leaf_bridge_loaded_mod_count() == 1);
    char id[64]{};
    LEAF_CHECK(leaf_bridge_mod_id_at(0, id, sizeof(id)) == LEAF_OK);
    LEAF_CHECK(std::string{id} == "greeter");
    LEAF_CHECK(leaf_bridge_shutdown() == LEAF_OK);
}
