package net.neoforged.neoforge.common;

import net.neoforged.bus.api.IEventBus;

/** Stub NeoForge common bus. */
public final class NeoForge {
    public static final IEventBus EVENT_BUS = new IEventBus.Simple();

    private NeoForge() {}
}
