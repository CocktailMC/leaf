package dev.leafmc.bridge.fabric;

import dev.leafmc.bridge.LeafBridge;
import dev.leafmc.bridge.LeafRuntime;
import net.fabricmc.api.ModInitializer;
import net.fabricmc.fabric.api.event.lifecycle.v1.ServerLifecycleEvents;
import net.fabricmc.fabric.api.networking.v1.ServerPlayConnectionEvents;

/**
 * Fabric bootstrap only. Does not discover or manage {@code .leafmod} packages.
 */
public final class LeafFabricBootstrap implements ModInitializer {
    @Override
    public void onInitialize() {
        LeafRuntime.start(LeafBridge.LOADER_FABRIC, LeafBridge.MAPPING_INTERMEDIARY);

        ServerLifecycleEvents.SERVER_STARTING.register(LeafFabricBootstrap::onServerStarting);
        ServerLifecycleEvents.SERVER_STARTED.register(LeafFabricBootstrap::onServerStarted);
        ServerLifecycleEvents.SERVER_STOPPING.register(LeafFabricBootstrap::onServerStopping);
        ServerLifecycleEvents.END_SERVER_TICK.register(LeafFabricBootstrap::onEndServerTick);

        ServerPlayConnectionEvents.JOIN.register((player, ignored) -> onPlayerJoin(player));
        ServerPlayConnectionEvents.DISCONNECT.register((player, ignored) -> onPlayerLeave(player));
    }

    public static void onServerStarting() {
        LeafBridge.requireOk("fabric starting", LeafBridge.onServerStarting());
    }

    public static void onServerStarted() {
        LeafBridge.requireOk("fabric started", LeafBridge.onServerStarted());
    }

    public static void onServerStopping() {
        LeafBridge.requireOk("fabric stopping", LeafBridge.onServerStopping());
        LeafBridge.requireOk("fabric shutdown", LeafBridge.shutdown());
    }

    public static void onEndServerTick() {
        if (LeafBridge.isInitialized()) {
            LeafBridge.requireOk("fabric pump", LeafBridge.pumpMain(64));
        }
    }

    public static void onPlayerJoin(long playerHandle) {
        if (LeafBridge.isInitialized()) {
            LeafBridge.requireOk("fabric join", LeafBridge.emitPlayerJoin(playerHandle));
        }
    }

    public static void onPlayerLeave(long playerHandle) {
        if (LeafBridge.isInitialized()) {
            LeafBridge.requireOk("fabric leave", LeafBridge.emitPlayerLeave(playerHandle));
        }
    }
}
