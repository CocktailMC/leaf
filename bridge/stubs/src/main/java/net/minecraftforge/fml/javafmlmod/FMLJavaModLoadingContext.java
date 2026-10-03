package net.minecraftforge.fml.javafmlmod;

/** Compile stub. */
public class FMLJavaModLoadingContext {
    public static FMLJavaModLoadingContext get() {
        return new FMLJavaModLoadingContext();
    }

    public net.minecraftforge.fml.event.lifecycle.EventBus getModEventBus() {
        return new net.minecraftforge.fml.event.lifecycle.EventBus();
    }
}
