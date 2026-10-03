#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

#include "leaf/abi/leaf_error_v1.h"

namespace leaf {

/// ABI-stable numeric error protocol: 0xSSMMEEEE.
using error_code = std::uint32_t;

inline constexpr error_code ok_code = LEAF_OK;

enum class error_severity : std::uint8_t {
    success = LEAF_SEVERITY_SUCCESS,
    info = LEAF_SEVERITY_INFO,
    error = LEAF_SEVERITY_ERROR,
    fatal = LEAF_SEVERITY_FATAL,
    panic = LEAF_SEVERITY_PANIC,
    warning = LEAF_SEVERITY_WARNING,
};

enum class error_module : std::uint8_t {
    core = LEAF_MODULE_CORE,
    mod_loader = LEAF_MODULE_MOD_LOADER,
    event = LEAF_MODULE_EVENT,
    scheduler = LEAF_MODULE_SCHEDULER,
    handle = LEAF_MODULE_HANDLE,
    abi = LEAF_MODULE_ABI,
    mapping = LEAF_MODULE_MAPPING,
    jvm_bridge = LEAF_MODULE_JVM_BRIDGE,
    fabric_bridge = LEAF_MODULE_FABRIC_BRIDGE,
    forge_bridge = LEAF_MODULE_FORGE_BRIDGE,
    neoforge_bridge = LEAF_MODULE_NEOFORGE_BRIDGE,
    resource = LEAF_MODULE_RESOURCE,
    network = LEAF_MODULE_NETWORK,
    config = LEAF_MODULE_CONFIG,
    dependency = LEAF_MODULE_DEPENDENCY,
    package = LEAF_MODULE_PACKAGE,
    platform = LEAF_MODULE_PLATFORM,
    filesystem = LEAF_MODULE_FILESYSTEM,
    memory = LEAF_MODULE_MEMORY,
    thread = LEAF_MODULE_THREAD,
    security = LEAF_MODULE_SECURITY,
    compatibility = LEAF_MODULE_COMPATIBILITY,
    kotlin_native = LEAF_MODULE_KOTLIN_NATIVE,
};

[[nodiscard]] constexpr error_code make_error_code(
    std::uint8_t severity,
    std::uint8_t module,
    std::uint16_t number) noexcept {
    return LEAF_MAKE_ERROR(severity, module, number);
}

[[nodiscard]] constexpr error_code make_error_code(
    error_severity severity,
    error_module module,
    std::uint16_t number) noexcept {
    return make_error_code(
        static_cast<std::uint8_t>(severity),
        static_cast<std::uint8_t>(module),
        number);
}

[[nodiscard]] constexpr std::uint8_t error_severity_of(error_code code) noexcept {
    return LEAF_ERROR_SEVERITY(code);
}

[[nodiscard]] constexpr std::uint8_t error_module_of(error_code code) noexcept {
    return LEAF_ERROR_MODULE(code);
}

[[nodiscard]] constexpr std::uint16_t error_number_of(error_code code) noexcept {
    return LEAF_ERROR_NUMBER(code);
}

[[nodiscard]] constexpr bool is_success(error_code code) noexcept {
    return code == ok_code;
}

[[nodiscard]] constexpr bool is_failure(error_code code) noexcept {
    return code != ok_code;
}

/// Official engine error constants (numeric ABI). Prefer these over inventing codes.
namespace ec {

inline constexpr error_code ok = LEAF_OK;

// Core
inline constexpr error_code unknown = LEAF_CORE_UNKNOWN;
inline constexpr error_code invalid_argument = LEAF_CORE_INVALID_ARGUMENT;
inline constexpr error_code out_of_range = LEAF_CORE_OUT_OF_RANGE;
inline constexpr error_code not_found = LEAF_CORE_NOT_FOUND;
inline constexpr error_code already_exists = LEAF_CORE_ALREADY_EXISTS;
inline constexpr error_code not_supported = LEAF_CORE_NOT_SUPPORTED;
inline constexpr error_code permission_denied = LEAF_CORE_PERMISSION_DENIED;
inline constexpr error_code timed_out = LEAF_CORE_TIMED_OUT;
inline constexpr error_code cancelled = LEAF_CORE_CANCELLED;
inline constexpr error_code exhausted = LEAF_CORE_EXHAUSTED;

// Handle
inline constexpr error_code invalid_handle = LEAF_HANDLE_INVALID;
inline constexpr error_code stale_handle = LEAF_HANDLE_EXPIRED;
inline constexpr error_code handle_type_mismatch = LEAF_HANDLE_TYPE_MISMATCH;
inline constexpr error_code handle_table_full = LEAF_HANDLE_TABLE_FULL;

// Mod loader
inline constexpr error_code mod_not_found = LEAF_MOD_NOT_LOADED;
inline constexpr error_code mod_manifest_invalid = LEAF_MOD_MANIFEST_INVALID;
inline constexpr error_code mod_dependency_unsatisfied = LEAF_MOD_DEPENDENCY_MISSING;
inline constexpr error_code mod_version_incompatible = LEAF_MOD_VERSION_UNSUPPORTED;
inline constexpr error_code mod_load_failed = LEAF_MOD_LOAD_FAILED;
inline constexpr error_code mod_entry_missing = LEAF_MOD_ENTRY_NOT_FOUND;
inline constexpr error_code mod_already_loaded = LEAF_MOD_ALREADY_LOADED;
inline constexpr error_code mod_state_invalid = LEAF_MOD_STATE_INVALID;
inline constexpr error_code mod_dependency_cycle = LEAF_MOD_DEPENDENCY_CONFLICT;
inline constexpr error_code mod_duplicate_id = LEAF_MOD_DUPLICATE_ID;

inline constexpr error_code capability_missing = LEAF_ABI_CAPABILITY_MISSING;
inline constexpr error_code abi_version_mismatch = LEAF_MOD_ABI_MISMATCH;
inline constexpr error_code symbol_not_resolved = LEAF_MAPPING_SYMBOL_NOT_FOUND;
inline constexpr error_code mapping_not_found = LEAF_MAPPING_SYMBOL_NOT_FOUND;

inline constexpr error_code wrong_thread = LEAF_SCHED_WRONG_THREAD;
inline constexpr error_code scheduler_shutdown = LEAF_SCHED_SHUTDOWN;
inline constexpr error_code scheduler_queue_full = LEAF_SCHED_QUEUE_FULL;
inline constexpr error_code scheduler_task_not_found = LEAF_SCHED_TASK_NOT_FOUND;
inline constexpr error_code scheduler_invalid_delay = LEAF_SCHED_INVALID_DELAY;

inline constexpr error_code event_not_found = LEAF_EVENT_NOT_REGISTERED;
inline constexpr error_code event_schema_mismatch = LEAF_EVENT_SCHEMA_UNSUPPORTED;
inline constexpr error_code event_wrong_kind = LEAF_EVENT_DISPATCH_FORBIDDEN;
inline constexpr error_code event_recursive_limit = LEAF_EVENT_RECURSION_LIMIT;
inline constexpr error_code subscription_invalid = LEAF_EVENT_INVALID_SUBSCRIPTION;
inline constexpr error_code event_monitor_mutation = LEAF_EVENT_MONITOR_MUTATION;
inline constexpr error_code event_callback_failed = LEAF_EVENT_CALLBACK_FAILED;

inline constexpr error_code io_error = LEAF_FS_IO_ERROR;
inline constexpr error_code path_not_found = LEAF_FS_PATH_NOT_FOUND;
inline constexpr error_code resource_corrupt = LEAF_RESOURCE_CORRUPT;

} // namespace ec

struct error_info {
    error_code code{ok_code};
    std::string_view symbol{"LEAF_OK"};
    std::string_view message{"Success."};
    std::string_view module_name{"core"};
};

[[nodiscard]] error_info lookup_error(error_code code) noexcept;

[[nodiscard]] std::string_view error_symbol(error_code code) noexcept;
[[nodiscard]] std::string_view error_message(error_code code) noexcept;
[[nodiscard]] std::string_view module_name(error_code code) noexcept;

[[nodiscard]] std::string format_error_code(error_code code);

/// Rich error for std::expected. Message is NOT ABI-stable — code is.
class error {
public:
    error() = default;

    explicit error(error_code code, std::string message = {})
        : code_(code)
        , message_(std::move(message)) {}

    [[nodiscard]] error_code code() const noexcept { return code_; }

    [[nodiscard]] std::string_view message() const noexcept {
        if (!message_.empty()) {
            return message_;
        }
        return error_message(code_);
    }

    [[nodiscard]] bool empty_message() const noexcept { return message_.empty(); }

    [[nodiscard]] std::string format() const {
        std::string out = format_error_code(code_);
        out += ' ';
        out += error_symbol(code_);
        const auto msg = message();
        if (!msg.empty()) {
            out += ": ";
            out += msg;
        }
        return out;
    }

    [[nodiscard]] friend bool operator==(const error& a, const error& b) noexcept {
        return a.code_ == b.code_ && a.message_ == b.message_;
    }

private:
    error_code code_{ec::unknown};
    std::string message_;
};

[[nodiscard]] inline error make_error(error_code code, std::string message = {}) {
    return error{code, std::move(message)};
}

} // namespace leaf
