#include "leaf/loader/dependency_graph.hpp"

#include "leaf/core/version.hpp"

#include <queue>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace leaf {
namespace {

[[nodiscard]] status ensure_unique_ids(const std::vector<discovered_mod>& mods) {
    std::unordered_set<std::string> seen;
    for (const auto& mod : mods) {
        if (!seen.insert(mod.manifest.id).second) {
            return err(
                ec::mod_duplicate_id,
                "duplicate mod id '" + mod.manifest.id + "'");
        }
    }
    return ok();
}

[[nodiscard]] status check_minecraft_and_targets(
    const discovered_mod& mod,
    const resolve_options& options) {
    if (!mod.manifest.minecraft.contains(options.minecraft)) {
        return err(
            ec::mod_version_incompatible,
            "mod '" + mod.manifest.id + "' does not support Minecraft "
                + options.minecraft.to_string());
    }

    if (!options.host_target.empty() && !mod.manifest.targets.empty()) {
        bool ok_target = false;
        for (const auto& t : mod.manifest.targets) {
            if (t == options.host_target) {
                ok_target = true;
                break;
            }
        }
        if (!ok_target) {
            return err(
                ec::mod_version_incompatible,
                "mod '" + mod.manifest.id + "' has no binary for target "
                    + options.host_target);
        }
    }

    if (mod.manifest.leaf_api != leaf_abi_version_major) {
        return err(
            ec::abi_version_mismatch,
            "mod '" + mod.manifest.id + "' requires Leaf ABI "
                + std::to_string(mod.manifest.leaf_api) + ", engine provides "
                + std::to_string(leaf_abi_version_major));
    }

    return ok();
}

} // namespace

result<resolve_report> resolve_mods(
    std::vector<discovered_mod> mods,
    const resolve_options& options) {
    if (auto st = ensure_unique_ids(mods); !st) {
        return err<resolve_report>(st.error());
    }

    std::unordered_map<std::string, std::size_t> index;
    index.reserve(mods.size());
    for (std::size_t i = 0; i < mods.size(); ++i) {
        index.emplace(mods[i].manifest.id, i);
    }

    for (auto& mod : mods) {
        if (auto st = check_minecraft_and_targets(mod, options); !st) {
            return err<resolve_report>(st.error());
        }
    }

    // adjacency: dependency -> dependents (edges for Kahn: dep before mod)
    // For each required dep, edge dep_id -> mod_id meaning dep must load first.
    std::vector<std::vector<std::size_t>> outgoing(mods.size());
    std::vector<std::size_t> indegree(mods.size(), 0);

    for (std::size_t i = 0; i < mods.size(); ++i) {
        for (const auto& dep : mods[i].manifest.dependencies) {
            const auto it = index.find(dep.id);
            if (it == index.end()) {
                if (dep.optional) {
                    continue;
                }
                return err<resolve_report>(
                    ec::mod_dependency_unsatisfied,
                    "mod '" + mods[i].manifest.id + "' requires missing dependency '"
                        + dep.id + "'");
            }

            const auto& provider = mods[it->second];
            if (!dep.version.satisfied_by(provider.manifest.mod_version)) {
                return err<resolve_report>(
                    ec::mod_dependency_unsatisfied,
                    "mod '" + mods[i].manifest.id + "' requires '" + dep.id
                        + "' " + dep.version.to_string() + ", found "
                        + provider.manifest.mod_version.to_string());
            }

            // Edge: provider -> dependent
            outgoing[it->second].push_back(i);
            ++indegree[i];
        }
    }

    std::queue<std::size_t> ready;
    for (std::size_t i = 0; i < mods.size(); ++i) {
        if (indegree[i] == 0) {
            ready.push(i);
        }
    }

    resolve_report report;
    report.load_order.reserve(mods.size());

    while (!ready.empty()) {
        const auto i = ready.front();
        ready.pop();

        mods[i].state = mod_state::resolved;
        report.load_order.push_back(std::move(mods[i]));

        for (const auto dependent : outgoing[i]) {
            if (--indegree[dependent] == 0) {
                ready.push(dependent);
            }
        }
    }

    if (report.load_order.size() != mods.size()) {
        return err<resolve_report>(
            ec::mod_dependency_cycle,
            "dependency cycle detected among Leaf Mods");
    }

    return report;
}

} // namespace leaf
