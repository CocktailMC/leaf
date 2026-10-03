package dev.leafmc.bridge.harness;

import dev.leafmc.bridge.LeafBridge;
import dev.leafmc.bridge.fabric.LeafFabricBootstrap;
import dev.leafmc.bridge.forge.LeafForgeBootstrap;
import dev.leafmc.bridge.neoforge.LeafNeoForgeBootstrap;

import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;

/**
 * Offline three-loader smoke: each bootstrap path init → load leafmods →
 * lifecycle → shutdown. Does not require a live Minecraft JVM.
 */
public final class MultiLoaderHarness {
    public static void main(String[] args) {
        Path leafmods = Path.of(System.getProperty("leaf.leafmods.dir", "leafmods"));
        if (!Files.isDirectory(leafmods)) {
            throw new IllegalStateException("leafmods dir missing: " + leafmods.toAbsolutePath());
        }

        runFabric();
        runForge();
        runNeoForge();
        System.out.println("MultiLoaderHarness OK");
    }

    private static void ensureClean() {
        if (LeafBridge.isInitialized()) {
            LeafBridge.requireOk("shutdown-prev", LeafBridge.shutdown());
        }
    }

    private static void runFabric() {
        System.out.println("== Fabric ==");
        ensureClean();
        new LeafFabricBootstrap().onInitialize();
        expectMods();
        LeafFabricBootstrap.onEndServerTick();
        LeafFabricBootstrap.onServerStarting();
        LeafFabricBootstrap.onServerStarted();
        LeafBridge.requireOk("join", LeafBridge.emitPlayerJoin(1001L));
        LeafFabricBootstrap.onServerStopping();
    }

    private static void runForge() {
        System.out.println("== Forge ==");
        ensureClean();
        LeafBridge.Config cfg = LeafBridge.configFromSystem(
                LeafBridge.LOADER_FORGE, LeafBridge.MAPPING_SRG);
        LeafBridge.requireOk("forge init", LeafBridge.init(cfg));
        expectMods();
        LeafForgeBootstrap.onServerTick();
        LeafForgeBootstrap.onServerStarting();
        LeafForgeBootstrap.onServerStarted();
        LeafBridge.requireOk("join", LeafBridge.emitPlayerJoin(1002L));
        LeafForgeBootstrap.onServerStopping();
    }

    private static void runNeoForge() {
        System.out.println("== NeoForge ==");
        ensureClean();
        LeafBridge.Config cfg = LeafBridge.configFromSystem(
                LeafBridge.LOADER_NEOFORGE, LeafBridge.MAPPING_MOJMAP);
        LeafBridge.requireOk("neoforge init", LeafBridge.init(cfg));
        expectMods();
        LeafNeoForgeBootstrap.onServerTick();
        LeafNeoForgeBootstrap.onServerStarting();
        LeafNeoForgeBootstrap.onServerStarted();
        LeafBridge.requireOk("join", LeafBridge.emitPlayerJoin(1003L));
        LeafNeoForgeBootstrap.onServerStopping();
    }

    private static void expectMods() {
        if (!LeafBridge.isInitialized()) {
            throw new IllegalStateException("bridge not initialized");
        }
        List<String> ids = LeafBridge.loadedModIds();
        System.out.println("loaded leafmods: " + ids);
        String expect = System.getProperty("leaf.expect.mod", "greeter");
        if (!expect.isBlank() && !ids.contains(expect)) {
            throw new IllegalStateException(
                    "expected leafmod id '" + expect + "' in " + ids);
        }
    }
}
