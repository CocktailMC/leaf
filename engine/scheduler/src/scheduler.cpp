#include "leaf/scheduler/scheduler.hpp"

#include <algorithm>
#include <utility>

namespace leaf {

task_handle::task_handle(scheduler* sched, task_id id) noexcept
    : sched_(sched)
    , id_(id) {}

void task_handle::cancel() noexcept {
    if (sched_ && id_ != 0) {
        (void)sched_->cancel(id_);
    }
    sched_ = nullptr;
    id_ = 0;
}

bool task_handle::active() const noexcept {
    return sched_ != nullptr && id_ != 0;
}

scheduler::scheduler(scheduler_config config)
    : config_(config)
    , running_(true) {
    worker_ids_.resize(config_.worker_threads);
    workers_.reserve(config_.worker_threads);
    for (std::size_t i = 0; i < config_.worker_threads; ++i) {
        workers_.emplace_back([this, i] { worker_loop_(i); });
    }
}

scheduler::~scheduler() {
    shutdown();
}

void scheduler::shutdown() {
    bool expected = true;
    if (!running_.compare_exchange_strong(expected, false)) {
        return;
    }
    cv_.notify_all();
    for (auto& w : workers_) {
        if (w.joinable()) {
            w.join();
        }
    }
    workers_.clear();
    std::scoped_lock lock(mutex_);
    main_queue_.clear();
    async_queue_.clear();
    timed_.clear();
}

void scheduler::bind_main_thread() noexcept {
    main_thread_id_ = std::this_thread::get_id();
    main_bound_ = true;
}

bool scheduler::is_main_thread() const noexcept {
    return main_bound_ && std::this_thread::get_id() == main_thread_id_;
}

bool scheduler::is_worker_thread() const noexcept {
    const auto id = std::this_thread::get_id();
    for (const auto& wid : worker_ids_) {
        if (wid == id) {
            return true;
        }
    }
    return false;
}

thread_kind scheduler::current_thread_kind() const noexcept {
    if (is_main_thread()) {
        return thread_kind::main;
    }
    if (is_worker_thread()) {
        return thread_kind::worker;
    }
    return thread_kind::unknown;
}

status scheduler::enqueue_main_unlocked_(task_fn fn) {
    if (!fn) {
        return err(ec::invalid_argument, "null main task");
    }
    if (main_queue_.size() >= config_.max_queue_depth) {
        return err(LEAF_SCHED_QUEUE_FULL, "main queue full");
    }
    main_queue_.push_back(std::move(fn));
    return ok();
}

status scheduler::enqueue_async_unlocked_(task_fn fn) {
    if (!fn) {
        return err(ec::invalid_argument, "null async task");
    }
    if (async_queue_.size() >= config_.max_queue_depth) {
        return err(LEAF_SCHED_QUEUE_FULL, "async queue full");
    }
    async_queue_.push_back(std::move(fn));
    cv_.notify_one();
    return ok();
}

status scheduler::post_main(task_fn fn, bool inline_if_main) {
    if (!running_) {
        return err(ec::scheduler_shutdown, "scheduler stopped");
    }
    if (inline_if_main && is_main_thread()) {
        try {
            fn();
        } catch (...) {
            return err(ec::unknown, "main task threw");
        }
        return ok();
    }
    std::scoped_lock lock(mutex_);
    return enqueue_main_unlocked_(std::move(fn));
}

status scheduler::post_async(task_fn fn) {
    if (!running_) {
        return err(ec::scheduler_shutdown, "scheduler stopped");
    }
    std::scoped_lock lock(mutex_);
    return enqueue_async_unlocked_(std::move(fn));
}

result<task_handle> scheduler::delay(task_fn fn, clock::duration after) {
    if (!running_) {
        return err<task_handle>(ec::scheduler_shutdown, "scheduler stopped");
    }
    if (!fn) {
        return err<task_handle>(ec::invalid_argument, "null delay task");
    }
    if (after < clock::duration::zero()) {
        return err<task_handle>(LEAF_SCHED_INVALID_DELAY, "negative delay");
    }

    const auto id = next_id_++;
    timed_task t;
    t.id = id;
    t.when = clock::now() + after;
    t.repeat = false;
    t.on_main = true;
    t.fn = std::move(fn);

    {
        std::scoped_lock lock(mutex_);
        timed_.emplace(id, std::move(t));
    }
    return task_handle{this, id};
}

result<task_handle> scheduler::repeat(task_fn fn, clock::duration interval) {
    if (!running_) {
        return err<task_handle>(ec::scheduler_shutdown, "scheduler stopped");
    }
    if (!fn) {
        return err<task_handle>(ec::invalid_argument, "null repeat task");
    }
    if (interval <= clock::duration::zero()) {
        return err<task_handle>(LEAF_SCHED_INVALID_DELAY, "non-positive interval");
    }

    const auto id = next_id_++;
    timed_task t;
    t.id = id;
    t.when = clock::now() + interval;
    t.interval = interval;
    t.repeat = true;
    t.on_main = true;
    t.fn = std::move(fn);

    {
        std::scoped_lock lock(mutex_);
        timed_.emplace(id, std::move(t));
    }
    return task_handle{this, id};
}

status scheduler::cancel(task_id id) {
    std::scoped_lock lock(mutex_);
    auto it = timed_.find(id);
    if (it == timed_.end()) {
        return err(LEAF_SCHED_TASK_NOT_FOUND, "timed task not found");
    }
    it->second.cancelled = true;
    timed_.erase(it);
    return ok();
}

void scheduler::enqueue_due_timers_unlocked_(clock::time_point now) {
    std::vector<task_id> due;
    due.reserve(timed_.size());
    for (const auto& [id, t] : timed_) {
        if (!t.cancelled && t.when <= now) {
            due.push_back(id);
        }
    }
    for (task_id id : due) {
        auto it = timed_.find(id);
        if (it == timed_.end()) {
            continue;
        }
        auto task = std::move(it->second);
        timed_.erase(it);

        if (task.on_main) {
            (void)enqueue_main_unlocked_(task.fn);
        } else {
            (void)enqueue_async_unlocked_(task.fn);
        }

        if (task.repeat && !task.cancelled) {
            task.when = now + task.interval;
            timed_.emplace(id, std::move(task));
        }
    }
}

status scheduler::pump_main(std::size_t budget) {
    if (!running_) {
        return err(ec::scheduler_shutdown, "scheduler stopped");
    }
    if (main_bound_ && !is_main_thread()) {
        return err(ec::wrong_thread, "pump_main requires main thread");
    }

    std::vector<task_fn> batch;
    batch.reserve(budget);

    {
        std::scoped_lock lock(mutex_);
        enqueue_due_timers_unlocked_(clock::now());
        while (!main_queue_.empty() && batch.size() < budget) {
            batch.push_back(std::move(main_queue_.front()));
            main_queue_.pop_front();
        }
    }

    for (auto& fn : batch) {
        try {
            fn();
        } catch (...) {
            // Isolate task failures; continue draining.
        }
    }
    return ok();
}

std::size_t scheduler::main_queue_size() const {
    std::scoped_lock lock(mutex_);
    return main_queue_.size();
}

std::size_t scheduler::async_queue_size() const {
    std::scoped_lock lock(mutex_);
    return async_queue_.size();
}

void scheduler::worker_loop_(std::size_t index) {
    {
        std::scoped_lock lock(mutex_);
        if (index < worker_ids_.size()) {
            worker_ids_[index] = std::this_thread::get_id();
        }
    }

    while (running_.load()) {
        task_fn fn;
        {
            std::unique_lock lock(mutex_);
            cv_.wait(lock, [&] {
                return !running_.load() || !async_queue_.empty();
            });
            if (!running_.load() && async_queue_.empty()) {
                return;
            }
            if (async_queue_.empty()) {
                continue;
            }
            fn = std::move(async_queue_.front());
            async_queue_.pop_front();
        }
        try {
            fn();
        } catch (...) {
            // Isolated.
        }
    }
}

} // namespace leaf
