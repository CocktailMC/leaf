#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

#include "leaf/core/result.hpp"
#include "leaf/scheduler/thread_kind.hpp"

namespace leaf {

using task_id = std::uint64_t;
using clock = std::chrono::steady_clock;
using task_fn = std::function<void()>;

/// Cancel handle for delay/repeat tasks. Destructor does NOT cancel;
/// call cancel() explicitly (or use cancel_owner on unload).
class task_handle {
public:
    task_handle() = default;
    task_handle(class scheduler* sched, task_id id) noexcept;

    void cancel() noexcept;

    [[nodiscard]] bool active() const noexcept;
    [[nodiscard]] task_id id() const noexcept { return id_; }

private:
    class scheduler* sched_{nullptr};
    task_id id_{0};
};

struct scheduler_config {
    std::size_t worker_threads{2};
    std::size_t max_queue_depth{10'000};
};

/// LEAF scheduler: main queue + worker pool + delayed/repeating tasks.
/// Event Bus is NOT a scheduler — world-touching work must go through here.
class scheduler {
public:
    explicit scheduler(scheduler_config config = {});
    ~scheduler();

    scheduler(const scheduler&) = delete;
    scheduler& operator=(const scheduler&) = delete;

    /// Mark the calling thread as the Minecraft / LEAF main thread.
    void bind_main_thread() noexcept;

    [[nodiscard]] bool main_thread_bound() const noexcept { return main_bound_; }
    [[nodiscard]] bool is_main_thread() const noexcept;
    [[nodiscard]] bool is_worker_thread() const noexcept;
    [[nodiscard]] thread_kind current_thread_kind() const noexcept;

    /// Queue work for the main thread. Runs immediately if already on main
    /// and `inline_if_main` is true.
    [[nodiscard]] status post_main(task_fn fn, bool inline_if_main = true);

    /// Queue work on the worker pool.
    [[nodiscard]] status post_async(task_fn fn);

    [[nodiscard]] result<task_handle> delay(task_fn fn, clock::duration after);

    [[nodiscard]] result<task_handle> repeat(task_fn fn, clock::duration interval);

    [[nodiscard]] status cancel(task_id id);

    /// Drain due timers into main/async queues and run up to `budget` main tasks.
    /// Must be called from the main thread (Bridge tick / tests).
    [[nodiscard]] status pump_main(std::size_t budget = 256);

    void shutdown();

    [[nodiscard]] bool running() const noexcept { return running_.load(); }

    [[nodiscard]] std::size_t main_queue_size() const;
    [[nodiscard]] std::size_t async_queue_size() const;

    friend class task_handle;

private:
    struct timed_task {
        task_id id{0};
        clock::time_point when{};
        clock::duration interval{}; // zero => one-shot
        bool repeat{false};
        bool on_main{true};
        task_fn fn;
        bool cancelled{false};
    };

    void worker_loop_(std::size_t index);
    void enqueue_due_timers_unlocked_(clock::time_point now);
    [[nodiscard]] status enqueue_main_unlocked_(task_fn fn);
    [[nodiscard]] status enqueue_async_unlocked_(task_fn fn);

    scheduler_config config_;
    std::atomic<bool> running_{false};
    std::atomic<task_id> next_id_{1};

    std::thread::id main_thread_id_{};
    bool main_bound_{false};

    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::deque<task_fn> main_queue_;
    std::deque<task_fn> async_queue_;
    std::unordered_map<task_id, timed_task> timed_;
    std::vector<std::thread> workers_;
    std::vector<std::thread::id> worker_ids_;
};

} // namespace leaf
