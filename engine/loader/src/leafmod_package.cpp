#include "leaf/loader/leafmod_package.hpp"

#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>

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
    return name.size() > leaf.size()
        && name.compare(name.size() - leaf.size(), leaf.size(), leaf) == 0;
}

} // namespace

result<std::filesystem::path> materialize_leafmod_package(
    const std::filesystem::path& package,
    const std::filesystem::path& leafmods_dir) {
    std::error_code ec;
    if (std::filesystem::is_directory(package, ec)) {
        return package;
    }
    if (!std::filesystem::is_regular_file(package, ec)) {
        return err<std::filesystem::path>(
            ec::mod_load_failed,
            "not a leafmod package: " + package.string());
    }
    if (!ends_with_leafmod(package.filename().string())) {
        return err<std::filesystem::path>(
            ec::invalid_argument,
            "package name must end with .leafmod or .leaf");
    }

    if (package.string().find(' ') != std::string::npos
        || leafmods_dir.string().find(' ') != std::string::npos) {
        return err<std::filesystem::path>(
            ec::invalid_argument,
            "zip leafmod paths with spaces are not supported yet");
    }

    const auto extract_root = leafmods_dir / ".leafmc-extract" / package.stem();
    const auto marker = extract_root / "leaf.mod.json";
    const auto stamp = extract_root / ".source_size";

    const auto src_size = std::filesystem::file_size(package, ec);
    const std::string size_text =
        std::to_string(static_cast<unsigned long long>(src_size));

    bool needs_extract = !std::filesystem::is_regular_file(marker, ec);
    if (!needs_extract && std::filesystem::is_regular_file(stamp, ec)) {
        FILE* f = std::fopen(stamp.string().c_str(), "rb");
        if (!f) {
            needs_extract = true;
        } else {
            char buf[64]{};
            const auto n = std::fread(buf, 1, sizeof(buf) - 1, f);
            std::fclose(f);
            if (std::string_view{buf, n} != size_text) {
                needs_extract = true;
            }
        }
    } else {
        needs_extract = true;
    }

    if (needs_extract) {
        std::filesystem::remove_all(extract_root, ec);
        std::filesystem::create_directories(extract_root, ec);
        if (ec) {
            return err<std::filesystem::path>(
                ec::io_error,
                "failed to create extract dir: " + extract_root.string());
        }

        const std::string cmd =
            "unzip -qo " + package.string() + " -d " + extract_root.string();
        const int rc = std::system(cmd.c_str());
        if (rc != 0) {
            return err<std::filesystem::path>(
                ec::mod_load_failed,
                "unzip failed for " + package.string()
                    + " (is unzip installed?)");
        }
        if (!std::filesystem::is_regular_file(extract_root / "leaf.mod.json", ec)) {
            return err<std::filesystem::path>(
                ec::mod_manifest_invalid,
                "zip leafmod missing leaf.mod.json: " + package.string());
        }
        FILE* f = std::fopen(stamp.string().c_str(), "wb");
        if (f) {
            std::fwrite(size_text.data(), 1, size_text.size(), f);
            std::fclose(f);
        }
    }

    return extract_root;
}

} // namespace leaf
