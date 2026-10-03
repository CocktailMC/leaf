#pragma once

#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

#include "leaf/events/event_id.hpp"
#include "leaf/events/event_types.hpp"

namespace leaf {

struct listener_trace_entry {
    subscription_id subscription{0};
    mod_id owner{0};
    event_priority priority{event_priority::normal};
    std::uint64_t duration_ns{0};
    decision vote{decision::pass};
    bool error{false};
};

struct event_trace_record {
    event_id id{0};
    std::uint64_t sequence{0};
    std::uint64_t duration_ns{0};
    decision final_decision{decision::pass};
    std::vector<listener_trace_entry> listeners;
};

struct listener_metrics {
    subscription_id subscription{0};
    mod_id owner{0};
    event_id event{0};
    std::uint64_t calls{0};
    std::uint64_t errors{0};
    std::uint64_t total_ns{0};
    std::uint64_t max_ns{0};
};

class event_trace {
public:
    void set_enabled(bool enabled) noexcept { enabled_ = enabled; }
    [[nodiscard]] bool enabled() const noexcept { return enabled_; }

    void set_slow_threshold_ns(std::uint64_t ns) noexcept { slow_threshold_ns_ = ns; }

    void begin_dispatch(event_id id, std::uint64_t sequence);
    void record_listener(listener_trace_entry entry);
    void end_dispatch(decision final_decision);

    [[nodiscard]] std::vector<event_trace_record> take_records();

    void note_listener_timing(
        subscription_id sub,
        mod_id owner,
        event_id event,
        std::uint64_t duration_ns,
        bool error);

    [[nodiscard]] std::vector<listener_metrics> metrics_snapshot() const;

private:
    bool enabled_{false};
    std::uint64_t slow_threshold_ns_{5'000'000}; // 5ms
    event_trace_record current_{};
    bool in_dispatch_{false};
    std::chrono::steady_clock::time_point start_{};
    mutable std::mutex mutex_;
    std::vector<event_trace_record> history_;
    std::vector<listener_metrics> metrics_;
};

} // namespace leaf
