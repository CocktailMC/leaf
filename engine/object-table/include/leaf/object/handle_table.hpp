#pragma once

#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "leaf/core/result.hpp"
#include "leaf/object/handle.hpp"

namespace leaf {

/// Thread-safe generational handle table.
///
/// `Tag` is the public handle tag (e.g. `player_tag`).
/// `T` is the engine-owned payload (JNI GlobalRef wrapper, native object, …).
///
/// Layout of a non-null raw handle:
///   [63:32] generation
///   [31:0]  slot index (1-based; 0 is reserved for null)
///
/// When a slot is freed, its generation is incremented so stale handles fail
/// lookup with ec::stale_handle instead of silently aliasing.
template <typename Tag, typename T>
class handle_table {
public:
    using handle_type = handle<Tag>;
    using value_type = T;

    explicit handle_table(std::size_t initial_capacity = 64) {
        slots_.reserve(initial_capacity);
        free_list_.reserve(initial_capacity);
    }

    handle_table(const handle_table&) = delete;
    handle_table& operator=(const handle_table&) = delete;
    handle_table(handle_table&&) = delete;
    handle_table& operator=(handle_table&&) = delete;

    /// Insert a value and return a typed handle. Generation starts at 1.
    [[nodiscard]] result<handle_type> create(T value) {
        std::scoped_lock lock(mutex_);

        std::uint32_t index = 0;
        std::uint32_t generation = 1;

        if (!free_list_.empty()) {
            index = free_list_.back();
            free_list_.pop_back();
            auto& entry = slots_[index - 1];
            generation = entry.generation;
            entry.occupied = true;
            entry.value = std::move(value);
        } else {
            if (slots_.size() >= max_slots_) {
                return err<handle_type>(
                    ec::handle_table_full,
                    "handle table capacity exhausted");
            }
            slots_.push_back(slot_entry{
                .value = std::move(value),
                .generation = 1,
                .occupied = true,
            });
            index = static_cast<std::uint32_t>(slots_.size());
            generation = 1;
        }

        return handle_type{pack_handle_bits(index, generation)};
    }

    [[nodiscard]] result<std::reference_wrapper<T>> get(handle_type h) {
        std::scoped_lock lock(mutex_);
        auto* entry = find_slot(h);
        if (!entry) {
            return err<std::reference_wrapper<T>>(
                last_lookup_error_,
                last_lookup_message_);
        }
        return std::ref(entry->value);
    }

    [[nodiscard]] result<std::reference_wrapper<const T>> get(handle_type h) const {
        std::scoped_lock lock(mutex_);
        const auto* entry = find_slot(h);
        if (!entry) {
            return err<std::reference_wrapper<const T>>(
                last_lookup_error_,
                last_lookup_message_);
        }
        return std::cref(entry->value);
    }

    /// Destroy the slot. Further use of the same raw bits yields stale_handle.
    [[nodiscard]] status destroy(handle_type h) {
        std::scoped_lock lock(mutex_);
        auto* entry = find_slot(h);
        if (!entry) {
            return err(last_lookup_error_, last_lookup_message_);
        }

        entry->occupied = false;
        entry->value = T{};
        // Wrap generation carefully; 0 is reserved / invalid in packed form.
        if (entry->generation == UINT32_MAX) {
            entry->generation = 1;
        } else {
            ++entry->generation;
        }

        const auto index = handle_index(h.raw());
        free_list_.push_back(index);
        return ok();
    }

    [[nodiscard]] bool contains(handle_type h) const {
        std::scoped_lock lock(mutex_);
        return find_slot(h) != nullptr;
    }

    [[nodiscard]] std::size_t size() const {
        std::scoped_lock lock(mutex_);
        return slots_.size() - free_list_.size();
    }

    [[nodiscard]] std::size_t capacity_slots() const {
        std::scoped_lock lock(mutex_);
        return slots_.size();
    }

    void clear() {
        std::scoped_lock lock(mutex_);
        slots_.clear();
        free_list_.clear();
    }

private:
    struct slot_entry {
        T value{};
        std::uint32_t generation{1};
        bool occupied{false};
    };

    static constexpr std::size_t max_slots_ = 0x7FFFFFFFu;

    mutable std::mutex mutex_;
    std::vector<slot_entry> slots_;
    std::vector<std::uint32_t> free_list_;
    mutable error_code last_lookup_error_{ec::invalid_handle};
    mutable std::string last_lookup_message_;

    [[nodiscard]] slot_entry* find_slot(handle_type h) {
        return const_cast<slot_entry*>(std::as_const(*this).find_slot(h));
    }

    [[nodiscard]] const slot_entry* find_slot(handle_type h) const {
        if (!h.valid()) {
            last_lookup_error_ = ec::invalid_handle;
            last_lookup_message_ = "null handle";
            return nullptr;
        }

        const auto index = handle_index(h.raw());
        const auto generation = handle_generation(h.raw());

        if (index == 0 || index > slots_.size()) {
            last_lookup_error_ = ec::invalid_handle;
            last_lookup_message_ = "handle index out of range";
            return nullptr;
        }

        const auto& entry = slots_[index - 1];
        if (!entry.occupied) {
            last_lookup_error_ = ec::stale_handle;
            last_lookup_message_ = "handle slot is free";
            return nullptr;
        }
        if (entry.generation != generation) {
            last_lookup_error_ = ec::stale_handle;
            last_lookup_message_ = "handle generation mismatch";
            return nullptr;
        }
        return &entry;
    }
};

} // namespace leaf
