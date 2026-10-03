#pragma once

#include <filesystem>
#include <string_view>

#include "leaf/core/result.hpp"
#include "leaf/loader/manifest.hpp"

namespace leaf {

/// Parse a `leaf.mod.json` document into a typed manifest.
[[nodiscard]] result<mod_manifest> parse_mod_manifest(std::string_view json_text);

/// Read and parse `leaf.mod.json` from a filesystem path.
[[nodiscard]] result<mod_manifest> load_mod_manifest(const std::filesystem::path& path);

} // namespace leaf
