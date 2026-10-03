#include "leaf/loader/discovery.hpp"
#include "leaf/loader/host_target.hpp"
#include "leaf/loader/mod_engine.hpp"
#include "test_harness.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

#ifndef LEAF_TEST_GREETER_LEAFMODS_DIR
#  error "LEAF_TEST_GREETER_LEAFMODS_DIR must be defined"
#endif

void test_zip_leafmod_discovery_and_load() {
    const std::filesystem::path greeter_dir =
        std::filesystem::path{LEAF_TEST_GREETER_LEAFMODS_DIR} / "greeter.leafmod";
    LEAF_CHECK(std::filesystem::is_directory(greeter_dir));

    const auto tmp = std::filesystem::temp_directory_path() / "leafmc_zip_leafmods";
    std::filesystem::remove_all(tmp);
    std::filesystem::create_directories(tmp);

    const auto zip_path = tmp / "greeter.leafmod";
    const std::string cmd = "cd " + greeter_dir.string()
        + " && zip -qr " + zip_path.string() + " leaf.mod.json native";
    LEAF_CHECK(std::system(cmd.c_str()) == 0);
    LEAF_CHECK(std::filesystem::is_regular_file(zip_path));

    auto discovered = leaf::discover_mods(tmp);
    LEAF_CHECK(discovered.has_value());
    LEAF_CHECK(discovered->size() == 1);
    LEAF_CHECK(discovered->front().manifest.id == "greeter");
    LEAF_CHECK(std::filesystem::is_directory(
        tmp / ".leafmc-extract" / "greeter"));

    leaf::capability_set caps;
    caps.enable(leaf::capability::registry_modern);
    leaf::mod_engine engine{caps};
    leaf::engine_bootstrap_options opt{
        .leafmods_dir = tmp,
        .resolve =
            {
                .minecraft = leaf::version{1, 21, 1},
                .host_target = leaf::current_host_target(),
            },
        .capabilities = caps,
    };
    LEAF_CHECK(engine.bootstrap(opt).has_value());
    LEAF_CHECK(engine.enable_all().has_value());
    LEAF_CHECK(engine.find("greeter") != nullptr);
    LEAF_CHECK(engine.disable_all().has_value());
    LEAF_CHECK(engine.unload_all().has_value());
}

void test_leaf_alias_package_discovery() {
    const std::filesystem::path greeter_dir =
        std::filesystem::path{LEAF_TEST_GREETER_LEAFMODS_DIR} / "greeter.leafmod";
    LEAF_CHECK(std::filesystem::is_directory(greeter_dir));

    const auto tmp = std::filesystem::temp_directory_path() / "leafmc_leaf_alias";
    std::filesystem::remove_all(tmp);
    std::filesystem::create_directories(tmp);

    const auto zip_path = tmp / "greeter.leaf";
    const std::string cmd = "cd " + greeter_dir.string()
        + " && zip -qr " + zip_path.string() + " leaf.mod.json native";
    LEAF_CHECK(std::system(cmd.c_str()) == 0);
    LEAF_CHECK(std::filesystem::is_regular_file(zip_path));

    auto discovered = leaf::discover_mods(tmp);
    LEAF_CHECK(discovered.has_value());
    LEAF_CHECK(discovered->size() == 1);
    LEAF_CHECK(discovered->front().manifest.id == "greeter");
    LEAF_CHECK(std::filesystem::is_directory(
        tmp / ".leafmc-extract" / "greeter"));
}
