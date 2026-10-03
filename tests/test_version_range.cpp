#include "leaf/loader/version_range.hpp"
#include "test_harness.hpp"

void test_version_range() {
    {
        const auto v = leaf::parse_minecraft_version("1.21.x");
        LEAF_CHECK(v.has_value());
        LEAF_CHECK(v->patch.wildcard);
        LEAF_CHECK(v->matches(leaf::version{1, 21, 0}));
        LEAF_CHECK(v->matches(leaf::version{1, 21, 1}));
        LEAF_CHECK(!v->matches(leaf::version{1, 20, 1}));
    }

    {
        leaf::minecraft_version_range range;
        range.min = *leaf::parse_minecraft_version("1.20.1");
        range.max = *leaf::parse_minecraft_version("1.21.x");
        LEAF_CHECK(range.contains(leaf::version{1, 20, 1}));
        LEAF_CHECK(range.contains(leaf::version{1, 21, 1}));
        LEAF_CHECK(!range.contains(leaf::version{1, 19, 4}));
        LEAF_CHECK(!range.contains(leaf::version{1, 22, 0}));
    }

    {
        const auto any = leaf::parse_version_requirement("*");
        LEAF_CHECK(any.has_value());
        LEAF_CHECK(any->satisfied_by(leaf::version{9, 9, 9}));

        const auto at_least = leaf::parse_version_requirement(">=1.2.0");
        LEAF_CHECK(at_least.has_value());
        LEAF_CHECK(at_least->satisfied_by(leaf::version{1, 2, 0}));
        LEAF_CHECK(at_least->satisfied_by(leaf::version{1, 3, 0}));
        LEAF_CHECK(!at_least->satisfied_by(leaf::version{1, 1, 9}));

        const auto exact = leaf::parse_version_requirement("1.0.0");
        LEAF_CHECK(exact.has_value());
        LEAF_CHECK(exact->satisfied_by(leaf::version{1, 0, 0}));
        LEAF_CHECK(!exact->satisfied_by(leaf::version{1, 0, 1}));
    }
}
