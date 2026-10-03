#include "leaf/core/version.hpp"
#include "test_harness.hpp"

void test_version() {
    using leaf::parse_version;
    using leaf::version;

    {
        const auto v = parse_version("1.20.1");
        LEAF_CHECK(v.has_value());
        LEAF_CHECK(v->major == 1);
        LEAF_CHECK(v->minor == 20);
        LEAF_CHECK(v->patch == 1);
        LEAF_CHECK(v->to_string() == "1.20.1");
    }

    {
        const auto bad = parse_version("1.20");
        LEAF_CHECK(!bad.has_value());
        LEAF_CHECK(bad.error().code() == leaf::ec::invalid_argument);
    }

    {
        const auto bad = parse_version("1.20.x");
        LEAF_CHECK(!bad.has_value());
    }

    {
        constexpr version engine{1, 2, 3};
        constexpr version need{1, 2, 0};
        constexpr version newer_minor{1, 3, 0};
        constexpr version other_major{2, 0, 0};
        constexpr version older{1, 1, 9};
        LEAF_CHECK(engine.compatible_with(need));
        LEAF_CHECK(newer_minor.compatible_with(need));
        LEAF_CHECK(!engine.compatible_with(other_major));
        LEAF_CHECK(!older.compatible_with(need));
    }
}
