#include "leaf/core/error.hpp"

#include <cstdio>

namespace leaf {
namespace {

struct entry {
    error_code code;
    std::string_view symbol;
    std::string_view message;
    std::string_view module_name;
};

constexpr entry k_table[] = {
    {LEAF_OK, "LEAF_OK", "Success.", "core"},

    {LEAF_CORE_UNKNOWN, "LEAF_CORE_UNKNOWN", "Unknown error.", "core"},
    {LEAF_CORE_INVALID_ARGUMENT, "LEAF_CORE_INVALID_ARGUMENT", "Invalid argument.", "core"},
    {LEAF_CORE_OUT_OF_RANGE, "LEAF_CORE_OUT_OF_RANGE", "Value out of range.", "core"},
    {LEAF_CORE_NOT_FOUND, "LEAF_CORE_NOT_FOUND", "Resource not found.", "core"},
    {LEAF_CORE_ALREADY_EXISTS, "LEAF_CORE_ALREADY_EXISTS", "Resource already exists.", "core"},
    {LEAF_CORE_NOT_SUPPORTED, "LEAF_CORE_NOT_SUPPORTED", "Operation not supported.", "core"},
    {LEAF_CORE_PERMISSION_DENIED, "LEAF_CORE_PERMISSION_DENIED", "Permission denied.", "core"},
    {LEAF_CORE_TIMED_OUT, "LEAF_CORE_TIMED_OUT", "Operation timed out.", "core"},
    {LEAF_CORE_CANCELLED, "LEAF_CORE_CANCELLED", "Operation cancelled.", "core"},
    {LEAF_CORE_EXHAUSTED, "LEAF_CORE_EXHAUSTED", "Resource exhausted.", "core"},
    {LEAF_CORE_INIT_FAILED, "LEAF_CORE_INIT_FAILED", "Initialization failed.", "core"},

    {LEAF_MOD_MANIFEST_NOT_FOUND, "LEAF_MOD_MANIFEST_NOT_FOUND", "Mod manifest not found.", "mod_loader"},
    {LEAF_MOD_MANIFEST_INVALID, "LEAF_MOD_MANIFEST_INVALID", "Mod manifest invalid.", "mod_loader"},
    {LEAF_MOD_ENTRY_NOT_FOUND, "LEAF_MOD_ENTRY_NOT_FOUND", "Mod entry symbol not found.", "mod_loader"},
    {LEAF_MOD_DEPENDENCY_MISSING, "LEAF_MOD_DEPENDENCY_MISSING", "Required mod dependency missing.", "mod_loader"},
    {LEAF_MOD_DEPENDENCY_CONFLICT, "LEAF_MOD_DEPENDENCY_CONFLICT", "Mod dependency conflict or cycle.", "mod_loader"},
    {LEAF_MOD_VERSION_UNSUPPORTED, "LEAF_MOD_VERSION_UNSUPPORTED", "Mod version unsupported.", "mod_loader"},
    {LEAF_MOD_PLATFORM_UNSUPPORTED, "LEAF_MOD_PLATFORM_UNSUPPORTED", "Mod platform unsupported.", "mod_loader"},
    {LEAF_MOD_LOAD_FAILED, "LEAF_MOD_LOAD_FAILED", "Mod failed to load.", "mod_loader"},
    {LEAF_MOD_ALREADY_LOADED, "LEAF_MOD_ALREADY_LOADED", "Mod already loaded.", "mod_loader"},
    {LEAF_MOD_NOT_LOADED, "LEAF_MOD_NOT_LOADED", "Mod is not loaded.", "mod_loader"},
    {LEAF_MOD_SYMBOL_MISSING, "LEAF_MOD_SYMBOL_MISSING", "Required native symbol missing.", "mod_loader"},
    {LEAF_MOD_ABI_MISMATCH, "LEAF_MOD_ABI_MISMATCH", "Mod ABI mismatch.", "mod_loader"},
    {LEAF_MOD_STATE_INVALID, "LEAF_MOD_STATE_INVALID", "Invalid mod lifecycle state.", "mod_loader"},
    {LEAF_MOD_DUPLICATE_ID, "LEAF_MOD_DUPLICATE_ID", "Duplicate mod id.", "mod_loader"},

    {LEAF_EVENT_INVALID_ID, "LEAF_EVENT_INVALID_ID", "Invalid event identifier.", "event"},
    {LEAF_EVENT_SCHEMA_UNSUPPORTED, "LEAF_EVENT_SCHEMA_UNSUPPORTED", "Unsupported event schema version.", "event"},
    {LEAF_EVENT_INVALID_SUBSCRIPTION, "LEAF_EVENT_INVALID_SUBSCRIPTION", "Invalid subscription handle.", "event"},
    {LEAF_EVENT_SUBSCRIPTION_INACTIVE, "LEAF_EVENT_SUBSCRIPTION_INACTIVE", "Subscription is inactive.", "event"},
    {LEAF_EVENT_THREAD_VIOLATION, "LEAF_EVENT_THREAD_VIOLATION", "Event dispatch thread violation.", "event"},
    {LEAF_EVENT_RECURSION_LIMIT, "LEAF_EVENT_RECURSION_LIMIT", "Recursive event dispatch limit exceeded.", "event"},
    {LEAF_EVENT_INVALID_DECISION, "LEAF_EVENT_INVALID_DECISION", "Invalid decision value.", "event"},
    {LEAF_EVENT_PAYLOAD_INVALID, "LEAF_EVENT_PAYLOAD_INVALID", "Event payload invalid or corrupted.", "event"},
    {LEAF_EVENT_OWNER_UNLOADED, "LEAF_EVENT_OWNER_UNLOADED", "Event owner mod has been unloaded.", "event"},
    {LEAF_EVENT_DYNAMIC_ID_CONFLICT, "LEAF_EVENT_DYNAMIC_ID_CONFLICT", "Dynamic event id conflict.", "event"},
    {LEAF_EVENT_ALREADY_REGISTERED, "LEAF_EVENT_ALREADY_REGISTERED", "Event already registered.", "event"},
    {LEAF_EVENT_NOT_REGISTERED, "LEAF_EVENT_NOT_REGISTERED", "Event is not registered.", "event"},
    {LEAF_EVENT_DISPATCH_FORBIDDEN, "LEAF_EVENT_DISPATCH_FORBIDDEN", "Event dispatch forbidden for this kind/path.", "event"},
    {LEAF_EVENT_MONITOR_MUTATION, "LEAF_EVENT_MONITOR_MUTATION", "Monitor listeners cannot mutate decisions.", "event"},
    {LEAF_EVENT_CALLBACK_FAILED, "LEAF_EVENT_CALLBACK_FAILED", "Event listener callback failed.", "event"},

    {LEAF_SCHED_WRONG_THREAD, "LEAF_SCHED_WRONG_THREAD", "Operation called on the wrong thread.", "scheduler"},
    {LEAF_SCHED_SHUTDOWN, "LEAF_SCHED_SHUTDOWN", "Scheduler has shut down.", "scheduler"},
    {LEAF_SCHED_QUEUE_FULL, "LEAF_SCHED_QUEUE_FULL", "Scheduler queue is full.", "scheduler"},
    {LEAF_SCHED_TASK_NOT_FOUND, "LEAF_SCHED_TASK_NOT_FOUND", "Scheduler task not found.", "scheduler"},
    {LEAF_SCHED_INVALID_DELAY, "LEAF_SCHED_INVALID_DELAY", "Invalid scheduler delay.", "scheduler"},

    {LEAF_HANDLE_INVALID, "LEAF_HANDLE_INVALID", "Invalid handle.", "handle"},
    {LEAF_HANDLE_EXPIRED, "LEAF_HANDLE_EXPIRED", "Handle has expired.", "handle"},
    {LEAF_HANDLE_TYPE_MISMATCH, "LEAF_HANDLE_TYPE_MISMATCH", "Handle type mismatch.", "handle"},
    {LEAF_HANDLE_GENERATION_MISMATCH, "LEAF_HANDLE_GENERATION_MISMATCH", "Handle generation mismatch.", "handle"},
    {LEAF_HANDLE_OWNER_INVALID, "LEAF_HANDLE_OWNER_INVALID", "Handle owner invalid.", "handle"},
    {LEAF_HANDLE_TABLE_FULL, "LEAF_HANDLE_TABLE_FULL", "Handle table capacity exhausted.", "handle"},

    {LEAF_ABI_CAPABILITY_MISSING, "LEAF_ABI_CAPABILITY_MISSING", "Required capability is missing.", "abi"},
    {LEAF_ABI_VERSION_MISMATCH, "LEAF_ABI_VERSION_MISMATCH", "ABI version mismatch.", "abi"},

    {LEAF_MAPPING_SYMBOL_NOT_FOUND, "LEAF_MAPPING_SYMBOL_NOT_FOUND", "Mapping symbol not found.", "mapping"},
    {LEAF_FS_IO_ERROR, "LEAF_FS_IO_ERROR", "Filesystem I/O error.", "filesystem"},
    {LEAF_FS_PATH_NOT_FOUND, "LEAF_FS_PATH_NOT_FOUND", "Filesystem path not found.", "filesystem"},
    {LEAF_RESOURCE_CORRUPT, "LEAF_RESOURCE_CORRUPT", "Resource is corrupt.", "resource"},
};

} // namespace

error_info lookup_error(error_code code) noexcept {
    for (const auto& e : k_table) {
        if (e.code == code) {
            return error_info{e.code, e.symbol, e.message, e.module_name};
        }
    }
    return error_info{
        LEAF_CORE_UNKNOWN,
        "LEAF_CORE_UNKNOWN",
        "Unknown error.",
        "core",
    };
}

std::string_view error_symbol(error_code code) noexcept {
    // Lifetime: string_views point at static table literals.
    for (const auto& e : k_table) {
        if (e.code == code) {
            return e.symbol;
        }
    }
    return "LEAF_CORE_UNKNOWN";
}

std::string_view error_message(error_code code) noexcept {
    for (const auto& e : k_table) {
        if (e.code == code) {
            return e.message;
        }
    }
    return "Unknown error.";
}

std::string_view module_name(error_code code) noexcept {
    if (code == LEAF_OK) {
        return "core";
    }
    switch (error_module_of(code)) {
        case LEAF_MODULE_CORE: return "core";
        case LEAF_MODULE_MOD_LOADER: return "mod_loader";
        case LEAF_MODULE_EVENT: return "event";
        case LEAF_MODULE_SCHEDULER: return "scheduler";
        case LEAF_MODULE_HANDLE: return "handle";
        case LEAF_MODULE_ABI: return "abi";
        case LEAF_MODULE_MAPPING: return "mapping";
        case LEAF_MODULE_JVM_BRIDGE: return "jvm_bridge";
        case LEAF_MODULE_FABRIC_BRIDGE: return "fabric_bridge";
        case LEAF_MODULE_FORGE_BRIDGE: return "forge_bridge";
        case LEAF_MODULE_NEOFORGE_BRIDGE: return "neoforge_bridge";
        case LEAF_MODULE_RESOURCE: return "resource";
        case LEAF_MODULE_NETWORK: return "network";
        case LEAF_MODULE_CONFIG: return "config";
        case LEAF_MODULE_DEPENDENCY: return "dependency";
        case LEAF_MODULE_PACKAGE: return "package";
        case LEAF_MODULE_PLATFORM: return "platform";
        case LEAF_MODULE_FILESYSTEM: return "filesystem";
        case LEAF_MODULE_MEMORY: return "memory";
        case LEAF_MODULE_THREAD: return "thread";
        case LEAF_MODULE_SECURITY: return "security";
        case LEAF_MODULE_COMPATIBILITY: return "compatibility";
        case LEAF_MODULE_KOTLIN_NATIVE: return "kotlin_native";
        default: return "unknown";
    }
}

std::string format_error_code(error_code code) {
    char buf[16];
    std::snprintf(buf, sizeof(buf), "0x%08X", static_cast<unsigned>(code));
    return std::string{buf};
}

} // namespace leaf
