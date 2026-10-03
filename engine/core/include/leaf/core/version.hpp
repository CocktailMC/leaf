#pragma once

#include <compare>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "leaf/core/result.hpp"

namespace leaf {

/// Semantic version used for Leaf API, engine, and .leafmod manifests.
struct version {
    std::uint32_t major{0};
    std::uint32_t minor{0};
    std::uint32_t patch{0};

    [[nodiscard]] constexpr auto operator<=>(const version&) const noexcept = default;

    [[nodiscard]] constexpr bool compatible_with(version required) const noexcept {
        // Same major, engine >= required (minor.patch).
        return major == required.major
            && (minor > required.minor
                || (minor == required.minor && patch >= required.patch));
    }

    [[nodiscard]] std::string to_string() const {
        return std::to_string(major) + '.' + std::to_string(minor) + '.'
            + std::to_string(patch);
    }
};

/// Parse "MAJOR.MINOR.PATCH". Wildcards like "1.21.x" are not handled here;
/// they belong to the Minecraft version constraint parser.
[[nodiscard]] inline result<version> parse_version(std::string_view text) {
    if (text.empty()) {
        return err<version>(ec::invalid_argument, "empty version string");
    }

    auto parse_u32 = [](std::string_view s) -> std::optional<std::uint32_t> {
        if (s.empty()) {
            return std::nullopt;
        }
        std::uint32_t value = 0;
        for (char c : s) {
            if (c < '0' || c > '9') {
                return std::nullopt;
            }
            const auto digit = static_cast<std::uint32_t>(c - '0');
            if (value > (UINT32_MAX - digit) / 10) {
                return std::nullopt;
            }
            value = value * 10 + digit;
        }
        return value;
    };

    const auto first_dot = text.find('.');
    if (first_dot == std::string_view::npos) {
        return err<version>(ec::invalid_argument, "expected MAJOR.MINOR.PATCH");
    }
    const auto second_dot = text.find('.', first_dot + 1);
    if (second_dot == std::string_view::npos) {
        return err<version>(ec::invalid_argument, "expected MAJOR.MINOR.PATCH");
    }
    if (text.find('.', second_dot + 1) != std::string_view::npos) {
        return err<version>(ec::invalid_argument, "too many version components");
    }

    const auto major = parse_u32(text.substr(0, first_dot));
    const auto minor = parse_u32(text.substr(first_dot + 1, second_dot - first_dot - 1));
    const auto patch = parse_u32(text.substr(second_dot + 1));
    if (!major || !minor || !patch) {
        return err<version>(ec::invalid_argument, "non-numeric version component");
    }

    return version{*major, *minor, *patch};
}

/// Leaf C ABI major version currently implemented by this engine build.
inline constexpr std::uint32_t leaf_abi_version_major = 1;

/// Engine release version (independent of Minecraft / Loader).
inline constexpr version engine_version{0, 1, 0};

} // namespace leaf
