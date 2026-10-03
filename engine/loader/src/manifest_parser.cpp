#include "leaf/loader/manifest_parser.hpp"

#include "json_lite.hpp"

#include <cctype>
#include <fstream>
#include <sstream>

namespace leaf {
namespace {

[[nodiscard]] result<const json::object*> require_object(
    const json::value& v,
    std::string_view field) {
    const auto* obj = v.as_object();
    if (!obj) {
        return err<const json::object*>(
            ec::mod_manifest_invalid,
            std::string(field) + " must be an object");
    }
    return obj;
}

[[nodiscard]] result<std::string> require_string(
    const json::object& obj,
    std::string_view key) {
    const auto it = obj.find(key);
    if (it == obj.end() || !it->second.as_string()) {
        return err<std::string>(
            ec::mod_manifest_invalid,
            "missing or invalid string field '" + std::string(key) + "'");
    }
    return *it->second.as_string();
}

[[nodiscard]] result<std::uint32_t> require_u32_string_or_number(
    const json::object& obj,
    std::string_view key) {
    const auto it = obj.find(key);
    if (it == obj.end()) {
        return err<std::uint32_t>(
            ec::mod_manifest_invalid,
            "missing field '" + std::string(key) + "'");
    }
    if (const auto* s = it->second.as_string()) {
        if (s->empty()) {
            return err<std::uint32_t>(
                ec::mod_manifest_invalid,
                "empty numeric string for '" + std::string(key) + "'");
        }
        std::uint32_t value = 0;
        for (char c : *s) {
            if (c < '0' || c > '9') {
                return err<std::uint32_t>(
                    ec::mod_manifest_invalid,
                    "non-digit in '" + std::string(key) + "'");
            }
            value = value * 10u + static_cast<std::uint32_t>(c - '0');
        }
        return value;
    }
    if (const auto* n = it->second.as_number()) {
        if (*n < 0 || *n > static_cast<double>(UINT32_MAX)) {
            return err<std::uint32_t>(
                ec::mod_manifest_invalid,
                "numeric field out of range for '" + std::string(key) + "'");
        }
        return static_cast<std::uint32_t>(*n);
    }
    return err<std::uint32_t>(
        ec::mod_manifest_invalid,
        "field '" + std::string(key) + "' must be string or number");
}

[[nodiscard]] result<mod_dependency> parse_dependency(
    const json::value& node,
    bool optional) {
    if (const auto* s = node.as_string()) {
        if (s->empty()) {
            return err<mod_dependency>(
                ec::mod_manifest_invalid,
                "empty dependency id");
        }
        return mod_dependency{.id = *s, .optional = optional};
    }

    const auto* obj = node.as_object();
    if (!obj) {
        return err<mod_dependency>(
            ec::mod_manifest_invalid,
            "dependency must be string or object");
    }

    auto id = require_string(*obj, "id");
    if (!id) {
        return err<mod_dependency>(id.error());
    }

    version_requirement req{};
    if (const auto it = obj->find("version"); it != obj->end()) {
        if (!it->second.as_string()) {
            return err<mod_dependency>(
                ec::mod_manifest_invalid,
                "dependency.version must be a string");
        }
        auto parsed = parse_version_requirement(*it->second.as_string());
        if (!parsed) {
            return err<mod_dependency>(parsed.error());
        }
        req = *parsed;
    }

    return mod_dependency{
        .id = std::move(*id),
        .version = req,
        .optional = optional,
    };
}

[[nodiscard]] status append_dependencies(
    std::vector<mod_dependency>& out,
    const json::object& deps_obj,
    std::string_view key,
    bool optional) {
    const auto it = deps_obj.find(key);
    if (it == deps_obj.end()) {
        return ok();
    }
    const auto* arr = it->second.as_array();
    if (!arr) {
        return err(
            ec::mod_manifest_invalid,
            "dependencies." + std::string(key) + " must be an array");
    }
    for (const auto& item : *arr) {
        auto dep = parse_dependency(item, optional);
        if (!dep) {
            return err(dep.error());
        }
        out.push_back(std::move(*dep));
    }
    return ok();
}

[[nodiscard]] bool is_valid_mod_id(std::string_view id) noexcept {
    if (id.empty() || id.size() > 64) {
        return false;
    }
    if (!(std::islower(static_cast<unsigned char>(id[0]))
            || std::isdigit(static_cast<unsigned char>(id[0])))) {
        return false;
    }
    for (char c : id) {
        const auto uc = static_cast<unsigned char>(c);
        if (!(std::islower(uc) || std::isdigit(uc) || c == '_' || c == '-')) {
            return false;
        }
    }
    return true;
}

} // namespace

result<mod_manifest> parse_mod_manifest(std::string_view json_text) {
    auto root = json::parse(json_text);
    if (!root) {
        return err<mod_manifest>(root.error());
    }

    auto obj_r = require_object(*root, "root");
    if (!obj_r) {
        return err<mod_manifest>(obj_r.error());
    }
    const json::object& obj = **obj_r;

    mod_manifest manifest;

    auto id = require_string(obj, "id");
    if (!id) {
        return err<mod_manifest>(id.error());
    }
    if (!is_valid_mod_id(*id)) {
        return err<mod_manifest>(
            ec::mod_manifest_invalid,
            "invalid mod id '" + *id + "'");
    }
    manifest.id = std::move(*id);

    auto name = require_string(obj, "name");
    if (!name) {
        return err<mod_manifest>(name.error());
    }
    manifest.name = std::move(*name);

    auto ver_s = require_string(obj, "version");
    if (!ver_s) {
        return err<mod_manifest>(ver_s.error());
    }
    auto ver = parse_version(*ver_s);
    if (!ver) {
        return err<mod_manifest>(
            ec::mod_manifest_invalid,
            "invalid mod version");
    }
    manifest.mod_version = *ver;

    if (const auto leaf_it = obj.find("leaf"); leaf_it != obj.end()) {
        auto leaf_obj = require_object(leaf_it->second, "leaf");
        if (!leaf_obj) {
            return err<mod_manifest>(leaf_obj.error());
        }
        auto api = require_u32_string_or_number(**leaf_obj, "api");
        if (!api) {
            return err<mod_manifest>(api.error());
        }
        manifest.leaf_api = *api;
    }

    if (const auto rt = obj.find("runtime"); rt != obj.end()) {
        const auto* s = rt->second.as_string();
        if (!s) {
            return err<mod_manifest>(
                ec::mod_manifest_invalid,
                "runtime must be a string");
        }
        if (*s != "native") {
            return err<mod_manifest>(
                ec::mod_manifest_invalid,
                "unsupported runtime '" + *s + "'");
        }
        manifest.runtime = mod_runtime::native;
    }

    if (const auto entry = obj.find("entry"); entry != obj.end()) {
        const auto* s = entry->second.as_string();
        if (!s || s->empty()) {
            return err<mod_manifest>(
                ec::mod_manifest_invalid,
                "entry must be a non-empty string");
        }
        manifest.entry = *s;
    }

    if (const auto mc = obj.find("minecraft"); mc != obj.end()) {
        auto mc_obj = require_object(mc->second, "minecraft");
        if (!mc_obj) {
            return err<mod_manifest>(mc_obj.error());
        }
        if (const auto min_it = (**mc_obj).find("min"); min_it != (**mc_obj).end()) {
            const auto* s = min_it->second.as_string();
            if (!s) {
                return err<mod_manifest>(
                    ec::mod_manifest_invalid,
                    "minecraft.min must be a string");
            }
            auto parsed = parse_minecraft_version(*s);
            if (!parsed) {
                return err<mod_manifest>(parsed.error());
            }
            manifest.minecraft.min = *parsed;
        }
        if (const auto max_it = (**mc_obj).find("max"); max_it != (**mc_obj).end()) {
            const auto* s = max_it->second.as_string();
            if (!s) {
                return err<mod_manifest>(
                    ec::mod_manifest_invalid,
                    "minecraft.max must be a string");
            }
            auto parsed = parse_minecraft_version(*s);
            if (!parsed) {
                return err<mod_manifest>(parsed.error());
            }
            manifest.minecraft.max = *parsed;
        }
    }

    if (const auto deps = obj.find("dependencies"); deps != obj.end()) {
        auto deps_obj = require_object(deps->second, "dependencies");
        if (!deps_obj) {
            return err<mod_manifest>(deps_obj.error());
        }
        if (auto st = append_dependencies(
                manifest.dependencies, **deps_obj, "required", false);
            !st) {
            return err<mod_manifest>(st.error());
        }
        if (auto st = append_dependencies(
                manifest.dependencies, **deps_obj, "optional", true);
            !st) {
            return err<mod_manifest>(st.error());
        }
    }

    if (const auto targets = obj.find("targets"); targets != obj.end()) {
        const auto* arr = targets->second.as_array();
        if (!arr) {
            return err<mod_manifest>(
                ec::mod_manifest_invalid,
                "targets must be an array");
        }
        for (const auto& item : *arr) {
            const auto* s = item.as_string();
            if (!s || s->empty()) {
                return err<mod_manifest>(
                    ec::mod_manifest_invalid,
                    "targets entries must be non-empty strings");
            }
            manifest.targets.push_back(*s);
        }
    }

    return manifest;
}

result<mod_manifest> load_mod_manifest(const std::filesystem::path& path) {
    std::ifstream in(path);
    if (!in) {
        return err<mod_manifest>(
            ec::path_not_found,
            "cannot open " + path.string());
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    return parse_mod_manifest(ss.str());
}

} // namespace leaf
