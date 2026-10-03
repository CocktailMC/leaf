package dev.leafmc.bridge.smoke;

import dev.leafmc.bridge.LeafBridge;

import java.nio.file.Files;
import java.nio.file.Path;

/**
 * Headless smoke test: load libleaf_bridge via FFM and exercise lifecycle.
 *
 * <pre>
 * java -Dleaf.bridge.library=.../libleaf_bridge.so \
 *      -cp ... dev.leafmc.bridge.smoke.BridgeSmoke
 * </pre>
 */
public final class BridgeSmoke {
    public static void main(String[] args) throws Exception {
        Path leafmods = Files.createTempDirectory("leafmc-ffm-leafmods");
        LeafBridge.Config cfg = new LeafBridge.Config();
        cfg.minecraftVersion = "1.21.1";
        cfg.leafmodsDir = leafmods.toAbsolutePath().toString();
        cfg.loader = LeafBridge.LOADER_FABRIC;
        cfg.mapping = LeafBridge.MAPPING_INTERMEDIARY;
        cfg.autoLoadMods = true;

        int init = LeafBridge.init(cfg);
        if (!LeafBridge.isOk(init)) {
            fail("init", init);
        }
        if (!LeafBridge.isInitialized()) {
            throw new IllegalStateException("expected initialized");
        }
        expectOk("pump", LeafBridge.pumpMain(32));
        expectOk("starting", LeafBridge.onServerStarting());
        expectOk("started", LeafBridge.onServerStarted());
        expectOk("join", LeafBridge.emitPlayerJoin(42L));
        int[] decision = new int[] {-1};
        expectOk("join_request", LeafBridge.emitPlayerJoinRequest(7L, decision));
        if (decision[0] != LeafBridge.DECISION_PASS) {
            throw new IllegalStateException("unexpected decision " + decision[0]);
        }
        expectOk("stopping", LeafBridge.onServerStopping());
        expectOk("shutdown", LeafBridge.shutdown());
        System.out.println("BridgeSmoke OK");
    }

    private static void expectOk(String step, int code) {
        if (!LeafBridge.isOk(code)) {
            fail(step, code);
        }
    }

    private static void fail(String step, int code) {
        throw new IllegalStateException(step + " failed: " + LeafBridge.formatError(code));
    }
}
