#include "leaf/events/event_trace.hpp"

#include <algorithm>

namespace leaf {

void event_trace::begin_dispatch(event_id id, std::uint64_t sequence) {
    if (!enabled_) {
        in_dispatch_ = false;
        return;
    }
    current_ = event_trace_record{};
    current_.id = id;
    current_.sequence = sequence;
    current_.listeners.clear();
    start_ = std::chrono::steady_clock::now();
    in_dispatch_ = true;
}

void event_trace::record_listener(listener_trace_entry entry) {
    if (!enabled_ || !in_dispatch_) {
        return;
    }
    current_.listeners.push_back(std::move(entry));
}

void event_trace::end_dispatch(decision final_decision) {
    if (!enabled_ || !in_dispatch_) {
        return;
    }
    const auto end = std::chrono::steady_clock::now();
    current_.duration_ns = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(end - start_).count());
    current_.final_decision = final_decision;

    std::scoped_lock lock(mutex_);
    history_.push_back(std::move(current_));
    if (history_.size() > 256) {
        history_.erase(history_.begin(), history_.begin() + static_cast<std::ptrdiff_t>(history_.size() - 256));
    }
    in_dispatch_ = false;
}

std::vector<event_trace_record> event_trace::take_records() {
    std::scoped_lock lock(mutex_);
    std::vector<event_trace_record> out;
    out.swap(history_);
    return out;
}

void event_trace::note_listener_timing(
    subscription_id sub,
    mod_id owner,
    event_id event,
    std::uint64_t duration_ns,
    bool error) {
    std::scoped_lock lock(mutex_);
    auto it = std::find_if(metrics_.begin(), metrics_.end(), [&](const listener_metrics& m) {
        return m.subscription == sub;
    });
    if (it == metrics_.end()) {
        metrics_.push_back(listener_metrics{
            .subscription = sub,
            .owner = owner,
            .event = event,
        });
        it = metrics_.end() - 1;
    }
    ++it->calls;
    if (error) {
        ++it->errors;
    }
    it->total_ns += duration_ns;
    if (duration_ns > it->max_ns) {
        it->max_ns = duration_ns;
    }
    (void)slow_threshold_ns_;
}

std::vector<listener_metrics> event_trace::metrics_snapshot() const {
    std::scoped_lock lock(mutex_);
    return metrics_;
}

} // namespace leaf
