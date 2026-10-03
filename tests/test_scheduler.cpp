#include "leaf/events/events.hpp"
#include "leaf/scheduler/scheduler.hpp"
#include "test_harness.hpp"

#include <atomic>
#include <chrono>
#include <thread>

using namespace std::chrono_literals;

void test_scheduler_main_async_delay() {
    leaf::scheduler_config cfg;
    cfg.worker_threads = 2;
    leaf::scheduler sched{cfg};
    sched.bind_main_thread();

    std::atomic<int> main_hits{0};
    std::atomic<int> async_hits{0};
    std::atomic<int> delay_hits{0};

    LEAF_CHECK(sched.post_main([&] { ++main_hits; }, /*inline*/ true).has_value());
    LEAF_CHECK(main_hits.load() == 1);

    LEAF_CHECK(sched.post_main([&] { ++main_hits; }, /*inline*/ false).has_value());
    LEAF_CHECK(main_hits.load() == 1);
    LEAF_CHECK(sched.pump_main().has_value());
    LEAF_CHECK(main_hits.load() == 2);

    LEAF_CHECK(sched.post_async([&] { ++async_hits; }).has_value());
    for (int i = 0; i < 100 && async_hits.load() == 0; ++i) {
        std::this_thread::sleep_for(5ms);
    }
    LEAF_CHECK(async_hits.load() == 1);

    auto delayed = sched.delay([&] { ++delay_hits; }, 30ms);
    LEAF_CHECK(delayed.has_value());
    LEAF_CHECK(sched.pump_main().has_value());
    LEAF_CHECK(delay_hits.load() == 0);
    std::this_thread::sleep_for(40ms);
    LEAF_CHECK(sched.pump_main().has_value());
    LEAF_CHECK(delay_hits.load() == 1);

    auto rep = sched.repeat([&] { ++delay_hits; }, 20ms);
    LEAF_CHECK(rep.has_value());
    std::this_thread::sleep_for(50ms);
    LEAF_CHECK(sched.pump_main().has_value());
    LEAF_CHECK(delay_hits.load() >= 2);
    rep->cancel();

    sched.shutdown();
}

void test_event_requires_main_when_bound() {
    leaf::scheduler sched;
    leaf::event_runtime rt;
    rt.attach_scheduler(&sched);
    sched.bind_main_thread();

    // Publishing from main is OK.
    struct payload {
        std::uint32_t pad{0};
    } p{};
    LEAF_CHECK(rt.publish_notification(leaf::event_ids::core_server_started, p).has_value());

    // Simulate off-main by clearing bind via a worker posting a publish.
    std::atomic<leaf::error_code> off_main_code{leaf::ok_code};
    LEAF_CHECK(sched.post_async([&] {
        auto st = rt.publish_notification(leaf::event_ids::core_server_started, p);
        if (!st) {
            off_main_code = st.error().code();
        }
    }).has_value());

    for (int i = 0; i < 100 && off_main_code.load() == leaf::ok_code; ++i) {
        std::this_thread::sleep_for(5ms);
    }
    LEAF_CHECK(off_main_code.load() == LEAF_EVENT_THREAD_VIOLATION);

    sched.shutdown();
}

void test_scheduler_bundle() {
    test_scheduler_main_async_delay();
    test_event_requires_main_when_bound();
}
