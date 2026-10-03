#pragma once

#include <string>
#include <string_view>

#include "leaf/core/result.hpp"

namespace leaf {

/// Detect the running host target triple used under `native/<target>/`.
/// Examples: `linux-x86_64`, `windows-x86_64`, `linux-aarch64`, `macos-arm64`.
[[nodiscard]] std::string current_host_target();

/// Shared-library file name for a mod id on the current platform.
/// Linux: `lib{id}.so`  Windows: `{id}.dll`  macOS: `lib{id}.dylib`
[[nodiscard]] std::string native_library_filename(std::string_view mod_id);

} // namespace leaf
