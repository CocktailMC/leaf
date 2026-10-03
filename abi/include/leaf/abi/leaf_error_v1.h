/* Leaf Error Code ABI v1 — 0xSSMMEEEE
 *
 * SS   = Severity
 * MM   = Module ID (never reuse)
 * EEEE = Module-local error number (0x0000 reserved)
 *
 * Only the numeric code is a long-term ABI contract.
 * Human messages and symbols may improve over time.
 */

#ifndef LEAF_ERROR_V1_H
#define LEAF_ERROR_V1_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t leaf_error_code;

#define LEAF_OK 0x00000000u

/* Severity (SS) */
#define LEAF_SEVERITY_SUCCESS 0x00u
#define LEAF_SEVERITY_INFO    0x01u
#define LEAF_SEVERITY_ERROR   0x02u
#define LEAF_SEVERITY_FATAL   0x03u
#define LEAF_SEVERITY_PANIC   0x04u
#define LEAF_SEVERITY_WARNING 0x05u /* log/diag only; avoid as API failure */

/* Module (MM) — permanently allocated */
#define LEAF_MODULE_CORE            0x01u
#define LEAF_MODULE_MOD_LOADER      0x02u
#define LEAF_MODULE_EVENT           0x03u
#define LEAF_MODULE_SCHEDULER       0x04u
#define LEAF_MODULE_HANDLE          0x05u
#define LEAF_MODULE_ABI             0x06u
#define LEAF_MODULE_MAPPING         0x07u
#define LEAF_MODULE_JVM_BRIDGE      0x08u
#define LEAF_MODULE_FABRIC_BRIDGE   0x09u
#define LEAF_MODULE_FORGE_BRIDGE    0x0Au
#define LEAF_MODULE_NEOFORGE_BRIDGE 0x0Bu
#define LEAF_MODULE_RESOURCE        0x0Cu
#define LEAF_MODULE_NETWORK         0x0Du
#define LEAF_MODULE_CONFIG          0x0Eu
#define LEAF_MODULE_DEPENDENCY      0x0Fu
#define LEAF_MODULE_PACKAGE         0x10u
#define LEAF_MODULE_PLATFORM        0x11u
#define LEAF_MODULE_FILESYSTEM      0x12u
#define LEAF_MODULE_MEMORY          0x13u
#define LEAF_MODULE_THREAD          0x14u
#define LEAF_MODULE_SECURITY        0x15u
#define LEAF_MODULE_COMPATIBILITY   0x16u
#define LEAF_MODULE_KOTLIN_NATIVE   0x17u

#if defined(__cplusplus)
#  define LEAF_MAKE_ERROR(severity, module, number) \
    (static_cast<leaf_error_code>( \
        ((static_cast<leaf_error_code>(severity) & 0xFFu) << 24) | \
        ((static_cast<leaf_error_code>(module) & 0xFFu) << 16) | \
        (static_cast<leaf_error_code>(number) & 0xFFFFu)))
#  define LEAF_ERROR_SEVERITY(code) \
    (static_cast<uint8_t>((static_cast<leaf_error_code>(code) >> 24) & 0xFFu))
#  define LEAF_ERROR_MODULE(code) \
    (static_cast<uint8_t>((static_cast<leaf_error_code>(code) >> 16) & 0xFFu))
#  define LEAF_ERROR_NUMBER(code) \
    (static_cast<uint16_t>(static_cast<leaf_error_code>(code) & 0xFFFFu))
#else
#  define LEAF_MAKE_ERROR(severity, module, number) \
    ((leaf_error_code)( \
        (((leaf_error_code)(severity) & 0xFFu) << 24) | \
        (((leaf_error_code)(module) & 0xFFu) << 16) | \
        ((leaf_error_code)(number) & 0xFFFFu)))
#  define LEAF_ERROR_SEVERITY(code) \
    ((uint8_t)(((leaf_error_code)(code) >> 24) & 0xFFu))
#  define LEAF_ERROR_MODULE(code) \
    ((uint8_t)(((leaf_error_code)(code) >> 16) & 0xFFu))
#  define LEAF_ERROR_NUMBER(code) \
    ((uint16_t)((leaf_error_code)(code) & 0xFFFFu))
#endif

/* ---- CORE (0x01) ---- */
#define LEAF_CORE_UNKNOWN            LEAF_MAKE_ERROR(0x02, 0x01, 0x0001)
#define LEAF_CORE_INVALID_ARGUMENT   LEAF_MAKE_ERROR(0x02, 0x01, 0x0002)
#define LEAF_CORE_OUT_OF_RANGE       LEAF_MAKE_ERROR(0x02, 0x01, 0x0003)
#define LEAF_CORE_NOT_FOUND          LEAF_MAKE_ERROR(0x02, 0x01, 0x0004)
#define LEAF_CORE_ALREADY_EXISTS     LEAF_MAKE_ERROR(0x02, 0x01, 0x0005)
#define LEAF_CORE_NOT_SUPPORTED      LEAF_MAKE_ERROR(0x02, 0x01, 0x0006)
#define LEAF_CORE_PERMISSION_DENIED  LEAF_MAKE_ERROR(0x02, 0x01, 0x0007)
#define LEAF_CORE_TIMED_OUT          LEAF_MAKE_ERROR(0x02, 0x01, 0x0008)
#define LEAF_CORE_CANCELLED          LEAF_MAKE_ERROR(0x02, 0x01, 0x0009)
#define LEAF_CORE_EXHAUSTED          LEAF_MAKE_ERROR(0x02, 0x01, 0x000A)
#define LEAF_CORE_INIT_FAILED        LEAF_MAKE_ERROR(0x02, 0x01, 0x000B)

