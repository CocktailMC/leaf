#pragma once

#include "leaf/abi/leaf_abi_v1.h"
#include "leaf/core/error.hpp"

namespace leaf {

[[nodiscard]] constexpr LeafStatus to_abi_status(error_code code) noexcept {
    if (code == ok_code) {
        return LEAF_STATUS_OK;
    }
    switch (code) {
        case ec::invalid_argument:
            return LEAF_STATUS_INVALID_ARGUMENT;
        case ec::not_found:
        case ec::event_not_found:
        case ec::path_not_found:
        case ec::mod_not_found:
            return LEAF_STATUS_NOT_FOUND;
        case ec::not_supported:
        case ec::event_wrong_kind:
            return LEAF_STATUS_NOT_SUPPORTED;
        case ec::wrong_thread:
            return LEAF_STATUS_WRONG_THREAD;
        case ec::invalid_handle:
            return LEAF_STATUS_INVALID_HANDLE;
        case ec::stale_handle:
            return LEAF_STATUS_STALE_HANDLE;
        case ec::capability_missing:
            return LEAF_STATUS_CAPABILITY_MISSING;
        case ec::abi_version_mismatch:
            return LEAF_STATUS_ABI_MISMATCH;
        default:
            return LEAF_STATUS_ERROR;
    }
}

} // namespace leaf
