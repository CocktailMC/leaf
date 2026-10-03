#include "leaf/object/handle_table.hpp"
#include "test_harness.hpp"

#include <string>

namespace {

struct player_stub {
    std::string name;
};

using player_table = leaf::handle_table<leaf::player_tag, player_stub>;

} // namespace

void test_handle_table() {
    player_table table;

    auto h1 = table.create(player_stub{"Alice"});
    LEAF_CHECK(h1.has_value());
    LEAF_CHECK(h1->valid());
    LEAF_CHECK(table.size() == 1);

    {
        auto got = table.get(*h1);
        LEAF_CHECK(got.has_value());
        LEAF_CHECK(got->get().name == "Alice");
    }

    auto h2 = table.create(player_stub{"Bob"});
    LEAF_CHECK(h2.has_value());
    LEAF_CHECK(h1->raw() != h2->raw());
    LEAF_CHECK(table.size() == 2);

    LEAF_CHECK(table.destroy(*h1).has_value());
    LEAF_CHECK(table.size() == 1);

    {
        auto stale = table.get(*h1);
        LEAF_CHECK(!stale.has_value());
        LEAF_CHECK(stale.error().code() == leaf::ec::stale_handle);
    }

    auto h3 = table.create(player_stub{"Carol"});
    LEAF_CHECK(h3.has_value());
    LEAF_CHECK(leaf::handle_index(h3->raw()) == leaf::handle_index(h1->raw()));
    LEAF_CHECK(
        leaf::handle_generation(h3->raw())
        == leaf::handle_generation(h1->raw()) + 1);
    LEAF_CHECK(h3->raw() != h1->raw());

    {
        auto got = table.get(*h3);
        LEAF_CHECK(got.has_value());
        LEAF_CHECK(got->get().name == "Carol");
    }

    {
        auto bad = table.get(leaf::player_handle{});
        LEAF_CHECK(!bad.has_value());
        LEAF_CHECK(bad.error().code() == leaf::ec::invalid_handle);
    }

    LEAF_CHECK(table.contains(*h2));
    LEAF_CHECK(!table.contains(*h1));
}
