#include "leaf/loader/version_range.hpp"

#include <cctype>

namespace leaf {
namespace {

[[nodiscard]] bool is_digit(char c) noexcept {
    return c >= '0' && c <= '9';
}

[[nodiscard]] result<version_component> parse_component(std::string_view text) {
    if (text.empty()) {
        return err<version_component>(
            ec::invalid_argument,
            "empty version component");
    }
    if (text == "x" || text == "X" || text == "*") {
        return version_component{.wildcard = true, .value = 0};
    }
    std::uint32_t value = 0;
    for (char c : text) {
        if (!is_digit(c)) {
            return err<version_component>(
                ec::invalid_argument,
                "invalid version component");
        }
        const auto digit = static_cast<std::uint32_t>(c - '0');
        if (value > (UINT32_MAX - digit) / 10) {
            return err<version_component>(
                ec::invalid_argument,
                "version component overflow");
        }
        value = value * 10 + digit;
    }
    return version_component{.wildcard = false, .value = value};
}

} // namespace

result<minecraft_version> parse_minecraft_version(std::string_view text) {
    if (text.empty()) {
        return err<minecraft_version>(
            ec::invalid_argument,
            "empty minecraft version");
    }

    const auto first = text.find('.');
    if (first == std::string_view::npos) {
        return err<minecraft_version>(
            ec::invalid_argument,
            "expected MAJOR.MINOR.PATCH");
    }
    const auto second = text.find('.', first + 1);
    if (second == std::string_view::npos) {
        return err<minecraft_version>(
            ec::invalid_argument,
            "expected MAJOR.MINOR.PATCH");
    }
    if (text.find('.', second + 1) != std::string_view::npos) {
        return err<minecraft_version>(
            ec::invalid_argument,
            "too many version components");
    }

    const auto major = parse_component(text.substr(0, first));
    if (!major) {
        return err<minecraft_version>(major.error());
    }
    const auto minor = parse_component(text.substr(first + 1, second - first - 1));
    if (!minor) {
        return err<minecraft_version>(minor.error());
    }
    const auto patch = parse_component(text.substr(second + 1));
    if (!patch) {
        return err<minecraft_version>(patch.error());
    }

    return minecraft_version{*major, *minor, *patch};
}

result<version_requirement> parse_version_requirement(std::string_view text) {
    if (text.empty() || text == "*" || text == "any") {
        return version_requirement{};
    }

    version_req_kind kind = version_req_kind::exact;
    std::string_view body = text;
    if (text.size() >= 2 && text[0] == '>' && text[1] == '=') {
        kind = version_req_kind::at_least;
        body = text.substr(2);
    }

    const auto ver = parse_version(body);
    if (!ver) {
        return err<version_requirement>(
            ec::invalid_argument,
            "invalid dependency version requirement");
    }
    return version_requirement{.kind = kind, .value = *ver};
}

} // namespace leaf
