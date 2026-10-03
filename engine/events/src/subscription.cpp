#include "leaf/events/subscription.hpp"

#include "leaf/events/event_runtime.hpp"

namespace leaf {

subscription::subscription(event_runtime* runtime, subscription_id id) noexcept
    : runtime_(runtime)
    , id_(id) {}

subscription::~subscription() {
    unsubscribe();
}

subscription::subscription(subscription&& other) noexcept
    : runtime_(other.runtime_)
    , id_(other.id_) {
    other.runtime_ = nullptr;
    other.id_ = 0;
}

subscription& subscription::operator=(subscription&& other) noexcept {
    if (this != &other) {
        unsubscribe();
        runtime_ = other.runtime_;
        id_ = other.id_;
        other.runtime_ = nullptr;
        other.id_ = 0;
    }
    return *this;
}

void subscription::unsubscribe() noexcept {
    if (runtime_ && id_ != 0) {
        (void)runtime_->unsubscribe(id_);
    }
    runtime_ = nullptr;
    id_ = 0;
}

} // namespace leaf
