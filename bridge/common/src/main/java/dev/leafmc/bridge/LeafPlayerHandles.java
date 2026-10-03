package dev.leafmc.bridge;

import java.util.Map;
import java.util.UUID;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicLong;

/**
 * Stable opaque Leaf player handles backed by Minecraft player UUIDs.
 */
public final class LeafPlayerHandles {
    private static final AtomicLong NEXT = new AtomicLong(1);
    private static final Map<UUID, Long> UUID_TO_HANDLE = new ConcurrentHashMap<>();
    private static final Map<Long, UUID> HANDLE_TO_UUID = new ConcurrentHashMap<>();

    private LeafPlayerHandles() {}

    public static long allocate(UUID uuid) {
        return UUID_TO_HANDLE.computeIfAbsent(uuid, id -> {
            long handle = NEXT.getAndIncrement();
            HANDLE_TO_UUID.put(handle, id);
            return handle;
        });
    }

    public static UUID uuidOf(long handle) {
        return HANDLE_TO_UUID.get(handle);
    }

    public static Long handleOf(UUID uuid) {
        return UUID_TO_HANDLE.get(uuid);
    }

    public static void release(UUID uuid) {
        Long handle = UUID_TO_HANDLE.remove(uuid);
        if (handle != null) {
            HANDLE_TO_UUID.remove(handle);
        }
    }

    public static void release(long handle) {
        UUID uuid = HANDLE_TO_UUID.remove(handle);
        if (uuid != null) {
            UUID_TO_HANDLE.remove(uuid);
        }
    }
}
