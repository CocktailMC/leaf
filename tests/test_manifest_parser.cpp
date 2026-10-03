#include "leaf/loader/manifest_parser.hpp"
#include "test_harness.hpp"

void test_manifest_parser() {
    constexpr std::string_view json = R"({
      "id": "example",
      "name": "Example Mod",
      "version": "1.0.0",
      "leaf": { "api": "1" },
      "runtime": "native",
      "entry": "leaf_mod_entry",
      "minecraft": { "min": "1.20.1", "max": "1.21.x" },
      "dependencies": {
        "required": [ { "id": "economy", "version": ">=1.0.0" } ],
        "optional": [ "world" ]
      },
      "targets": ["linux-x86_64", "windows-x86_64"]
    })";

    const auto m = leaf::parse_mod_manifest(json);
    LEAF_CHECK(m.has_value());
    LEAF_CHECK(m->id == "example");
    LEAF_CHECK(m->name == "Example Mod");
    const leaf::version expected{1, 0, 0};
    LEAF_CHECK(m->mod_version == expected);
    LEAF_CHECK(m->leaf_api == 1);
    LEAF_CHECK(m->entry == "leaf_mod_entry");
    LEAF_CHECK(m->dependencies.size() == 2);
    LEAF_CHECK(m->dependencies[0].id == "economy");
    LEAF_CHECK(!m->dependencies[0].optional);
    LEAF_CHECK(m->dependencies[1].id == "world");
    LEAF_CHECK(m->dependencies[1].optional);
    LEAF_CHECK(m->targets.size() == 2);

    const auto bad = leaf::parse_mod_manifest(R"({"id":"BadId","name":"x","version":"1.0.0"})");
    LEAF_CHECK(!bad.has_value());
    LEAF_CHECK(bad.error().code() == leaf::ec::mod_manifest_invalid);
}