#define LEAF_PANIC_CORE_STATE_CORRUPTED   LEAF_MAKE_ERROR(0x04, 0x01, 0x0001)
#define LEAF_PANIC_CORE_INTERNAL_INVARIANT LEAF_MAKE_ERROR(0x04, 0x01, 0x0002)

/* ---- MOD_LOADER (0x02) ---- */
#define LEAF_MOD_MANIFEST_NOT_FOUND      LEAF_MAKE_ERROR(0x02, 0x02, 0x0001)
#define LEAF_MOD_MANIFEST_INVALID        LEAF_MAKE_ERROR(0x02, 0x02, 0x0002)
#define LEAF_MOD_ENTRY_NOT_FOUND         LEAF_MAKE_ERROR(0x02, 0x02, 0x0003)
#define LEAF_MOD_DEPENDENCY_MISSING      LEAF_MAKE_ERROR(0x02, 0x02, 0x0004)
#define LEAF_MOD_DEPENDENCY_CONFLICT     LEAF_MAKE_ERROR(0x02, 0x02, 0x0005)
#define LEAF_MOD_VERSION_UNSUPPORTED     LEAF_MAKE_ERROR(0x02, 0x02, 0x0006)
#define LEAF_MOD_PLATFORM_UNSUPPORTED    LEAF_MAKE_ERROR(0x02, 0x02, 0x0007)
#define LEAF_MOD_LOAD_FAILED             LEAF_MAKE_ERROR(0x02, 0x02, 0x0008)
#define LEAF_MOD_ALREADY_LOADED          LEAF_MAKE_ERROR(0x02, 0x02, 0x0009)
#define LEAF_MOD_NOT_LOADED              LEAF_MAKE_ERROR(0x02, 0x02, 0x000A)
#define LEAF_MOD_SYMBOL_MISSING          LEAF_MAKE_ERROR(0x02, 0x02, 0x000B)
#define LEAF_MOD_ABI_MISMATCH            LEAF_MAKE_ERROR(0x02, 0x02, 0x000C)
#define LEAF_MOD_STATE_INVALID           LEAF_MAKE_ERROR(0x02, 0x02, 0x000D)
#define LEAF_MOD_DUPLICATE_ID            LEAF_MAKE_ERROR(0x02, 0x02, 0x000E)

/* ---- EVENT (0x03) ---- */
#define LEAF_EVENT_INVALID_ID            LEAF_MAKE_ERROR(0x02, 0x03, 0x0001)
#define LEAF_EVENT_SCHEMA_UNSUPPORTED    LEAF_MAKE_ERROR(0x02, 0x03, 0x0002)
#define LEAF_EVENT_INVALID_SUBSCRIPTION  LEAF_MAKE_ERROR(0x02, 0x03, 0x0003)
#define LEAF_EVENT_SUBSCRIPTION_INACTIVE LEAF_MAKE_ERROR(0x02, 0x03, 0x0004)
#define LEAF_EVENT_THREAD_VIOLATION      LEAF_MAKE_ERROR(0x02, 0x03, 0x0005)
#define LEAF_EVENT_RECURSION_LIMIT       LEAF_MAKE_ERROR(0x02, 0x03, 0x0006)
#define LEAF_EVENT_INVALID_DECISION      LEAF_MAKE_ERROR(0x02, 0x03, 0x0007)
#define LEAF_EVENT_PAYLOAD_INVALID       LEAF_MAKE_ERROR(0x02, 0x03, 0x0008)
#define LEAF_EVENT_OWNER_UNLOADED        LEAF_MAKE_ERROR(0x02, 0x03, 0x0009)
#define LEAF_EVENT_DYNAMIC_ID_CONFLICT   LEAF_MAKE_ERROR(0x02, 0x03, 0x000A)
#define LEAF_EVENT_ALREADY_REGISTERED    LEAF_MAKE_ERROR(0x02, 0x03, 0x000B)
#define LEAF_EVENT_NOT_REGISTERED        LEAF_MAKE_ERROR(0x02, 0x03, 0x000C)
#define LEAF_EVENT_DISPATCH_FORBIDDEN    LEAF_MAKE_ERROR(0x02, 0x03, 0x000D)
#define LEAF_EVENT_MONITOR_MUTATION      LEAF_MAKE_ERROR(0x02, 0x03, 0x000E)
#define LEAF_EVENT_CALLBACK_FAILED       LEAF_MAKE_ERROR(0x02, 0x03, 0x000F)

