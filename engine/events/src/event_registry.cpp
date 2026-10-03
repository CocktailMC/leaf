#include "leaf/events/event_registry.hpp"

namespace leaf {

event_registry::event_registry() = default;

status event_registry::register_core(event_descriptor desc) {
    if (desc.id == 0 || desc.name.empty()) {
        return err(ec::invalid_argument, "invalid core event descriptor");
    }
    if (by_id_.contains(desc.id) || by_name_.contains(desc.name)) {
        return err(ec::already_exists, "event already registered");
    }
    desc.core = true;
    index_(std::move(desc));
    return ok();
}

result<event_id> event_registry::register_dynamic(event_descriptor desc) {
    if (desc.name.empty()) {
        return err<event_id>(ec::invalid_argument, "dynamic event needs a name");
    }
    if (!desc.name.starts_with("mod.")) {
        return err<event_id>(
            ec::invalid_argument,
            "dynamic event name must start with 'mod.'");
    }
    if (by_name_.contains(desc.name)) {
        return err<event_id>(ec::already_exists, "event name already registered");
    }
    desc.core = false;
    desc.id = next_dynamic_++;
    const event_id id = desc.id;
    index_(std::move(desc));
    return id;
}

const event_descriptor* event_registry::find(event_id id) const noexcept {
    const auto it = by_id_.find(id);
    if (it == by_id_.end()) {
        return nullptr;
    }
    return &ordered_[it->second];
}

const event_descriptor* event_registry::find_by_name(std::string_view name) const noexcept {
    const auto it = by_name_.find(std::string{name});
    if (it == by_name_.end()) {
        return nullptr;
    }
    return &ordered_[it->second];
}

void event_registry::unregister_owner(mod_id owner) {
    if (owner == engine_mod_id) {
        return;
    }
    std::vector<event_descriptor> kept;
    kept.reserve(ordered_.size());
    by_id_.clear();
    by_name_.clear();
    for (auto& d : ordered_) {
        if (d.core || d.owner != owner) {
            kept.push_back(std::move(d));
        }
    }
    ordered_.clear();
    for (auto& d : kept) {
        index_(std::move(d));
    }
}

void event_registry::index_(event_descriptor desc) {
    const auto idx = ordered_.size();
    by_id_.emplace(desc.id, idx);
    by_name_.emplace(desc.name, idx);
    ordered_.push_back(std::move(desc));
}

event_registry make_default_event_registry() {
    event_registry reg;

    const auto add = [&](event_id id,
                         std::string_view name,
                         event_kind kind,
                         std::uint32_t schema = 1) {
        event_descriptor d;
        d.id = id;
        d.name = std::string{name};
        d.schema_version = schema;
        d.kind = kind;
        d.dispatch = dispatch_model::main_sync;
        d.policy = decision_policy::deny_over_allow;
        d.core = true;
        d.owner = engine_mod_id;
        (void)reg.register_core(std::move(d));
    };

    add(event_ids::core_server_starting, "leaf.core.server.starting", event_kind::notification);
    add(event_ids::core_server_started, "leaf.core.server.started", event_kind::notification);
    add(event_ids::player_join, "leaf.player.join", event_kind::notification);
    add(event_ids::player_leave, "leaf.player.leave", event_kind::notification);
    add(event_ids::player_join_request, "leaf.player.join_request", event_kind::decision);
    add(event_ids::player_chat, "leaf.player.chat", event_kind::decision);
    add(event_ids::player_death, "leaf.player.death", event_kind::notification);
    add(event_ids::entity_spawn, "leaf.entity.spawn", event_kind::notification);
    add(event_ids::entity_remove, "leaf.entity.remove", event_kind::notification);
    add(event_ids::world_load, "leaf.world.load", event_kind::notification);
    add(event_ids::block_break, "leaf.world.block_break", event_kind::decision);
    add(event_ids::block_place, "leaf.world.block_place", event_kind::decision);
    add(event_ids::server_tick, "leaf.server.tick", event_kind::notification);

    return reg;
}

} // namespace leaf
