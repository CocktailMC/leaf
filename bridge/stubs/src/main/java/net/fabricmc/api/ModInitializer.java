package net.fabricmc.api;

/**
 * Matches the production Fabric API package. Stub compile uses this; Loom
 * replaces stubs with the real Fabric Loader dependency.
 */
@FunctionalInterface
public interface ModInitializer {
    void onInitialize();
}
