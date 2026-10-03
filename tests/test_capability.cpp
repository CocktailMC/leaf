#include "leaf/core/capability.hpp"
#include "test_harness.hpp"

void test_capability() {
    leaf::capability_set caps;
    LEAF_CHECK(caps.empty());
    LEAF_CHECK(!caps.has(leaf::capability::data_components));

    caps.enable(leaf::capability::data_components);
    caps.enable(leaf::capability::registry_modern);
    LEAF_CHECK(caps.has(leaf::capability::data_components));
    LEAF_CHECK(caps.has(leaf::capability::registry_modern));
    LEAF_CHECK(!caps.has(leaf::capability::legacy_item_nbt));
    LEAF_CHECK(caps.count() == 2);

    caps.disable(leaf::capability::data_components);
    LEAF_CHECK(!caps.has(leaf::capability::data_components));
    LEAF_CHECK(caps.count() == 1);
}
