#pragma once

#include <filesystem>
#include <string_view>

#include "leaf/core/result.hpp"
#include "leaf/loader/manifest.hpp"

namespace leaf {

/// Resolve `root/native/<host_target>/<libfilename>` for a discovered mod.
[[nodiscard]] result<std::filesystem::path> resolve_native_library_path(
    const discovered_mod& mod,
    std::string_view host_target);

} // namespace leaf
