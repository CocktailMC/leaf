/* Leaf Event Packet ABI v1 — stable event protocol (not C++ classes). */

#ifndef LEAF_EVENT_V1_H
#define LEAF_EVENT_V1_H

#include <stdint.h>

#include "leaf/abi/leaf_abi_v1.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Official core event IDs (registry-owned). Do not reuse. */
enum LeafCoreEventId {
    LEAF_EVENT_CORE_SERVER_STARTING = 0x0001,
    LEAF_EVENT_CORE_SERVER_STARTED = 0x0002,

    LEAF_EVENT_PLAYER_JOIN = 0x0100,
    LEAF_EVENT_PLAYER_LEAVE = 0x0101,
    LEAF_EVENT_PLAYER_JOIN_REQUEST = 0x0102,
    LEAF_EVENT_PLAYER_CHAT = 0x0103,
    LEAF_EVENT_PLAYER_DEATH = 0x0104,

    LEAF_EVENT_ENTITY_SPAWN = 0x0200,
    LEAF_EVENT_ENTITY_REMOVE = 0x0201,

    LEAF_EVENT_WORLD_LOAD = 0x0300,
    LEAF_EVENT_BLOCK_BREAK = 0x0301,
    LEAF_EVENT_BLOCK_PLACE = 0x0302,

    LEAF_EVENT_SERVER_TICK = 0x0400
};

enum LeafEventKind {
    LEAF_EVENT_KIND_NOTIFICATION = 1,
    LEAF_EVENT_KIND_DECISION = 2,
    LEAF_EVENT_KIND_INTERNAL = 3
};

enum LeafDispatchModel {
    LEAF_DISPATCH_MAIN_SYNC = 1,
    LEAF_DISPATCH_WORKER_ASYNC = 2,
    LEAF_DISPATCH_PARALLEL_READONLY = 3
};

enum LeafEventPriority {
    LEAF_PRIORITY_EARLIEST = 0,
    LEAF_PRIORITY_EARLY = 1,
    LEAF_PRIORITY_NORMAL = 2,
    LEAF_PRIORITY_LATE = 3,
    LEAF_PRIORITY_LATEST = 4,
    LEAF_PRIORITY_MONITOR = 5
};

enum LeafDecision {
    LEAF_DECISION_PASS = 0,
    LEAF_DECISION_ALLOW = 1,
    LEAF_DECISION_DENY = 2
};

enum LeafCallbackStatus {
    LEAF_CALLBACK_OK = 0,
    LEAF_CALLBACK_ERROR = 1
};

enum LeafEventFlag {
    LEAF_EVENT_FLAG_NONE = 0,
    LEAF_EVENT_FLAG_HAS_VARIABLE_DATA = 1u << 0
};

enum LeafEventSource {
    LEAF_EVENT_SOURCE_UNKNOWN = 0,
    LEAF_EVENT_SOURCE_ENGINE = 1,
    LEAF_EVENT_SOURCE_BRIDGE_FABRIC = 2,
    LEAF_EVENT_SOURCE_BRIDGE_FORGE = 3,
    LEAF_EVENT_SOURCE_BRIDGE_NEOFORGE = 4,
    LEAF_EVENT_SOURCE_TEST = 5,
    LEAF_EVENT_SOURCE_MOD = 6
};

typedef struct LeafEventHeaderV1 {
    uint64_t event_id;
    uint32_t schema_version;
    uint32_t payload_size;

    uint64_t timestamp_ns;
    uint64_t sequence;

    uint32_t flags;
    uint32_t source;
} LeafEventHeaderV1;

typedef struct LeafPlayerJoinPayloadV1 {
    LeafHandle player;
} LeafPlayerJoinPayloadV1;

typedef struct LeafPlayerJoinRequestPayloadV1 {
    LeafHandle player;
} LeafPlayerJoinRequestPayloadV1;

typedef struct LeafPlayerLeavePayloadV1 {
    LeafHandle player;
} LeafPlayerLeavePayloadV1;

/* Fixed UTF-8 chat text (NUL-terminated). Truncated if longer. */
typedef struct LeafPlayerChatPayloadV1 {
    LeafHandle player;
    char message[256];
} LeafPlayerChatPayloadV1;

typedef struct LeafPlayerDeathPayloadV1 {
    LeafHandle player;
} LeafPlayerDeathPayloadV1;

typedef struct LeafEntityLifecyclePayloadV1 {
    LeafHandle entity; /* opaque; typically network entity id */
    uint32_t entity_type_id;
    int32_t x;
    int32_t y;
    int32_t z;
    uint32_t dimension; /* 0 overworld, 1 nether, 2 end */
} LeafEntityLifecyclePayloadV1;

typedef struct LeafWorldLoadPayloadV1 {
    uint32_t dimension; /* 0 overworld, 1 nether, 2 end */
} LeafWorldLoadPayloadV1;

typedef struct LeafBlockChangePayloadV1 {
    LeafHandle player; /* may be 0 for non-player sources */
    int32_t x;
    int32_t y;
    int32_t z;
    uint32_t block_id;
} LeafBlockChangePayloadV1;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* LEAF_EVENT_V1_H */
