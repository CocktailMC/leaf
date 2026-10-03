package net.fabricmc.fabric.api.event.lifecycle.v1;

import java.util.ArrayList;
import java.util.List;
import java.util.function.Consumer;

/**
 * Minimal stub of Fabric ServerLifecycleEvents for compile + harness.
 * Replaced by real Fabric API under Loom.
 */
public final class ServerLifecycleEvents {
    private ServerLifecycleEvents() {}

    public static final Event<Runnable> SERVER_STARTING = new Event<>();
    public static final Event<Runnable> SERVER_STARTED = new Event<>();
    public static final Event<Runnable> SERVER_STOPPING = new Event<>();
    public static final Event<Runnable> END_SERVER_TICK = new Event<>();

    public static final class Event<T> {
        private final List<T> listeners = new ArrayList<>();

        public void register(T listener) {
            listeners.add(listener);
        }

        public void invoke(Consumer<T> caller) {
            for (T listener : listeners) {
                caller.accept(listener);
            }
        }
    }
}
