#pragma once

#include "leaf/abi/leaf_abi_v1.h"

#include <string_view>

namespace leaf::sdk {

/// Thin wrapper around {@link LeafStatus}.
enum class status : int {
    ok = LEAF_STATUS_OK,
    error = LEAF_STATUS_ERROR,
    invalid_argument = LEAF_STATUS_INVALID_ARGUMENT,
    not_found = LEAF_STATUS_NOT_FOUND,
    not_supported = LEAF_STATUS_NOT_SUPPORTED,
    wrong_thread = LEAF_STATUS_WRONG_THREAD,
    invalid_handle = LEAF_STATUS_INVALID_HANDLE,
    stale_handle = LEAF_STATUS_STALE_HANDLE,
    capability_missing = LEAF_STATUS_CAPABILITY_MISSING,
    abi_mismatch = LEAF_STATUS_ABI_MISMATCH,
};

[[nodiscard]] inline constexpr bool ok(status s) noexcept {
    return s == status::ok;
}

[[nodiscard]] inline constexpr status from_abi(LeafStatus s) noexcept {
    return static_cast<status>(static_cast<int>(s));
}

[[nodiscard]] inline constexpr LeafStatus to_abi(status s) noexcept {
    return static_cast<LeafStatus>(static_cast<int>(s));
}

[[nodiscard]] inline constexpr std::string_view status_name(status s) noexcept {
    switch (s) {
        case status::ok: return "OK";
        case status::error: return "ERROR";
        case status::invalid_argument: return "INVALID_ARGUMENT";
        case status::not_found: return "NOT_FOUND";
        case status::not_supported: return "NOT_SUPPORTED";
        case status::wrong_thread: return "WRONG_THREAD";
        case status::invalid_handle: return "INVALID_HANDLE";
        case status::stale_handle: return "STALE_HANDLE";
        case status::capability_missing: return "CAPABILITY_MISSING";
        case status::abi_mismatch: return "ABI_MISMATCH";
    }
    return "UNKNOWN";
}

} // namespace leaf::sdk
