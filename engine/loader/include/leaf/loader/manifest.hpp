#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "leaf/core/version.hpp"
#include "leaf/loader/mod_state.hpp"
#include "leaf/loader/version_range.hpp"

namespace leaf {

enum class mod_runtime : std::uint8_t {
    native = 0,
};

struct mod_dependency {
    std::string id;
    version_requirement version{};
    bool optional{false};
};

struct mod_manifest {
    std::string id;
    std::string name;
    version mod_version{};

    std::uint32_t leaf_api{1};
    mod_runtime runtime{mod_runtime::native};
    std::string entry{"leaf_mod_entry"};

    minecraft_version_range minecraft{};
    std::vector<mod_dependency> dependencies;
    std::vector<std::string> targets;
};

/// A package found on disk before native libraries are loaded.
struct discovered_mod {
    mod_manifest manifest;
    std::filesystem::path root; // directory of the .leafmod package
    mod_state state{mod_state::discovered};
};

} // namespace leaf