#define LEAF_EVENT_REGISTRY_CORRUPTED        LEAF_MAKE_ERROR(0x03, 0x03, 0x0001)
#define LEAF_EVENT_DISPATCH_STATE_CORRUPTED  LEAF_MAKE_ERROR(0x03, 0x03, 0x0002)
#define LEAF_EVENT_INTERNAL_INVARIANT        LEAF_MAKE_ERROR(0x03, 0x03, 0x0003)

/* ---- SCHEDULER (0x04) ---- */
#define LEAF_SCHED_WRONG_THREAD      LEAF_MAKE_ERROR(0x02, 0x04, 0x0001)
#define LEAF_SCHED_SHUTDOWN          LEAF_MAKE_ERROR(0x02, 0x04, 0x0002)
#define LEAF_SCHED_QUEUE_FULL        LEAF_MAKE_ERROR(0x02, 0x04, 0x0003)
#define LEAF_SCHED_TASK_NOT_FOUND    LEAF_MAKE_ERROR(0x02, 0x04, 0x0004)
#define LEAF_SCHED_INVALID_DELAY     LEAF_MAKE_ERROR(0x02, 0x04, 0x0005)

/* ---- HANDLE (0x05) ---- */
#define LEAF_HANDLE_INVALID              LEAF_MAKE_ERROR(0x02, 0x05, 0x0001)
#define LEAF_HANDLE_EXPIRED              LEAF_MAKE_ERROR(0x02, 0x05, 0x0002)
#define LEAF_HANDLE_TYPE_MISMATCH        LEAF_MAKE_ERROR(0x02, 0x05, 0x0003)
#define LEAF_HANDLE_GENERATION_MISMATCH  LEAF_MAKE_ERROR(0x02, 0x05, 0x0004)
#define LEAF_HANDLE_OWNER_INVALID        LEAF_MAKE_ERROR(0x02, 0x05, 0x0005)
#define LEAF_HANDLE_TABLE_FULL           LEAF_MAKE_ERROR(0x02, 0x05, 0x0006)

/* ---- ABI (0x06) ---- */
#define LEAF_ABI_CAPABILITY_MISSING  LEAF_MAKE_ERROR(0x02, 0x06, 0x0001)
#define LEAF_ABI_VERSION_MISMATCH    LEAF_MAKE_ERROR(0x02, 0x06, 0x0002)

/* ---- MAPPING (0x07) ---- */
#define LEAF_MAPPING_SYMBOL_NOT_FOUND        LEAF_MAKE_ERROR(0x02, 0x07, 0x0001)
#define LEAF_MAPPING_VERSION_NOT_FOUND       LEAF_MAKE_ERROR(0x02, 0x07, 0x0002)
#define LEAF_MAPPING_NAMESPACE_UNSUPPORTED   LEAF_MAKE_ERROR(0x02, 0x07, 0x0003)
#define LEAF_MAPPING_DESCRIPTOR_MISMATCH     LEAF_MAKE_ERROR(0x02, 0x07, 0x0004)
#define LEAF_MAPPING_DATABASE_CORRUPTED      LEAF_MAKE_ERROR(0x03, 0x07, 0x0005)

/* ---- JVM_BRIDGE (0x08) ---- */
#define LEAF_JVM_NOT_FOUND            LEAF_MAKE_ERROR(0x02, 0x08, 0x0001)
#define LEAF_JNI_ENV_UNAVAILABLE      LEAF_MAKE_ERROR(0x02, 0x08, 0x0002)
#define LEAF_JAVA_CLASS_NOT_FOUND     LEAF_MAKE_ERROR(0x02, 0x08, 0x0003)
#define LEAF_JAVA_METHOD_NOT_FOUND    LEAF_MAKE_ERROR(0x02, 0x08, 0x0004)
#define LEAF_JAVA_FIELD_NOT_FOUND     LEAF_MAKE_ERROR(0x02, 0x08, 0x0005)
#define LEAF_JAVA_EXCEPTION           LEAF_MAKE_ERROR(0x02, 0x08, 0x0006)
#define LEAF_JNI_GLOBAL_REF_FAILED    LEAF_MAKE_ERROR(0x02, 0x08, 0x0007)
#define LEAF_JNI_WRONG_THREAD         LEAF_MAKE_ERROR(0x02, 0x08, 0x0008)
#define LEAF_JVM_BRIDGE_CORRUPTED     LEAF_MAKE_ERROR(0x03, 0x08, 0x0001)

/* ---- FILESYSTEM (0x12) ---- */
#define LEAF_FS_IO_ERROR        LEAF_MAKE_ERROR(0x02, 0x12, 0x0001)
#define LEAF_FS_PATH_NOT_FOUND  LEAF_MAKE_ERROR(0x02, 0x12, 0x0002)

/* ---- RESOURCE (0x0C) ---- */
#define LEAF_RESOURCE_CORRUPT   LEAF_MAKE_ERROR(0x02, 0x0C, 0x0001)

/* ---- THREAD (0x14) ---- */
#define LEAF_THREAD_WRONG_THREAD LEAF_MAKE_ERROR(0x02, 0x14, 0x0001)

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* LEAF_ERROR_V1_H */
