#include "leaf/events/event_runtime.hpp"

#include "leaf/scheduler/scheduler.hpp"

#include <algorithm>
#include <chrono>
#include <exception>

namespace leaf {
namespace {

[[nodiscard]] int priority_rank(event_priority p) noexcept {
    return static_cast<int>(p);
}

} // namespace

event_runtime::event_runtime()
    : event_runtime(make_default_event_registry()) {}

event_runtime::event_runtime(event_registry registry)
    : registry_(std::move(registry)) {}

std::uint64_t event_runtime::now_ns_() const {
    using namespace std::chrono;
    return static_cast<std::uint64_t>(
        duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count());
}

status event_runtime::ensure_can_dispatch_(const event_descriptor& desc) {
    if (desc.dispatch != dispatch_model::main_sync) {
        return err(
            ec::not_supported,
            "async/parallel dispatch models require enqueue path (not direct publish)");
    }
    if (scheduler_ != nullptr && scheduler_->main_thread_bound()
        && !scheduler_->is_main_thread()) {
        return err(
            LEAF_EVENT_THREAD_VIOLATION,
            "main_sync event requires the main thread; use scheduler.post_main");
    }
    if (dispatch_depth_ >= max_dispatch_depth) {
        return err(
            ec::event_recursive_limit,
            "event dispatch recursion limit exceeded");
    }
    return ok();
}

void event_runtime::rebuild_snapshot_unlocked_(event_id id) {
    auto snap = std::make_shared<dispatch_snapshot>();
    snap->generation = ++snapshot_generation_[id];

    for (const auto& [sub_id, rec] : listeners_) {
        (void)sub_id;
        if (!rec.active || rec.event != id) {
            continue;
        }
        snap->listeners.push_back(rec);
    }

    std::stable_sort(
        snap->listeners.begin(),
        snap->listeners.end(),
        [](const listener_record& a, const listener_record& b) {
            if (priority_rank(a.priority) != priority_rank(b.priority)) {
                return priority_rank(a.priority) < priority_rank(b.priority);
            }
            // Same priority: registration sequence (mod load order encoded by
            // caller via subscribe timing; within a mod, subscribe order).
            return a.sequence < b.sequence;
        });

    snapshots_[id] = std::move(snap);
}

std::shared_ptr<const event_runtime::dispatch_snapshot>
event_runtime::snapshot_for_(event_id id) {
    std::scoped_lock lock(mutex_);
    auto it = snapshots_.find(id);
    if (it == snapshots_.end() || !it->second) {
        rebuild_snapshot_unlocked_(id);
        it = snapshots_.find(id);
    }
    return it->second;
}

result<subscription> event_runtime::subscribe_notification(
    event_id id,
    notification_callback callback,
    event_priority priority,
    mod_id owner) {
    if (!callback) {
        return err<subscription>(ec::invalid_argument, "null callback");
    }
    const auto* desc = registry_.find(id);
    if (!desc) {
        return err<subscription>(ec::event_not_found, "unknown event id");
    }
    if (desc->kind != event_kind::notification && desc->kind != event_kind::internal_signal) {
        return err<subscription>(
            ec::event_wrong_kind,
            "use subscribe_decision for decision events");
    }

    listener_record rec;
    rec.id = next_subscription_++;
    rec.owner = owner;
    rec.event = id;
    rec.priority = priority;
    rec.sequence = next_reg_sequence_++;
    rec.notify = std::move(callback);
    rec.decision_listener = false;
    rec.active = true;

    {
        std::scoped_lock lock(mutex_);
        listeners_.emplace(rec.id, rec);
        rebuild_snapshot_unlocked_(id);
    }

    return subscription{this, rec.id};
}

result<subscription> event_runtime::subscribe_decision(
    event_id id,
    decision_callback callback,
    event_priority priority,
    mod_id owner) {
    if (!callback) {
        return err<subscription>(ec::invalid_argument, "null callback");
    }
    if (priority == event_priority::monitor) {
        return err<subscription>(
            ec::event_monitor_mutation,
            "monitor listeners cannot participate in decisions");
    }
    const auto* desc = registry_.find(id);
    if (!desc) {
        return err<subscription>(ec::event_not_found, "unknown event id");
    }
    if (desc->kind != event_kind::decision) {
        return err<subscription>(
            ec::event_wrong_kind,
            "use subscribe_notification for notification events");
    }

    listener_record rec;
    rec.id = next_subscription_++;
    rec.owner = owner;
    rec.event = id;
    rec.priority = priority;
    rec.sequence = next_reg_sequence_++;
    rec.decide = std::move(callback);
    rec.decision_listener = true;
    rec.active = true;

    {
        std::scoped_lock lock(mutex_);
        listeners_.emplace(rec.id, rec);
        rebuild_snapshot_unlocked_(id);
    }

    return subscription{this, rec.id};
}

status event_runtime::unsubscribe(subscription_id id) {
    std::scoped_lock lock(mutex_);
    auto it = listeners_.find(id);
    if (it == listeners_.end()) {
        return err(ec::subscription_invalid, "unknown subscription");
    }
    const event_id event = it->second.event;
    listeners_.erase(it);
    rebuild_snapshot_unlocked_(event);
    return ok();
}

void event_runtime::unsubscribe_owner(mod_id owner) {
    std::scoped_lock lock(mutex_);
    std::vector<event_id> touched;
    for (auto it = listeners_.begin(); it != listeners_.end();) {
        if (it->second.owner == owner) {
            touched.push_back(it->second.event);
            it = listeners_.erase(it);
        } else {
            ++it;
        }
    }
    for (event_id id : touched) {
        rebuild_snapshot_unlocked_(id);
    }
    registry_.unregister_owner(owner);
}

std::uint64_t event_runtime::listener_count(event_id id) const {
    std::scoped_lock lock(mutex_);
    const auto it = snapshots_.find(id);
    if (it == snapshots_.end() || !it->second) {
        return 0;
    }
    return it->second->listeners.size();
}

status event_runtime::publish_notification(
    event_id id,
    std::span<const std::byte> payload,
    publish_options options,
    std::span<const std::byte> variable) {
    const auto* desc = registry_.find(id);
    if (!desc) {
        return err(ec::event_not_found, "unknown event id");
    }
    if (desc->kind == event_kind::decision) {
        return err(ec::event_wrong_kind, "use publish_decision");
    }
    if (options.schema_version != desc->schema_version) {
        // v1: exact match only; future: accept older schemas with adapters.
        return err(
            ec::event_schema_mismatch,
            "schema version mismatch");
    }
    if (auto st = ensure_can_dispatch_(*desc); !st) {
        return st;
    }

    const auto sequence = next_event_sequence_++;
    const auto parent = current_sequence_;
    const auto root = dispatch_depth_ == 0 ? sequence : root_sequence_;

    ++dispatch_depth_;
    parent_sequence_ = parent;
    current_sequence_ = sequence;
    if (dispatch_depth_ == 1) {
        root_sequence_ = root;
    }

    arena_.reset();
    event_header header{};
    header.id = id;
    header.schema_version = options.schema_version;
    header.timestamp_ns = now_ns_();
    header.sequence = sequence;
    header.source = static_cast<std::uint32_t>(options.source);

    auto packet = arena_.build_packet(header, payload, variable);
    if (!packet) {
        --dispatch_depth_;
        current_sequence_ = parent;
        return err(packet.error());
    }

    auto snap = snapshot_for_(id);
    trace_.begin_dispatch(id, sequence);

    status overall = ok();

    for (const auto& listener : snap->listeners) {
        if (listener.priority == event_priority::monitor) {
            // Monitor runs after normal listeners; still notification-only.
        }
        const auto t0 = std::chrono::steady_clock::now();
        bool error = false;
        try {
            if (listener.notify) {
                listener.notify(*packet);
            }
        } catch (const std::exception&) {
            error = true;
            overall = err(ec::event_callback_failed, "listener exception");
        } catch (...) {
            error = true;
            overall = err(ec::event_callback_failed, "listener exception");
        }
        const auto t1 = std::chrono::steady_clock::now();
        const auto dur = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());

        trace_.note_listener_timing(listener.id, listener.owner, id, dur, error);
        trace_.record_listener(listener_trace_entry{
            .subscription = listener.id,
            .owner = listener.owner,
            .priority = listener.priority,
            .duration_ns = dur,
            .vote = decision::pass,
            .error = error,
        });
    }

