package net.fabricmc.fabric.api.networking.v1;

import java.util.ArrayList;
import java.util.List;
import java.util.function.BiConsumer;

/**
 * Stub stand-in so bridges can register join hooks offline.
 * Production builds use Fabric API {@code ServerPlayConnectionEvents}.
 */
public final class ServerPlayConnectionEvents {
    private ServerPlayConnectionEvents() {}

    public static final Event JOIN = new Event();
    public static final Event DISCONNECT = new Event();

    public static final class Event {
        private final List<BiConsumer<Long, Object>> listeners = new ArrayList<>();

        public void register(BiConsumer<Long, Object> listener) {
            listeners.add(listener);
        }

        public void invoke(long playerHandle) {
            for (BiConsumer<Long, Object> listener : listeners) {
                listener.accept(playerHandle, null);
            }
        }
    }
}
