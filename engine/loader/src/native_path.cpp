#include "leaf/loader/native_path.hpp"

#include "leaf/loader/host_target.hpp"

#include <system_error>

namespace leaf {

result<std::filesystem::path> resolve_native_library_path(
    const discovered_mod& mod,
    std::string_view host_target) {
    if (host_target.empty() || host_target == "unknown") {
        return err<std::filesystem::path>(
            ec::not_supported,
            "unsupported or unknown host target");
    }

    const auto filename = native_library_filename(mod.manifest.id);
    const auto path = mod.root / "native" / std::string(host_target) / filename;

    std::error_code ec;
    if (!std::filesystem::is_regular_file(path, ec)) {
        return err<std::filesystem::path>(
            ec::mod_load_failed,
            "native library not found: " + path.string());
    }
    return path;
}

} // namespace leaf
