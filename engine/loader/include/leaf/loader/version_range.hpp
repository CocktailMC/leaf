#pragma once

#include <compare>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "leaf/core/result.hpp"
#include "leaf/core/version.hpp"

namespace leaf {

/// Minecraft-oriented version component that may be a wildcard (`x`).
struct version_component {
    bool wildcard{false};
    std::uint32_t value{0};

    [[nodiscard]] constexpr auto operator<=>(const version_component&) const noexcept = default;
};

/// Parsed Minecraft version pattern such as `1.20.1` or `1.21.x`.
struct minecraft_version {
    version_component major{};
    version_component minor{};
    version_component patch{};

    [[nodiscard]] constexpr bool matches(version concrete) const noexcept {
        auto match_one = [](version_component c, std::uint32_t v) constexpr {
            return c.wildcard || c.value == v;
        };
        return match_one(major, concrete.major)
            && match_one(minor, concrete.minor)
            && match_one(patch, concrete.patch);
    }

    /// Compare as a lower/upper bound using wildcards as 0 / UINT32_MAX.
    [[nodiscard]] constexpr version as_lower_bound() const noexcept {
        return version{
            major.wildcard ? 0u : major.value,
            minor.wildcard ? 0u : minor.value,
            patch.wildcard ? 0u : patch.value,
        };
    }

    [[nodiscard]] constexpr version as_upper_bound() const noexcept {
        return version{
            major.wildcard ? UINT32_MAX : major.value,
            minor.wildcard ? UINT32_MAX : minor.value,
            patch.wildcard ? UINT32_MAX : patch.value,
        };
    }

    [[nodiscard]] std::string to_string() const {
        auto part = [](version_component c) -> std::string {
            return c.wildcard ? "x" : std::to_string(c.value);
        };
        return part(major) + '.' + part(minor) + '.' + part(patch);
    }
};

/// Inclusive Minecraft version window from a manifest `minecraft.min/max`.
struct minecraft_version_range {
    std::optional<minecraft_version> min;
    std::optional<minecraft_version> max;

    [[nodiscard]] constexpr bool contains(version concrete) const noexcept {
        if (min) {
            const auto lower = min->as_lower_bound();
            if (concrete < lower) {
                return false;
            }
            // If min uses wildcards only on patch (1.21.x), any 1.21.* is ok
            // via as_lower_bound (1.21.0). Fine for inclusive min.
        }
        if (max) {
            const auto upper = max->as_upper_bound();
            if (concrete > upper) {
                return false;
            }
        }
        return true;
    }
};

/// Mod dependency version requirement.
/// Currently supports exact or `>=MAJOR.MINOR.PATCH`.
enum class version_req_kind : std::uint8_t {
    any = 0,
    exact = 1,
    at_least = 2,
};

struct version_requirement {
    version_req_kind kind{version_req_kind::any};
    version value{};

    [[nodiscard]] constexpr bool satisfied_by(version candidate) const noexcept {
        switch (kind) {
            case version_req_kind::any:
                return true;
            case version_req_kind::exact:
                return candidate == value;
            case version_req_kind::at_least:
                return candidate >= value;
        }
        return false;
    }

    [[nodiscard]] std::string to_string() const {
        switch (kind) {
            case version_req_kind::any:
                return "*";
            case version_req_kind::exact:
                return value.to_string();
            case version_req_kind::at_least:
                return ">=" + value.to_string();
        }
        return "*";
    }
};

[[nodiscard]] result<minecraft_version> parse_minecraft_version(std::string_view text);
[[nodiscard]] result<version_requirement> parse_version_requirement(std::string_view text);

} // namespace leaf
