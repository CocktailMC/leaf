package net.neoforged.bus.api;

import java.util.ArrayList;
import java.util.List;
import java.util.function.Consumer;

/** Compile stub. */
public interface IEventBus {
    <T> void addListener(Consumer<T> listener);

    default void post(Object event) {
        // optional; concrete stub buses override
    }

    /** Simple in-memory bus used by NeoForge stubs. */
    final class Simple implements IEventBus {
        private final List<Consumer<Object>> listeners = new ArrayList<>();

        @Override
        @SuppressWarnings("unchecked")
        public <T> void addListener(Consumer<T> listener) {
            listeners.add((Consumer<Object>) listener);
        }

        @Override
        public void post(Object event) {
            for (Consumer<Object> listener : List.copyOf(listeners)) {
                try {
                    listener.accept(event);
                } catch (ClassCastException ignored) {
                }
            }
        }
    }
}
