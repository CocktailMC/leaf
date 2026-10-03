#include "leaf/loader/dependency_graph.hpp"
#include "leaf/loader/discovery.hpp"
#include "test_harness.hpp"

#include <filesystem>
#include <string>

#ifndef LEAF_TEST_FIXTURES_DIR
#  error "LEAF_TEST_FIXTURES_DIR must be defined"
#endif

namespace {

std::filesystem::path fixtures() {
    return std::filesystem::path{LEAF_TEST_FIXTURES_DIR};
}

} // namespace

void test_discovery_and_resolve() {
    {
        auto mods = leaf::discover_mods(fixtures() / "leafmods");
        LEAF_CHECK(mods.has_value());
        LEAF_CHECK(mods->size() == 3);

        leaf::resolve_options opt{
            .minecraft = leaf::version{1, 21, 1},
            .host_target = "linux-x86_64",
        };
        auto report = leaf::resolve_mods(std::move(*mods), opt);
        LEAF_CHECK(report.has_value());
        LEAF_CHECK(report->load_order.size() == 3);

        // economy and world have no required deps; example requires economy.
        // example must appear after economy.
        std::size_t economy_pos = 99;
        std::size_t example_pos = 99;
        for (std::size_t i = 0; i < report->load_order.size(); ++i) {
            const auto& id = report->load_order[i].manifest.id;
            if (id == "economy") {
                economy_pos = i;
            }
            if (id == "example") {
                example_pos = i;
            }
            LEAF_CHECK(report->load_order[i].state == leaf::mod_state::resolved);
        }
        LEAF_CHECK(economy_pos < example_pos);
    }

    {
        auto mods = leaf::discover_mods(fixtures() / "leafmods_cycle");
        LEAF_CHECK(mods.has_value());
        leaf::resolve_options opt{
            .minecraft = leaf::version{1, 21, 1},
            .host_target = "linux-x86_64",
        };
        auto report = leaf::resolve_mods(std::move(*mods), opt);
        LEAF_CHECK(!report.has_value());
        LEAF_CHECK(report.error().code() == leaf::ec::mod_dependency_cycle);
    }

    {
        auto mods = leaf::discover_mods(fixtures() / "leafmods_dup");
        LEAF_CHECK(mods.has_value());
        leaf::resolve_options opt{
            .minecraft = leaf::version{1, 21, 1},
            .host_target = "linux-x86_64",
        };
        auto report = leaf::resolve_mods(std::move(*mods), opt);
        LEAF_CHECK(!report.has_value());
        LEAF_CHECK(report.error().code() == leaf::ec::mod_duplicate_id);
    }

    {
        auto mods = leaf::discover_mods(fixtures() / "leafmods");
        LEAF_CHECK(mods.has_value());
        leaf::resolve_options opt{
            .minecraft = leaf::version{1, 19, 2},
            .host_target = "linux-x86_64",
        };
        auto report = leaf::resolve_mods(std::move(*mods), opt);
        LEAF_CHECK(!report.has_value());
        LEAF_CHECK(
            report.error().code() == leaf::ec::mod_version_incompatible);
    }

    {
        // Bad packages are skipped with a warning; good ones still load.
        auto mods = leaf::discover_mods(fixtures() / "leafmods_partial");
        LEAF_CHECK(mods.has_value());
        LEAF_CHECK(mods->size() == 1);
        LEAF_CHECK((*mods)[0].manifest.id == "good");
    }
}
