#pragma once

#include <filesystem>

#include "leaf/core/result.hpp"

namespace leaf {

/// If `package` is a zip `.leafmod` file, extract into
/// `leafmods_dir/.leafmc-extract/<stem>/` and return that directory.
/// Directory packages are returned unchanged.
[[nodiscard]] result<std::filesystem::path> materialize_leafmod_package(
    const std::filesystem::path& package,
    const std::filesystem::path& leafmods_dir);

} // namespace leaf
