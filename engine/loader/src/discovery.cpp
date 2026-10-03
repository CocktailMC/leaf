#include "leaf/loader/discovery.hpp"

#include "leaf/loader/leafmod_package.hpp"
#include "leaf/loader/manifest_parser.hpp"

#include <iostream>
#include <string_view>
#include <system_error>

namespace leaf {

namespace {

bool ends_with_leafmod(const std::string& name) {
    constexpr std::string_view leafmod = ".leafmod";
    constexpr std::string_view leaf = ".leaf";
    if (name.size() > leafmod.size()
        && name.compare(
               name.size() - leafmod.size(), leafmod.size(), leafmod)
            == 0) {
        return true;
    }
    // Alias: *.leaf packages (same layout as *.leafmod).
    return name.size() > leaf.size()
        && name.compare(name.size() - leaf.size(), leaf.size(), leaf) == 0;
}

result<discovered_mod> load_discovered(
    const std::filesystem::path& root) {
    const auto manifest_path = root / "leaf.mod.json";
    std::error_code ec;
    if (!std::filesystem::is_regular_file(manifest_path, ec)) {
        return err<discovered_mod>(
            ec::mod_manifest_invalid,
            "missing leaf.mod.json in " + root.string());
    }

    auto manifest = load_mod_manifest(manifest_path);
    if (!manifest) {
        return err<discovered_mod>(
            ec::mod_manifest_invalid,
            "failed to parse " + manifest_path.string() + ": "
                + manifest.error().format());
    }

    return discovered_mod{
        .manifest = std::move(*manifest),
        .root = root,
        .state = mod_state::discovered,
    };
}

void warn_skip(const std::filesystem::path& path, std::string_view reason) {
    std::cerr << "[LEAF][WARN] skipping leafmod " << path.string() << ": "
              << reason << '\n';
}

} // namespace

result<std::vector<discovered_mod>> discover_mods(
    const std::filesystem::path& leafmods_dir) {
    std::error_code ec;
    if (!std::filesystem::exists(leafmods_dir, ec)) {
        return err<std::vector<discovered_mod>>(
            ec::path_not_found,
            "leafmods directory not found: " + leafmods_dir.string());
    }
    if (!std::filesystem::is_directory(leafmods_dir, ec)) {
        return err<std::vector<discovered_mod>>(
            ec::invalid_argument,
            "leafmods path is not a directory: " + leafmods_dir.string());
    }

    std::vector<discovered_mod> mods;

    for (const auto& entry :
        std::filesystem::directory_iterator(leafmods_dir, ec)) {
        if (ec) {
            return err<std::vector<discovered_mod>>(
                ec::io_error,
                ec.message());
        }

        const auto name = entry.path().filename().string();
        if (name.starts_with('.')) {
            continue; // skip .leafmc-extract and other hidden dirs
        }
        if (!ends_with_leafmod(name)) {
            continue;
        }

        auto root = materialize_leafmod_package(entry.path(), leafmods_dir);
        if (!root) {
            warn_skip(entry.path(), root.error().format());
            continue;
        }

        auto mod = load_discovered(*root);
        if (!mod) {
            warn_skip(entry.path(), mod.error().format());
            continue;
        }
        mods.push_back(std::move(*mod));
    }

    return mods;
}

} // namespace leaf
