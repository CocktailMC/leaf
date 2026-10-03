#include "leaf/abi/leaf_abi_v1.h"
#include "leaf/core/abi_status.hpp"
#include "test_harness.hpp"

#include <cstdint>
#include <type_traits>

void test_abi_header() {
    LEAF_CHECK(LEAF_ABI_VERSION_MAJOR == 1u);
    LEAF_CHECK(LEAF_ABI_VERSION_MINOR == 58u);
    LEAF_CHECK(sizeof(LeafHandle) == sizeof(std::uint64_t));
    LEAF_CHECK(std::is_standard_layout_v<LeafApiV1>);
    LEAF_CHECK(std::is_standard_layout_v<LeafPlayerJoinEventV1>);
    LEAF_CHECK(std::is_standard_layout_v<LeafModExportsV1>);

    LEAF_CHECK(leaf::to_abi_status(leaf::ec::ok) == LEAF_STATUS_OK);
    LEAF_CHECK(
        leaf::to_abi_status(leaf::ec::stale_handle)
        == LEAF_STATUS_STALE_HANDLE);
    LEAF_CHECK(
        leaf::to_abi_status(leaf::ec::wrong_thread)
        == LEAF_STATUS_WRONG_THREAD);
}
