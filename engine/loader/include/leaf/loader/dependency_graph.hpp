#pragma once

#include <string>
#include <vector>

#include "leaf/core/result.hpp"
#include "leaf/core/version.hpp"
#include "leaf/loader/manifest.hpp"

namespace leaf {

struct resolve_options {
    /// Concrete Minecraft version of the running game (e.g. 1.21.1).
    version minecraft{};
    /// Host target triple, e.g. "linux-x86_64". Empty skips target filter.
    std::string host_target;
};

struct resolve_report {
    /// Mods in dependency load order (dependencies before dependents).
    std::vector<discovered_mod> load_order;
};

/// Validate manifests, Minecraft range, targets, required deps, duplicates,
/// then produce a topological load order.
[[nodiscard]] result<resolve_report> resolve_mods(
    std::vector<discovered_mod> mods,
    const resolve_options& options);

} // namespace leaf