    trace_.end_dispatch(decision::pass);

    --dispatch_depth_;
    current_sequence_ = parent;
    arena_.reset();

    // Notification publish succeeds even if a listener errored; errors are
    // isolated. Return first error for diagnostics if any.
    return overall;
}

result<decision_result> event_runtime::publish_decision(
    event_id id,
    std::span<const std::byte> payload,
    publish_options options,
    std::span<const std::byte> variable) {
    const auto* desc = registry_.find(id);
    if (!desc) {
        return err<decision_result>(ec::event_not_found, "unknown event id");
    }
    if (desc->kind != event_kind::decision) {
        return err<decision_result>(ec::event_wrong_kind, "use publish_notification");
    }
    if (options.schema_version != desc->schema_version) {
        return err<decision_result>(
            ec::event_schema_mismatch,
            "schema version mismatch");
    }
    if (auto st = ensure_can_dispatch_(*desc); !st) {
        return err<decision_result>(st.error());
    }

    const auto sequence = next_event_sequence_++;
    const auto parent = current_sequence_;
    const auto root = dispatch_depth_ == 0 ? sequence : root_sequence_;

    ++dispatch_depth_;
    parent_sequence_ = parent;
    current_sequence_ = sequence;
    if (dispatch_depth_ == 1) {
        root_sequence_ = root;
    }

    arena_.reset();
    event_header header{};
    header.id = id;
    header.schema_version = options.schema_version;
    header.timestamp_ns = now_ns_();
    header.sequence = sequence;
    header.source = static_cast<std::uint32_t>(options.source);

    auto packet = arena_.build_packet(header, payload, variable);
    if (!packet) {
        --dispatch_depth_;
        current_sequence_ = parent;
        return err<decision_result>(packet.error());
    }

    auto snap = snapshot_for_(id);
    trace_.begin_dispatch(id, sequence);

    std::vector<decision> votes;
    votes.reserve(snap->listeners.size());

    for (const auto& listener : snap->listeners) {
        const auto t0 = std::chrono::steady_clock::now();
        decision vote = decision::pass;
        bool error = false;

        try {
            if (listener.decision_listener && listener.decide) {
                vote = listener.decide(*packet);
            } else if (listener.notify) {
                // Allow notification-style observers on decision events only
                // at monitor priority (read-only).
                if (listener.priority == event_priority::monitor) {
                    listener.notify(*packet);
                    vote = decision::pass;
                }
            }
        } catch (...) {
            error = true;
            vote = decision::pass;
        }

        const auto t1 = std::chrono::steady_clock::now();
        const auto dur = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());

        if (!error && listener.decision_listener) {
            votes.push_back(vote);
        }

        trace_.note_listener_timing(listener.id, listener.owner, id, dur, error);
        trace_.record_listener(listener_trace_entry{
            .subscription = listener.id,
            .owner = listener.owner,
            .priority = listener.priority,
            .duration_ns = dur,
            .vote = vote,
            .error = error,
        });
    }

    const decision final_decision = reduce_decisions(votes, desc->policy);
    trace_.end_dispatch(final_decision);

    --dispatch_depth_;
    current_sequence_ = parent;
    arena_.reset();

    return decision_result{
        .final_decision = final_decision,
        .sequence = sequence,
    };
}

} // namespace leaf
