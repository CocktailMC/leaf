package dev.leafmc.bridge.neoforge;

import dev.leafmc.bridge.LeafBridge;
import dev.leafmc.bridge.LeafRuntime;
import net.neoforged.bus.api.IEventBus;
import net.neoforged.fml.common.Mod;
import net.neoforged.fml.event.lifecycle.FMLCommonSetupEvent;
import net.neoforged.fml.javafmlmod.FMLJavaModLoadingContext;
import net.neoforged.neoforge.common.NeoForge;
import net.neoforged.neoforge.event.ServerTickEvent;
import net.neoforged.neoforge.event.server.ServerStartedEvent;
import net.neoforged.neoforge.event.server.ServerStartingEvent;
import net.neoforged.neoforge.event.server.ServerStoppingEvent;

/**
 * NeoForge bootstrap only. Does not implement a Leaf Mod loader.
 */
@Mod("leaf_bootstrap_neoforge")
public final class LeafNeoForgeBootstrap {
    public LeafNeoForgeBootstrap() {
        IEventBus bus = FMLJavaModLoadingContext.get().getModEventBus();
        bus.addListener(this::onCommonSetup);
    }

    private void onCommonSetup(FMLCommonSetupEvent event) {
        LeafRuntime.start(LeafBridge.LOADER_NEOFORGE, LeafBridge.MAPPING_MOJMAP);
        NeoForge.EVENT_BUS.addListener(this::handleServerStarting);
        NeoForge.EVENT_BUS.addListener(this::handleServerStarted);
        NeoForge.EVENT_BUS.addListener(this::handleServerStopping);
        NeoForge.EVENT_BUS.addListener(this::handleServerTick);
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

    private void handleServerTick(ServerTickEvent event) {
        if (event.phase == ServerTickEvent.Phase.END) {
            onServerTick();
        }
    }

    public static void onServerStarting() {
        LeafBridge.requireOk("neoforge starting", LeafBridge.onServerStarting());
    }

    public static void onServerStarted() {
        LeafBridge.requireOk("neoforge started", LeafBridge.onServerStarted());
    }

    public static void onServerStopping() {
        LeafBridge.requireOk("neoforge stopping", LeafBridge.onServerStopping());
        LeafBridge.requireOk("neoforge shutdown", LeafBridge.shutdown());
    }

    public static void onServerTick() {
        if (LeafBridge.isInitialized()) {
            LeafBridge.requireOk("neoforge pump", LeafBridge.pumpMain(64));
        }
    }
}
