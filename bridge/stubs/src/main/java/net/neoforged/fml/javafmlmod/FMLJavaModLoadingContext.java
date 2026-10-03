package net.neoforged.fml.javafmlmod;

import net.neoforged.bus.api.IEventBus;

/** Compile stub. */
public class FMLJavaModLoadingContext {
    private static final FMLJavaModLoadingContext INSTANCE = new FMLJavaModLoadingContext();
    private final IEventBus modBus = new IEventBus.Simple();

    public static FMLJavaModLoadingContext get() {
        return INSTANCE;
    }

    public IEventBus getModEventBus() {
        return modBus;
    }
}
