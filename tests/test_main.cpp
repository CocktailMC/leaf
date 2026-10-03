#include "test_harness.hpp"

#include <iostream>

void test_version();
void test_capability();
void test_handle_table();
void test_abi_header();
void test_version_range();
void test_manifest_parser();
void test_discovery_and_resolve();
void test_native_lifecycle();
void test_events_bundle();
void test_error_hex_codec();
void test_scheduler_bundle();
void test_bridge_bundle();
void test_minecraft_abi_bundle();
void test_sdk_bundle();
void test_zip_leafmod_discovery_and_load();
void test_leaf_alias_package_discovery();

int main() {
    test_version();
    test_capability();
    test_handle_table();
    test_abi_header();
    test_version_range();
    test_manifest_parser();
    test_discovery_and_resolve();
    test_native_lifecycle();
    test_events_bundle();
    test_error_hex_codec();
    test_scheduler_bundle();
    test_bridge_bundle();
    test_minecraft_abi_bundle();
    test_sdk_bundle();
    test_zip_leafmod_discovery_and_load();
    test_leaf_alias_package_discovery();

    const auto& s = leaf::test::global_stats();
    std::cout << "leaf_tests: " << s.passed << " passed, " << s.failed
              << " failed\n";
    return s.failed == 0 ? 0 : 1;
}
