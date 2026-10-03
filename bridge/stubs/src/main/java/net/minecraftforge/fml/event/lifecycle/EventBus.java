package net.minecraftforge.fml.event.lifecycle;

import java.util.ArrayList;
import java.util.List;
import java.util.function.Consumer;

/** Compile stub EventBus with optional dispatch for harnesses. */
public class EventBus {
    private final List<Consumer<Object>> listeners = new ArrayList<>();

    @SuppressWarnings("unchecked")
    public <T> void addListener(Consumer<T> listener) {
        listeners.add((Consumer<Object>) listener);
    }

    public void post(Object event) {
        for (Consumer<Object> listener : List.copyOf(listeners)) {
            try {
                listener.accept(event);
            } catch (ClassCastException ignored) {
                // typed listeners ignore unrelated events
            }
        }
    }
}

/** Compile stub. */
class FMLCommonSetupEvent {}
