package dev.leafmc.bridge;

/**
 * Shared bootstrap helpers used by Fabric / Forge / NeoForge entrypoints.
 * Bridges never scan {@code .leafmod} — they only start the native host.
 */
public final class LeafRuntime {
    private LeafRuntime() {}

    public static void start(int loader, int mapping) {
        LeafBridge.Config cfg = LeafBridge.configFromSystem(loader, mapping);
        LeafConfigFile.applyFileDefaults(cfg);
        if (cfg.gameDir != null && !cfg.gameDir.isBlank()
                && cfg.leafmodsDir != null
                && !java.nio.file.Path.of(cfg.leafmodsDir).isAbsolute()) {
            cfg.leafmodsDir = java.nio.file.Path.of(cfg.gameDir)
                    .resolve(cfg.leafmodsDir)
                    .toAbsolutePath()
                    .normalize()
                    .toString();
        }
        int code = LeafBridge.init(cfg);
        LeafBridge.requireOk("leaf_bridge_init", code);
    }

    public static void runServerLifecycleSmoke() {
        LeafBridge.requireOk("pump", LeafBridge.pumpMain(64));
        LeafBridge.requireOk("starting", LeafBridge.onServerStarting());
        LeafBridge.requireOk("started", LeafBridge.onServerStarted());
    }

    public static void stop() {
        if (LeafBridge.isInitialized()) {
            LeafBridge.requireOk("stopping", LeafBridge.onServerStopping());
            LeafBridge.requireOk("shutdown", LeafBridge.shutdown());
        }
    }
}
