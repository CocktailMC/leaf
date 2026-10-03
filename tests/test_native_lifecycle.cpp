#include "leaf/loader/host_target.hpp"
#include "leaf/loader/mod_engine.hpp"
#include "test_harness.hpp"

#include <filesystem>
#include <string>

#if defined(__linux__)
#  include <dlfcn.h>
#endif

#ifndef LEAF_TEST_RUNTIME_LEAFMODS_DIR
#  error "LEAF_TEST_RUNTIME_LEAFMODS_DIR must be defined"
#endif

void test_native_lifecycle() {
    const auto host = leaf::current_host_target();
    LEAF_CHECK(host != "unknown");

    const std::filesystem::path leafmods{LEAF_TEST_RUNTIME_LEAFMODS_DIR};
    LEAF_CHECK(std::filesystem::is_directory(leafmods / "hello.leafmod"));

    leaf::capability_set caps;
    caps.enable(leaf::capability::registry_modern);

    leaf::mod_engine engine{caps};
    std::string last_log;
    engine.api().set_log_sink([&](int, std::string_view msg) {
        last_log = std::string(msg);
    });

    leaf::engine_bootstrap_options opt{
        .leafmods_dir = leafmods,
        .resolve =
            {
                .minecraft = leaf::version{1, 21, 1},
                .host_target = host,
            },
        .capabilities = caps,
    };

    const auto boot = engine.bootstrap(opt);
    LEAF_CHECK(boot.has_value());
    LEAF_CHECK(engine.mods().size() == 1);

    auto* hello = engine.find("hello");
    LEAF_CHECK(hello != nullptr);
    LEAF_CHECK(hello->state() == leaf::mod_state::initialized);

    const auto lib_path = leafmods / "hello.leafmod" / "native" / host
        / leaf::native_library_filename("hello");

#if defined(__linux__)
    {
        using count_fn = int (*)();
        void* existing =
            dlopen(lib_path.c_str(), RTLD_NOW | RTLD_NOLOAD | RTLD_LOCAL);
        LEAF_CHECK(existing != nullptr);
        auto* load_c = reinterpret_cast<count_fn>(
            dlsym(existing, "leaf_test_hello_on_load_count"));
        auto* enable_c = reinterpret_cast<count_fn>(
            dlsym(existing, "leaf_test_hello_on_enable_count"));
        auto* disable_c = reinterpret_cast<count_fn>(
            dlsym(existing, "leaf_test_hello_on_disable_count"));
        auto* unload_c = reinterpret_cast<count_fn>(
            dlsym(existing, "leaf_test_hello_on_unload_count"));
        LEAF_CHECK(load_c != nullptr);
        LEAF_CHECK(load_c() == 1);
        LEAF_CHECK(enable_c() == 0);

        LEAF_CHECK(engine.enable_all().has_value());
        LEAF_CHECK(hello->state() == leaf::mod_state::enabled);
        LEAF_CHECK(enable_c() == 1);
        LEAF_CHECK(!last_log.empty());

        LEAF_CHECK(engine.disable_all().has_value());
        LEAF_CHECK(hello->state() == leaf::mod_state::disabled);
        LEAF_CHECK(disable_c() == 1);

        LEAF_CHECK(engine.unload_all().has_value());
        LEAF_CHECK(engine.mods().empty());
        LEAF_CHECK(unload_c() == 1);

        dlclose(existing);
    }
#else
    (void)lib_path;
    LEAF_CHECK(engine.enable_all().has_value());
    LEAF_CHECK(hello->state() == leaf::mod_state::enabled);
    LEAF_CHECK(engine.disable_all().has_value());
    LEAF_CHECK(hello->state() == leaf::mod_state::disabled);
    LEAF_CHECK(engine.unload_all().has_value());
    LEAF_CHECK(engine.mods().empty());
#endif
}
