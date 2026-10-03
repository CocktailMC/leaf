#pragma once

#include <filesystem>
#include <vector>

#include "leaf/core/result.hpp"
#include "leaf/loader/manifest.hpp"

namespace leaf {

/// Scan `leafmods_dir` for `*.leafmod` / `*.leaf` directory packages and zip
/// archives containing `leaf.mod.json`. Zip packages are extracted under
/// `leafmods_dir/.leafmc-extract/`.
[[nodiscard]] result<std::vector<discovered_mod>> discover_mods(
    const std::filesystem::path& leafmods_dir);

} // namespace leaf
