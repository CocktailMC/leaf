package dev.leafmc.bridge.forge;

import dev.leafmc.bridge.LeafBridge;
import dev.leafmc.bridge.LeafRuntime;
import net.minecraftforge.common.MinecraftForge;
import net.minecraftforge.event.TickEvent;
import net.minecraftforge.event.server.ServerStartedEvent;
import net.minecraftforge.event.server.ServerStartingEvent;
import net.minecraftforge.event.server.ServerStoppingEvent;
import net.minecraftforge.fml.common.Mod;
import net.minecraftforge.fml.event.lifecycle.EventBus;
import net.minecraftforge.fml.javafmlmod.FMLJavaModLoadingContext;

/**
 * Forge bootstrap only. Leaf Mods are owned exclusively by LEAFMC.
 */
@Mod("leaf_bootstrap_forge")
public final class LeafForgeBootstrap {
    public LeafForgeBootstrap() {
        EventBus bus = FMLJavaModLoadingContext.get().getModEventBus();
        bus.addListener(this::onCommonSetup);
    }

    private void onCommonSetup(Object event) {
        LeafRuntime.start(LeafBridge.LOADER_FORGE, LeafBridge.MAPPING_SRG);
        MinecraftForge.EVENT_BUS.addListener(this::handleServerStarting);
        MinecraftForge.EVENT_BUS.addListener(this::handleServerStarted);
        MinecraftForge.EVENT_BUS.addListener(this::handleServerStopping);
        MinecraftForge.EVENT_BUS.addListener(this::handleServerTick);
    }

    private void handleServerStarting(ServerStartingEvent event) {
        onServerStarting();
    }

    private void handleServerStarted(ServerStartedEvent event) {
        onServerStarted();
    }

    private void handleServerStopping(ServerStoppingEvent event) {
        onServerStopping();
    }

    private void handleServerTick(TickEvent.ServerTickEvent event) {
        if (event.phase == TickEvent.ServerTickEvent.Phase.END) {
            onServerTick();
        }
    }

    public static void onServerStarting() {
        LeafBridge.requireOk("forge starting", LeafBridge.onServerStarting());
    }

    public static void onServerStarted() {
        LeafBridge.requireOk("forge started", LeafBridge.onServerStarted());
    }

    public static void onServerStopping() {
        LeafBridge.requireOk("forge stopping", LeafBridge.onServerStopping());
        LeafBridge.requireOk("forge shutdown", LeafBridge.shutdown());
    }

    public static void onServerTick() {
        if (LeafBridge.isInitialized()) {
            LeafBridge.requireOk("forge pump", LeafBridge.pumpMain(64));
        }
    }
}
