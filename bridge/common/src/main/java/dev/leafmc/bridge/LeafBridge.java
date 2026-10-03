package dev.leafmc.bridge;

import java.io.IOException;
import java.io.InputStream;
import java.lang.foreign.Arena;
import java.lang.foreign.FunctionDescriptor;
import java.lang.foreign.Linker;
import java.lang.foreign.MemorySegment;
import java.lang.foreign.SymbolLookup;
import java.lang.foreign.ValueLayout;
import java.lang.invoke.MethodHandle;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;
import java.util.Objects;

/**
 * Thin Java façade over the LEAFMC native bridge host (C ABI) via FFM.
 * Loader-specific bootstraps must not scan or load {@code .leafmod} files.
 */
public final class LeafBridge {
    public static final int LOADER_UNKNOWN = 0;
    public static final int LOADER_FABRIC = 1;
    public static final int LOADER_FORGE = 2;
    public static final int LOADER_NEOFORGE = 3;

    public static final int MAPPING_UNKNOWN = 0;
    public static final int MAPPING_MOJMAP = 1;
    public static final int MAPPING_YARN = 2;
    public static final int MAPPING_INTERMEDIARY = 3;
    public static final int MAPPING_SRG = 4;

    public static final int DECISION_PASS = 0;
    public static final int DECISION_ALLOW = 1;
    public static final int DECISION_DENY = 2;

    private static final Linker LINKER = Linker.nativeLinker();
    private static volatile NativeSymbols symbols;

    private LeafBridge() {}

    public static final class Config {
        public String minecraftVersion = "1.21.1";
        public String leafmodsDir = "leafmods";
        public String gameDir = null;
        public int loader = LOADER_UNKNOWN;
        public int mapping = MAPPING_UNKNOWN;
        public boolean autoLoadMods = true;
    }

    /** Build a config from standard {@code leaf.*} system properties. */
    public static Config configFromSystem(int loader, int mapping) {
        Config cfg = new Config();
        cfg.loader = loader;
        cfg.mapping = mapping;
        cfg.minecraftVersion = System.getProperty("leaf.minecraft.version", "1.21.1");
        cfg.leafmodsDir = System.getProperty("leaf.leafmods.dir", "leafmods");
        cfg.gameDir = System.getProperty("leaf.game.dir");
        cfg.autoLoadMods = !"false".equalsIgnoreCase(
                System.getProperty("leaf.autoload", "true"));
        return cfg;
    }

    private static final class NativeSymbols {
        final MethodHandle initFlat;
        final MethodHandle shutdown;
        final MethodHandle isInitialized;
        final MethodHandle pumpMain;
        final MethodHandle onServerStarting;
        final MethodHandle onServerStarted;
        final MethodHandle onServerStopping;
        final MethodHandle emitPlayerJoin;
        final MethodHandle emitPlayerLeave;
        final MethodHandle emitPlayerJoinRequest;
        final MethodHandle emitPlayerChat;
        final MethodHandle emitPlayerDeath;
        final MethodHandle emitEntitySpawn;
        final MethodHandle emitEntityRemove;
        final MethodHandle emitWorldLoad;
        final MethodHandle emitBlockBreak;
        final MethodHandle emitBlockPlace;
        final MethodHandle loadedModCount;
        final MethodHandle modIdAt;
        final MethodHandle reloadMods;
        final MethodHandle setSendHook;
        final MethodHandle setBroadcastHook;
        final MethodHandle registerPlayer;
        final MethodHandle unregisterPlayer;
        final MethodHandle setInventoryHooks;
        final MethodHandle setInventoryStackHooks;
        final MethodHandle setGetBlockHook;
        final MethodHandle setWorldHooks;
        final MethodHandle setPlayerPosHooks;
        final MethodHandle setPlayerHealthHooks;
        final MethodHandle setPlayerFoodHooks;
        final MethodHandle setPlayerGamemodeHooks;
        final MethodHandle setPlayerXpHooks;
        final MethodHandle setPlayerLookHooks;
        final MethodHandle setPlaySoundHook;
        final MethodHandle setActionbarHook;
        final MethodHandle setTitleHook;
        final MethodHandle setKickPlayerHook;
        final MethodHandle setGiveItemHook;
        final MethodHandle setEffectHooks;
        final MethodHandle setSpawnParticleHook;
        final MethodHandle setWorldTimeHooks;
        final MethodHandle setPlayerVelocityHooks;
        final MethodHandle setPlayerFlagsHook;
        final MethodHandle setRunCommandHook;
        final MethodHandle setClearInventoryHook;
        final MethodHandle setPlayerFlightHook;
        final MethodHandle setGetBiomeHook;
        final MethodHandle setDifficultyHooks;
        final MethodHandle setWeatherHooks;
        final MethodHandle setGetLightLevelHook;
        final MethodHandle setPlayerLatencyHook;
        final MethodHandle setWorldSpawnHooks;
        final MethodHandle setPlayerOpHook;
        final MethodHandle setPlayerUuidHook;
        final MethodHandle setPlayerPermissionHook;
        final MethodHandle setFindPlayerUuidHook;
        final MethodHandle setFindPlayerNameHook;
        final MethodHandle setBlockRegistryHooks;
        final MethodHandle setGiveItemRegistryHook;
        final MethodHandle setInventoryRegistryHooks;
        final MethodHandle setTeleportHook;
        final MethodHandle setSelectedSlotHooks;
        final MethodHandle setGetWorldSeedHook;
        final MethodHandle setPlayerAbsorptionHooks;
        final MethodHandle setPlayerInvulnerableHooks;
        final MethodHandle setPlayerAirHooks;
        final MethodHandle setPlayerFireTicksHooks;
        final MethodHandle setPlayerFrozenTicksHooks;
        final MethodHandle setPlayerNoGravityHooks;
        final MethodHandle setPlayerSilentHooks;
        final MethodHandle setPlayerGlowingHooks;
        final MethodHandle setPlayerInvisibleHooks;
        final MethodHandle setPlayerPortalCooldownHooks;
        final MethodHandle setGetPlayerMaxAirHook;

        NativeSymbols(SymbolLookup lookup) {
            this.initFlat = downcall(lookup, "leaf_bridge_init_flat",
                    FunctionDescriptor.of(
                            ValueLayout.JAVA_INT,
                            ValueLayout.ADDRESS,
                            ValueLayout.ADDRESS,
                            ValueLayout.ADDRESS,
                            ValueLayout.JAVA_INT,
                            ValueLayout.JAVA_INT,
                            ValueLayout.JAVA_INT));
            this.shutdown = downcall(lookup, "leaf_bridge_shutdown",
                    FunctionDescriptor.of(ValueLayout.JAVA_INT));
            this.isInitialized = downcall(lookup, "leaf_bridge_is_initialized",
                    FunctionDescriptor.of(ValueLayout.JAVA_INT));
            this.pumpMain = downcall(lookup, "leaf_bridge_pump_main",
                    FunctionDescriptor.of(ValueLayout.JAVA_INT, ValueLayout.JAVA_INT));
            this.onServerStarting = downcall(lookup, "leaf_bridge_on_server_starting",
                    FunctionDescriptor.of(ValueLayout.JAVA_INT));
            this.onServerStarted = downcall(lookup, "leaf_bridge_on_server_started",
                    FunctionDescriptor.of(ValueLayout.JAVA_INT));
            this.onServerStopping = downcall(lookup, "leaf_bridge_on_server_stopping",
                    FunctionDescriptor.of(ValueLayout.JAVA_INT));
            this.emitPlayerJoin = downcall(lookup, "leaf_bridge_emit_player_join",
                    FunctionDescriptor.of(ValueLayout.JAVA_INT, ValueLayout.JAVA_LONG));
            this.emitPlayerLeave = downcall(lookup, "leaf_bridge_emit_player_leave",
                    FunctionDescriptor.of(ValueLayout.JAVA_INT, ValueLayout.JAVA_LONG));
            this.emitPlayerJoinRequest = downcall(lookup, "leaf_bridge_emit_player_join_request",
                    FunctionDescriptor.of(
                            ValueLayout.JAVA_INT,
                            ValueLayout.JAVA_LONG,
                            ValueLayout.ADDRESS));
            this.emitPlayerChat = downcall(lookup, "leaf_bridge_emit_player_chat",
                    FunctionDescriptor.of(
                            ValueLayout.JAVA_INT,
                            ValueLayout.JAVA_LONG,
                            ValueLayout.ADDRESS,
                            ValueLayout.ADDRESS));
            this.emitPlayerDeath = downcall(lookup, "leaf_bridge_emit_player_death",
                    FunctionDescriptor.of(ValueLayout.JAVA_INT, ValueLayout.JAVA_LONG));
            this.emitEntitySpawn = downcall(lookup, "leaf_bridge_emit_entity_spawn",
                    FunctionDescriptor.of(
                            ValueLayout.JAVA_INT,
                            ValueLayout.JAVA_LONG,
                            ValueLayout.JAVA_INT,
                            ValueLayout.JAVA_INT,
                            ValueLayout.JAVA_INT,
                            ValueLayout.JAVA_INT,
                            ValueLayout.JAVA_INT));
            this.emitEntityRemove = downcall(lookup, "leaf_bridge_emit_entity_remove",
                    FunctionDescriptor.of(
                            ValueLayout.JAVA_INT,
                            ValueLayout.JAVA_LONG,
                            ValueLayout.JAVA_INT,
                            ValueLayout.JAVA_INT,
                            ValueLayout.JAVA_INT,
                            ValueLayout.JAVA_INT,
                            ValueLayout.JAVA_INT));
            this.emitWorldLoad = downcall(lookup, "leaf_bridge_emit_world_load",
                    FunctionDescriptor.of(ValueLayout.JAVA_INT, ValueLayout.JAVA_INT));
            this.emitBlockBreak = downcall(lookup, "leaf_bridge_emit_block_break",
                    FunctionDescriptor.of(
                            ValueLayout.JAVA_INT,
                            ValueLayout.JAVA_LONG,
                            ValueLayout.JAVA_INT,
                            ValueLayout.JAVA_INT,
                            ValueLayout.JAVA_INT,
                            ValueLayout.JAVA_INT,
                            ValueLayout.ADDRESS));
            this.emitBlockPlace = downcall(lookup, "leaf_bridge_emit_block_place",
                    FunctionDescriptor.of(
                            ValueLayout.JAVA_INT,
                            ValueLayout.JAVA_LONG,
                            ValueLayout.JAVA_INT,
                            ValueLayout.JAVA_INT,
                            ValueLayout.JAVA_INT,
                            ValueLayout.JAVA_INT,
                            ValueLayout.ADDRESS));
            this.loadedModCount = downcall(lookup, "leaf_bridge_loaded_mod_count",
                    FunctionDescriptor.of(ValueLayout.JAVA_INT));
            this.modIdAt = downcall(lookup, "leaf_bridge_mod_id_at",
                    FunctionDescriptor.of(
                            ValueLayout.JAVA_INT,
                            ValueLayout.JAVA_INT,
                            ValueLayout.ADDRESS,
                            ValueLayout.JAVA_INT));
            this.reloadMods = downcall(lookup, "leaf_bridge_reload_mods",
                    FunctionDescriptor.of(ValueLayout.JAVA_INT));
            this.setSendHook = downcall(lookup, "leaf_bridge_set_send_player_message_hook",
                    FunctionDescriptor.ofVoid(ValueLayout.ADDRESS));
            this.setBroadcastHook = downcall(lookup, "leaf_bridge_set_broadcast_message_hook",
                    FunctionDescriptor.ofVoid(ValueLayout.ADDRESS));
            this.registerPlayer = downcall(lookup, "leaf_bridge_register_player",
                    FunctionDescriptor.of(
                            ValueLayout.JAVA_INT,
                            ValueLayout.JAVA_LONG,
                            ValueLayout.ADDRESS));
            this.unregisterPlayer = downcall(lookup, "leaf_bridge_unregister_player",
                    FunctionDescriptor.of(ValueLayout.JAVA_INT, ValueLayout.JAVA_LONG));
            this.setInventoryHooks = downcall(lookup, "leaf_bridge_set_inventory_hooks",
                    FunctionDescriptor.ofVoid(ValueLayout.ADDRESS, ValueLayout.ADDRESS));
            this.setInventoryStackHooks = downcall(
                    lookup,
                    "leaf_bridge_set_inventory_stack_hooks",
                    FunctionDescriptor.ofVoid(ValueLayout.ADDRESS, ValueLayout.ADDRESS));
            this.setGetBlockHook = downcall(lookup, "leaf_bridge_set_get_block_hook",
                    FunctionDescriptor.ofVoid(ValueLayout.ADDRESS));
            this.setWorldHooks = downcall(lookup, "leaf_bridge_set_world_hooks",
                    FunctionDescriptor.ofVoid(ValueLayout.ADDRESS, ValueLayout.ADDRESS));
            this.setPlayerPosHooks = downcall(
                    lookup,
                    "leaf_bridge_set_player_pos_hooks",
                    FunctionDescriptor.ofVoid(ValueLayout.ADDRESS, ValueLayout.ADDRESS));
            this.setPlayerHealthHooks = downcall(
                    lookup,
                    "leaf_bridge_set_player_health_hooks",
                    FunctionDescriptor.ofVoid(ValueLayout.ADDRESS, ValueLayout.ADDRESS));
            this.setPlayerFoodHooks = downcall(
                    lookup,
                    "leaf_bridge_set_player_food_hooks",
                    FunctionDescriptor.ofVoid(ValueLayout.ADDRESS, ValueLayout.ADDRESS));
            this.setPlayerGamemodeHooks = downcall(
                    lookup,
                    "leaf_bridge_set_player_gamemode_hooks",
                    FunctionDescriptor.ofVoid(ValueLayout.ADDRESS, ValueLayout.ADDRESS));
            this.setPlayerXpHooks = downcall(
                    lookup,
                    "leaf_bridge_set_player_xp_hooks",
                    FunctionDescriptor.ofVoid(ValueLayout.ADDRESS, ValueLayout.ADDRESS));
            this.setPlayerLookHooks = downcall(
                    lookup,
                    "leaf_bridge_set_player_look_hooks",
                    FunctionDescriptor.ofVoid(ValueLayout.ADDRESS, ValueLayout.ADDRESS));
            this.setPlaySoundHook = downcall(
                    lookup,
                    "leaf_bridge_set_play_sound_hook",
                    FunctionDescriptor.ofVoid(ValueLayout.ADDRESS));
            this.setActionbarHook = downcall(
                    lookup,
                    "leaf_bridge_set_actionbar_hook",
                    FunctionDescriptor.ofVoid(ValueLayout.ADDRESS));
            this.setTitleHook = downcall(
                    lookup,
                    "leaf_bridge_set_title_hook",
                    FunctionDescriptor.ofVoid(ValueLayout.ADDRESS));
            this.setKickPlayerHook = downcall(
                    lookup,
                    "leaf_bridge_set_kick_player_hook",
                    FunctionDescriptor.ofVoid(ValueLayout.ADDRESS));
            this.setGiveItemHook = downcall(
                    lookup,
                    "leaf_bridge_set_give_item_hook",
                    FunctionDescriptor.ofVoid(ValueLayout.ADDRESS));
            this.setEffectHooks = downcall(
                    lookup,
                    "leaf_bridge_set_effect_hooks",
                    FunctionDescriptor.ofVoid(ValueLayout.ADDRESS, ValueLayout.ADDRESS));
            this.setSpawnParticleHook = downcall(
                    lookup,
                    "leaf_bridge_set_spawn_particle_hook",
                    FunctionDescriptor.ofVoid(ValueLayout.ADDRESS));
            this.setWorldTimeHooks = downcall(
                    lookup,
                    "leaf_bridge_set_world_time_hooks",
                    FunctionDescriptor.ofVoid(ValueLayout.ADDRESS, ValueLayout.ADDRESS));
            this.setPlayerVelocityHooks = downcall(
                    lookup,
                    "leaf_bridge_set_player_velocity_hooks",
                    FunctionDescriptor.ofVoid(ValueLayout.ADDRESS, ValueLayout.ADDRESS));
            this.setPlayerFlagsHook = downcall(
                    lookup,
                    "leaf_bridge_set_player_flags_hook",
                    FunctionDescriptor.ofVoid(ValueLayout.ADDRESS));
            this.setRunCommandHook = downcall(
                    lookup,
                    "leaf_bridge_set_run_command_hook",
                    FunctionDescriptor.ofVoid(ValueLayout.ADDRESS));
            this.setClearInventoryHook = downcall(
                    lookup,
                    "leaf_bridge_set_clear_inventory_hook",
                    FunctionDescriptor.ofVoid(ValueLayout.ADDRESS));
            this.setPlayerFlightHook = downcall(
                    lookup,
                    "leaf_bridge_set_player_flight_hook",
                    FunctionDescriptor.ofVoid(ValueLayout.ADDRESS));
            this.setGetBiomeHook = downcall(
                    lookup,
                    "leaf_bridge_set_get_biome_hook",
                    FunctionDescriptor.ofVoid(ValueLayout.ADDRESS));
            this.setDifficultyHooks = downcall(
                    lookup,
                    "leaf_bridge_set_difficulty_hooks",
                    FunctionDescriptor.ofVoid(ValueLayout.ADDRESS, ValueLayout.ADDRESS));
            this.setWeatherHooks = downcall(
                    lookup,
                    "leaf_bridge_set_weather_hooks",
                    FunctionDescriptor.ofVoid(ValueLayout.ADDRESS, ValueLayout.ADDRESS));
            this.setGetLightLevelHook = downcall(
                    lookup,
                    "leaf_bridge_set_get_light_level_hook",
                    FunctionDescriptor.ofVoid(ValueLayout.ADDRESS));
            this.setPlayerLatencyHook = downcall(
                    lookup,
                    "leaf_bridge_set_player_latency_hook",
                    FunctionDescriptor.ofVoid(ValueLayout.ADDRESS));
            this.setWorldSpawnHooks = downcall(
                    lookup,
                    "leaf_bridge_set_world_spawn_hooks",
                    FunctionDescriptor.ofVoid(ValueLayout.ADDRESS, ValueLayout.ADDRESS));
            this.setPlayerOpHook = downcall(
                    lookup,
                    "leaf_bridge_set_player_op_hook",
                    FunctionDescriptor.ofVoid(ValueLayout.ADDRESS));
            this.setPlayerUuidHook = downcall(
                    lookup,
                    "leaf_bridge_set_player_uuid_hook",
                    FunctionDescriptor.ofVoid(ValueLayout.ADDRESS));
            this.setPlayerPermissionHook = downcall(
                    lookup,
                    "leaf_bridge_set_player_permission_hook",
                    FunctionDescriptor.ofVoid(ValueLayout.ADDRESS));
            this.setFindPlayerUuidHook = downcall(
                    lookup,
                    "leaf_bridge_set_find_player_uuid_hook",
                    FunctionDescriptor.ofVoid(ValueLayout.ADDRESS));
            this.setFindPlayerNameHook = downcall(
                    lookup,
                    "leaf_bridge_set_find_player_name_hook",
                    FunctionDescriptor.ofVoid(ValueLayout.ADDRESS));
            this.setBlockRegistryHooks = downcall(
                    lookup,
                    "leaf_bridge_set_block_registry_hooks",
                    FunctionDescriptor.ofVoid(ValueLayout.ADDRESS, ValueLayout.ADDRESS));
            this.setGiveItemRegistryHook = downcall(
                    lookup,
                    "leaf_bridge_set_give_item_registry_hook",
                    FunctionDescriptor.ofVoid(ValueLayout.ADDRESS));
            this.setInventoryRegistryHooks = downcall(
                    lookup,
                    "leaf_bridge_set_inventory_registry_hooks",
                    FunctionDescriptor.ofVoid(ValueLayout.ADDRESS, ValueLayout.ADDRESS));
            this.setTeleportHook = downcall(
                    lookup,
                    "leaf_bridge_set_teleport_hook",
                    FunctionDescriptor.ofVoid(ValueLayout.ADDRESS));
            this.setSelectedSlotHooks = downcall(
                    lookup,
                    "leaf_bridge_set_selected_slot_hooks",
                    FunctionDescriptor.ofVoid(ValueLayout.ADDRESS, ValueLayout.ADDRESS));
            this.setGetWorldSeedHook = downcall(
                    lookup,
                    "leaf_bridge_set_get_world_seed_hook",
                    FunctionDescriptor.ofVoid(ValueLayout.ADDRESS));
            this.setPlayerAbsorptionHooks = downcall(
                    lookup,
                    "leaf_bridge_set_player_absorption_hooks",
                    FunctionDescriptor.ofVoid(ValueLayout.ADDRESS, ValueLayout.ADDRESS));
            this.setPlayerInvulnerableHooks = downcall(
                    lookup,
                    "leaf_bridge_set_player_invulnerable_hooks",
                    FunctionDescriptor.ofVoid(ValueLayout.ADDRESS, ValueLayout.ADDRESS));
            this.setPlayerAirHooks = downcall(
                    lookup,
                    "leaf_bridge_set_player_air_hooks",
                    FunctionDescriptor.ofVoid(ValueLayout.ADDRESS, ValueLayout.ADDRESS));
            this.setPlayerFireTicksHooks = downcall(
                    lookup,
                    "leaf_bridge_set_player_fire_ticks_hooks",
                    FunctionDescriptor.ofVoid(ValueLayout.ADDRESS, ValueLayout.ADDRESS));
            this.setPlayerFrozenTicksHooks = downcall(
                    lookup,
                    "leaf_bridge_set_player_frozen_ticks_hooks",
                    FunctionDescriptor.ofVoid(ValueLayout.ADDRESS, ValueLayout.ADDRESS));
            this.setPlayerNoGravityHooks = downcall(
                    lookup,
                    "leaf_bridge_set_player_no_gravity_hooks",
                    FunctionDescriptor.ofVoid(ValueLayout.ADDRESS, ValueLayout.ADDRESS));
            this.setPlayerSilentHooks = downcall(
                    lookup,
                    "leaf_bridge_set_player_silent_hooks",
                    FunctionDescriptor.ofVoid(ValueLayout.ADDRESS, ValueLayout.ADDRESS));
            this.setPlayerGlowingHooks = downcall(
                    lookup,
                    "leaf_bridge_set_player_glowing_hooks",
                    FunctionDescriptor.ofVoid(ValueLayout.ADDRESS, ValueLayout.ADDRESS));
            this.setPlayerInvisibleHooks = downcall(
                    lookup,
                    "leaf_bridge_set_player_invisible_hooks",
                    FunctionDescriptor.ofVoid(ValueLayout.ADDRESS, ValueLayout.ADDRESS));
            this.setPlayerPortalCooldownHooks = downcall(
                    lookup,
                    "leaf_bridge_set_player_portal_cooldown_hooks",
                    FunctionDescriptor.ofVoid(ValueLayout.ADDRESS, ValueLayout.ADDRESS));
            this.setGetPlayerMaxAirHook = downcall(
                    lookup,
                    "leaf_bridge_set_get_player_max_air_hook",
                    FunctionDescriptor.ofVoid(ValueLayout.ADDRESS));
        }

        private static MethodHandle downcall(
                SymbolLookup lookup, String name, FunctionDescriptor desc) {
            MemorySegment sym = lookup.find(name)
                    .orElseThrow(() -> new UnsatisfiedLinkError("missing symbol: " + name));
            return LINKER.downcallHandle(sym, desc);
        }
    }

    public static synchronized void loadNative() {
        if (symbols != null) {
            return;
        }
        Path lib = resolveLibraryPath();
        SymbolLookup lookup = SymbolLookup.libraryLookup(lib, Arena.global());
        symbols = new NativeSymbols(lookup);
    }

    private static Path resolveLibraryPath() {
        String explicit = System.getProperty("leaf.bridge.library");
        if (explicit != null && !explicit.isBlank()) {
            Path p = Path.of(explicit);
            if (!Files.isRegularFile(p)) {
                throw new UnsatisfiedLinkError("leaf.bridge.library not found: " + p);
            }
            return p.toAbsolutePath().normalize();
        }

        String mapped = System.mapLibraryName("leaf_bridge");
        Path cwd = Path.of(mapped).toAbsolutePath();
        if (Files.isRegularFile(cwd)) {
            return cwd;
        }

        Path extracted = extractBundledNative(mapped);
        if (extracted != null) {
            return extracted;
        }

        throw new UnsatisfiedLinkError(
                "Set -Dleaf.bridge.library=/absolute/path/to/" + mapped
                        + " or embed native/" + hostTriple() + "/" + mapped
                        + " inside the bridge JAR");
    }

    private static String hostTriple() {
        String os = System.getProperty("os.name", "").toLowerCase(Locale.ROOT);
        String arch = System.getProperty("os.arch", "").toLowerCase(Locale.ROOT);
        String osPart;
        if (os.contains("win")) {
            osPart = "windows";
        } else if (os.contains("mac") || os.contains("darwin")) {
            osPart = "macos";
        } else {
            osPart = "linux";
        }
        String archPart;
        if (arch.contains("aarch64") || arch.equals("arm64")) {
            archPart = osPart.equals("linux") ? "aarch64" : "arm64";
        } else {
            archPart = "x86_64";
        }
        return osPart + "-" + archPart;
    }

    private static Path extractBundledNative(String mappedName) {
        String resource = "native/" + hostTriple() + "/" + mappedName;
        try (InputStream in = LeafBridge.class.getClassLoader().getResourceAsStream(resource)) {
            if (in == null) {
                return null;
            }
            Path dir = Path.of(System.getProperty("java.io.tmpdir"), "leafmc-native");
            Files.createDirectories(dir);
            Path out = dir.resolve(mappedName);
            Files.copy(in, out, StandardCopyOption.REPLACE_EXISTING);
            out.toFile().setExecutable(true);
            return out.toAbsolutePath().normalize();
        } catch (IOException e) {
            throw new UnsatisfiedLinkError("failed to extract bundled native: " + e.getMessage());
        }
    }

    private static NativeSymbols requireSymbols() {
        loadNative();
        return Objects.requireNonNull(symbols);
    }

    public static int init(Config config) {
        Objects.requireNonNull(config, "config");
        NativeSymbols s = requireSymbols();
        try (Arena arena = Arena.ofConfined()) {
            MemorySegment mc = arena.allocateFrom(config.minecraftVersion);
            MemorySegment mods = arena.allocateFrom(config.leafmodsDir);
            MemorySegment game = config.gameDir == null || config.gameDir.isBlank()
                    ? MemorySegment.NULL
                    : arena.allocateFrom(config.gameDir);
            return (int) s.initFlat.invokeExact(
                    mc,
                    mods,
                    game,
                    config.loader,
                    config.mapping,
                    config.autoLoadMods ? 1 : 0);
        } catch (Throwable t) {
            throw new IllegalStateException("leaf_bridge_init_flat failed", t);
        }
    }

    public static int shutdown() {
        try {
            return (int) requireSymbols().shutdown.invokeExact();
        } catch (Throwable t) {
            throw new IllegalStateException("leaf_bridge_shutdown failed", t);
        }
    }

    public static boolean isInitialized() {
        try {
            return ((int) requireSymbols().isInitialized.invokeExact()) != 0;
        } catch (Throwable t) {
            throw new IllegalStateException("leaf_bridge_is_initialized failed", t);
        }
    }

    public static int pumpMain(int budget) {
        try {
            return (int) requireSymbols().pumpMain.invokeExact(budget);
        } catch (Throwable t) {
            throw new IllegalStateException("leaf_bridge_pump_main failed", t);
        }
    }

    public static int onServerStarting() {
        try {
            return (int) requireSymbols().onServerStarting.invokeExact();
        } catch (Throwable t) {
            throw new IllegalStateException("leaf_bridge_on_server_starting failed", t);
        }
    }

    public static int onServerStarted() {
        try {
            return (int) requireSymbols().onServerStarted.invokeExact();
        } catch (Throwable t) {
            throw new IllegalStateException("leaf_bridge_on_server_started failed", t);
        }
    }

    public static int onServerStopping() {
        try {
            return (int) requireSymbols().onServerStopping.invokeExact();
        } catch (Throwable t) {
            throw new IllegalStateException("leaf_bridge_on_server_stopping failed", t);
        }
    }

    public static int emitPlayerJoin(long playerHandle) {
        try {
            return (int) requireSymbols().emitPlayerJoin.invokeExact(playerHandle);
        } catch (Throwable t) {
            throw new IllegalStateException("leaf_bridge_emit_player_join failed", t);
        }
    }

    public static int emitPlayerLeave(long playerHandle) {
        try {
            return (int) requireSymbols().emitPlayerLeave.invokeExact(playerHandle);
        } catch (Throwable t) {
            throw new IllegalStateException("leaf_bridge_emit_player_leave failed", t);
        }
    }

    public static int emitPlayerJoinRequest(long playerHandle, int[] outDecision) {
        Objects.requireNonNull(outDecision, "outDecision");
        if (outDecision.length < 1) {
            throw new IllegalArgumentException("outDecision length < 1");
        }
        try (Arena arena = Arena.ofConfined()) {
            MemorySegment out = arena.allocate(ValueLayout.JAVA_INT);
            int code = (int) requireSymbols().emitPlayerJoinRequest.invokeExact(
                    playerHandle, out);
            if (code == 0) {
                outDecision[0] = out.get(ValueLayout.JAVA_INT, 0);
            }
            return code;
        } catch (Throwable t) {
            throw new IllegalStateException("leaf_bridge_emit_player_join_request failed", t);
        }
    }

    /** Emit player chat decision. Returns bridge error code; outDecision[0] is LEAF_DECISION_*. */
    public static int emitPlayerChat(long playerHandle, String message, int[] outDecision) {
        Objects.requireNonNull(outDecision, "outDecision");
        if (outDecision.length < 1) {
            throw new IllegalArgumentException("outDecision length < 1");
        }
        try (Arena arena = Arena.ofConfined()) {
            MemorySegment msg = arena.allocateFrom(
                    message != null ? message : "", StandardCharsets.UTF_8);
            MemorySegment out = arena.allocate(ValueLayout.JAVA_INT);
            int code = (int) requireSymbols().emitPlayerChat.invokeExact(
                    playerHandle, msg, out);
            if (code == 0) {
                outDecision[0] = out.get(ValueLayout.JAVA_INT, 0);
            }
            return code;
        } catch (Throwable t) {
            throw new IllegalStateException("leaf_bridge_emit_player_chat failed", t);
        }
    }

    public static int emitPlayerDeath(long playerHandle) {
        try {
            return (int) requireSymbols().emitPlayerDeath.invokeExact(playerHandle);
        } catch (Throwable t) {
            throw new IllegalStateException("leaf_bridge_emit_player_death failed", t);
        }
    }

    public static int emitEntitySpawn(
            long entityHandle,
            int entityTypeId,
            int x,
            int y,
            int z,
            int dimension) {
        try {
            return (int) requireSymbols().emitEntitySpawn.invokeExact(
                    entityHandle, entityTypeId, x, y, z, dimension);
        } catch (Throwable t) {
            throw new IllegalStateException("leaf_bridge_emit_entity_spawn failed", t);
        }
    }

    public static int emitEntityRemove(
            long entityHandle,
            int entityTypeId,
            int x,
            int y,
            int z,
            int dimension) {
        try {
            return (int) requireSymbols().emitEntityRemove.invokeExact(
                    entityHandle, entityTypeId, x, y, z, dimension);
        } catch (Throwable t) {
            throw new IllegalStateException("leaf_bridge_emit_entity_remove failed", t);
        }
    }

    public static int emitWorldLoad(int dimension) {
        try {
            return (int) requireSymbols().emitWorldLoad.invokeExact(dimension);
        } catch (Throwable t) {
            throw new IllegalStateException("leaf_bridge_emit_world_load failed", t);
        }
    }

    public static int emitBlockBreak(
            long playerHandle, int x, int y, int z, int blockId, int[] outDecision) {
        return emitBlockChange(true, playerHandle, x, y, z, blockId, outDecision);
    }

    public static int emitBlockPlace(
            long playerHandle, int x, int y, int z, int blockId, int[] outDecision) {
        return emitBlockChange(false, playerHandle, x, y, z, blockId, outDecision);
    }

    private static int emitBlockChange(
            boolean isBreak,
            long playerHandle,
            int x,
            int y,
            int z,
            int blockId,
            int[] outDecision) {
        Objects.requireNonNull(outDecision, "outDecision");
        if (outDecision.length < 1) {
            throw new IllegalArgumentException("outDecision length < 1");
        }
        try (Arena arena = Arena.ofConfined()) {
            MemorySegment out = arena.allocate(ValueLayout.JAVA_INT);
            MethodHandle mh = isBreak
                    ? requireSymbols().emitBlockBreak
                    : requireSymbols().emitBlockPlace;
            int code = (int) mh.invokeExact(playerHandle, x, y, z, blockId, out);
            if (code == 0) {
                outDecision[0] = out.get(ValueLayout.JAVA_INT, 0);
            }
            return code;
        } catch (Throwable t) {
            throw new IllegalStateException(
                    (isBreak ? "leaf_bridge_emit_block_break" : "leaf_bridge_emit_block_place")
                            + " failed",
                    t);
        }
    }

    public static int loadedModCount() {
        try {
            return (int) requireSymbols().loadedModCount.invokeExact();
        } catch (Throwable t) {
            throw new IllegalStateException("leaf_bridge_loaded_mod_count failed", t);
        }
    }

    public static String modIdAt(int index) {
        try (Arena arena = Arena.ofConfined()) {
            MemorySegment buf = arena.allocate(256);
            int code = (int) requireSymbols().modIdAt.invokeExact(
                    index, buf, 256);
            if (code != 0) {
                throw new IllegalStateException("leaf_bridge_mod_id_at: " + formatError(code));
            }
            return buf.getString(0, StandardCharsets.UTF_8);
        } catch (IllegalStateException e) {
            throw e;
        } catch (Throwable t) {
            throw new IllegalStateException("leaf_bridge_mod_id_at failed", t);
        }
    }

    public static List<String> loadedModIds() {
        int n = loadedModCount();
        List<String> ids = new ArrayList<>(n);
        for (int i = 0; i < n; i++) {
            ids.add(modIdAt(i));
        }
        return ids;
    }

    public static int reloadMods() {
        try {
            return (int) requireSymbols().reloadMods.invokeExact();
        } catch (Throwable t) {
            throw new IllegalStateException("leaf_bridge_reload_mods failed", t);
        }
    }

    /** Install a native function pointer (FFM upcall stub) for chat delivery. */
    public static void setSendPlayerMessageHook(MemorySegment fn) {
        try {
            MemorySegment ptr = fn != null ? fn : MemorySegment.NULL;
            requireSymbols().setSendHook.invoke(ptr);
        } catch (Throwable t) {
            throw new IllegalStateException(
                    "leaf_bridge_set_send_player_message_hook failed", t);
        }
    }

    public static void setBroadcastMessageHook(MemorySegment fn) {
        try {
            MemorySegment ptr = fn != null ? fn : MemorySegment.NULL;
            requireSymbols().setBroadcastHook.invoke(ptr);
        } catch (Throwable t) {
            throw new IllegalStateException(
                    "leaf_bridge_set_broadcast_message_hook failed", t);
        }
    }

    public static void setInventoryHooks(MemorySegment getFn, MemorySegment setFn) {
        try {
            MemorySegment g = getFn != null ? getFn : MemorySegment.NULL;
            MemorySegment s = setFn != null ? setFn : MemorySegment.NULL;
            requireSymbols().setInventoryHooks.invoke(g, s);
        } catch (Throwable t) {
            throw new IllegalStateException("leaf_bridge_set_inventory_hooks failed", t);
        }
    }

    public static void setInventoryStackHooks(MemorySegment getFn, MemorySegment setFn) {
        try {
            MemorySegment g = getFn != null ? getFn : MemorySegment.NULL;
            MemorySegment s = setFn != null ? setFn : MemorySegment.NULL;
            requireSymbols().setInventoryStackHooks.invoke(g, s);
        } catch (Throwable t) {
            throw new IllegalStateException(
                    "leaf_bridge_set_inventory_stack_hooks failed", t);
        }
    }

    public static void setGetBlockHook(MemorySegment fn) {
        try {
            MemorySegment ptr = fn != null ? fn : MemorySegment.NULL;
            requireSymbols().setGetBlockHook.invoke(ptr);
        } catch (Throwable t) {
            throw new IllegalStateException("leaf_bridge_set_get_block_hook failed", t);
        }
    }

    public static void setWorldHooks(MemorySegment getFn, MemorySegment setFn) {
        try {
            MemorySegment g = getFn != null ? getFn : MemorySegment.NULL;
            MemorySegment s = setFn != null ? setFn : MemorySegment.NULL;
            requireSymbols().setWorldHooks.invoke(g, s);
        } catch (Throwable t) {
            throw new IllegalStateException("leaf_bridge_set_world_hooks failed", t);
        }
    }

    public static void setPlayerPosHooks(MemorySegment getFn, MemorySegment setFn) {
        try {
            MemorySegment g = getFn != null ? getFn : MemorySegment.NULL;
            MemorySegment s = setFn != null ? setFn : MemorySegment.NULL;
            requireSymbols().setPlayerPosHooks.invoke(g, s);
        } catch (Throwable t) {
            throw new IllegalStateException(
                    "leaf_bridge_set_player_pos_hooks failed", t);
        }
    }

    public static void setPlayerHealthHooks(MemorySegment getFn, MemorySegment setFn) {
        try {
            MemorySegment g = getFn != null ? getFn : MemorySegment.NULL;
            MemorySegment s = setFn != null ? setFn : MemorySegment.NULL;
            requireSymbols().setPlayerHealthHooks.invoke(g, s);
        } catch (Throwable t) {
            throw new IllegalStateException(
                    "leaf_bridge_set_player_health_hooks failed", t);
        }
    }

    public static void setPlayerFoodHooks(MemorySegment getFn, MemorySegment setFn) {
        try {
            MemorySegment g = getFn != null ? getFn : MemorySegment.NULL;
            MemorySegment s = setFn != null ? setFn : MemorySegment.NULL;
            requireSymbols().setPlayerFoodHooks.invoke(g, s);
        } catch (Throwable t) {
            throw new IllegalStateException(
                    "leaf_bridge_set_player_food_hooks failed", t);
        }
    }

    public static void setPlayerGamemodeHooks(MemorySegment getFn, MemorySegment setFn) {
        try {
            MemorySegment g = getFn != null ? getFn : MemorySegment.NULL;
            MemorySegment s = setFn != null ? setFn : MemorySegment.NULL;
            requireSymbols().setPlayerGamemodeHooks.invoke(g, s);
        } catch (Throwable t) {
            throw new IllegalStateException(
                    "leaf_bridge_set_player_gamemode_hooks failed", t);
        }
    }

    public static void setPlayerXpHooks(MemorySegment getFn, MemorySegment setFn) {
        try {
            MemorySegment g = getFn != null ? getFn : MemorySegment.NULL;
            MemorySegment s = setFn != null ? setFn : MemorySegment.NULL;
            requireSymbols().setPlayerXpHooks.invoke(g, s);
        } catch (Throwable t) {
            throw new IllegalStateException(
                    "leaf_bridge_set_player_xp_hooks failed", t);
        }
    }

    public static void setPlayerLookHooks(MemorySegment getFn, MemorySegment setFn) {
        try {
            MemorySegment g = getFn != null ? getFn : MemorySegment.NULL;
            MemorySegment s = setFn != null ? setFn : MemorySegment.NULL;
            requireSymbols().setPlayerLookHooks.invoke(g, s);
        } catch (Throwable t) {
            throw new IllegalStateException(
                    "leaf_bridge_set_player_look_hooks failed", t);
        }
    }

    public static void setPlaySoundHook(MemorySegment fn) {
        try {
            MemorySegment f = fn != null ? fn : MemorySegment.NULL;
            requireSymbols().setPlaySoundHook.invoke(f);
        } catch (Throwable t) {
            throw new IllegalStateException(
                    "leaf_bridge_set_play_sound_hook failed", t);
        }
    }

    public static void setActionbarHook(MemorySegment fn) {
        try {
            MemorySegment f = fn != null ? fn : MemorySegment.NULL;
            requireSymbols().setActionbarHook.invoke(f);
        } catch (Throwable t) {
            throw new IllegalStateException(
                    "leaf_bridge_set_actionbar_hook failed", t);
        }
    }

    public static void setTitleHook(MemorySegment fn) {
        try {
            MemorySegment f = fn != null ? fn : MemorySegment.NULL;
            requireSymbols().setTitleHook.invoke(f);
        } catch (Throwable t) {
            throw new IllegalStateException(
                    "leaf_bridge_set_title_hook failed", t);
        }
    }

    public static void setKickPlayerHook(MemorySegment fn) {
        try {
            MemorySegment f = fn != null ? fn : MemorySegment.NULL;
            requireSymbols().setKickPlayerHook.invoke(f);
        } catch (Throwable t) {
            throw new IllegalStateException(
                    "leaf_bridge_set_kick_player_hook failed", t);
        }
    }

    public static void setGiveItemHook(MemorySegment fn) {
        try {
            MemorySegment f = fn != null ? fn : MemorySegment.NULL;
            requireSymbols().setGiveItemHook.invoke(f);
        } catch (Throwable t) {
            throw new IllegalStateException(
                    "leaf_bridge_set_give_item_hook failed", t);
        }
    }

    public static void setEffectHooks(MemorySegment applyFn, MemorySegment clearFn) {
        try {
            MemorySegment a = applyFn != null ? applyFn : MemorySegment.NULL;
            MemorySegment c = clearFn != null ? clearFn : MemorySegment.NULL;
            requireSymbols().setEffectHooks.invoke(a, c);
        } catch (Throwable t) {
            throw new IllegalStateException(
                    "leaf_bridge_set_effect_hooks failed", t);
        }
    }

    public static void setSpawnParticleHook(MemorySegment fn) {
        try {
            MemorySegment f = fn != null ? fn : MemorySegment.NULL;
            requireSymbols().setSpawnParticleHook.invoke(f);
        } catch (Throwable t) {
            throw new IllegalStateException(
                    "leaf_bridge_set_spawn_particle_hook failed", t);
        }
    }

    public static void setWorldTimeHooks(MemorySegment getFn, MemorySegment setFn) {
        try {
            MemorySegment g = getFn != null ? getFn : MemorySegment.NULL;
            MemorySegment s = setFn != null ? setFn : MemorySegment.NULL;
            requireSymbols().setWorldTimeHooks.invoke(g, s);
        } catch (Throwable t) {
            throw new IllegalStateException(
                    "leaf_bridge_set_world_time_hooks failed", t);
        }
    }

    public static void setPlayerVelocityHooks(MemorySegment getFn, MemorySegment setFn) {
        try {
            MemorySegment g = getFn != null ? getFn : MemorySegment.NULL;
            MemorySegment s = setFn != null ? setFn : MemorySegment.NULL;
            requireSymbols().setPlayerVelocityHooks.invoke(g, s);
        } catch (Throwable t) {
            throw new IllegalStateException(
                    "leaf_bridge_set_player_velocity_hooks failed", t);
        }
    }

    public static void setPlayerFlagsHook(MemorySegment fn) {
        try {
            MemorySegment f = fn != null ? fn : MemorySegment.NULL;
            requireSymbols().setPlayerFlagsHook.invoke(f);
        } catch (Throwable t) {
            throw new IllegalStateException(
                    "leaf_bridge_set_player_flags_hook failed", t);
        }
    }

    public static void setRunCommandHook(MemorySegment fn) {
        try {
            MemorySegment f = fn != null ? fn : MemorySegment.NULL;
            requireSymbols().setRunCommandHook.invoke(f);
        } catch (Throwable t) {
            throw new IllegalStateException(
                    "leaf_bridge_set_run_command_hook failed", t);
        }
    }

    public static void setClearInventoryHook(MemorySegment fn) {
        try {
            MemorySegment f = fn != null ? fn : MemorySegment.NULL;
            requireSymbols().setClearInventoryHook.invoke(f);
        } catch (Throwable t) {
            throw new IllegalStateException(
                    "leaf_bridge_set_clear_inventory_hook failed", t);
        }
    }

    public static void setPlayerFlightHook(MemorySegment fn) {
        try {
            MemorySegment f = fn != null ? fn : MemorySegment.NULL;
            requireSymbols().setPlayerFlightHook.invoke(f);
        } catch (Throwable t) {
            throw new IllegalStateException(
                    "leaf_bridge_set_player_flight_hook failed", t);
        }
    }

    public static void setGetBiomeHook(MemorySegment fn) {
        try {
            MemorySegment f = fn != null ? fn : MemorySegment.NULL;
            requireSymbols().setGetBiomeHook.invoke(f);
        } catch (Throwable t) {
            throw new IllegalStateException(
                    "leaf_bridge_set_get_biome_hook failed", t);
        }
    }

    public static void setDifficultyHooks(MemorySegment getFn, MemorySegment setFn) {
        try {
            MemorySegment g = getFn != null ? getFn : MemorySegment.NULL;
            MemorySegment s = setFn != null ? setFn : MemorySegment.NULL;
            requireSymbols().setDifficultyHooks.invoke(g, s);
        } catch (Throwable t) {
            throw new IllegalStateException(
                    "leaf_bridge_set_difficulty_hooks failed", t);
        }
    }

    public static void setWeatherHooks(MemorySegment getFn, MemorySegment setFn) {
        try {
            MemorySegment g = getFn != null ? getFn : MemorySegment.NULL;
            MemorySegment s = setFn != null ? setFn : MemorySegment.NULL;
            requireSymbols().setWeatherHooks.invoke(g, s);
        } catch (Throwable t) {
            throw new IllegalStateException(
                    "leaf_bridge_set_weather_hooks failed", t);
        }
    }

    public static void setGetLightLevelHook(MemorySegment fn) {
        try {
            MemorySegment f = fn != null ? fn : MemorySegment.NULL;
            requireSymbols().setGetLightLevelHook.invoke(f);
        } catch (Throwable t) {
            throw new IllegalStateException(
                    "leaf_bridge_set_get_light_level_hook failed", t);
        }
    }

    public static void setPlayerLatencyHook(MemorySegment fn) {
        try {
            MemorySegment f = fn != null ? fn : MemorySegment.NULL;
            requireSymbols().setPlayerLatencyHook.invoke(f);
        } catch (Throwable t) {
            throw new IllegalStateException(
                    "leaf_bridge_set_player_latency_hook failed", t);
        }
    }

    public static void setWorldSpawnHooks(MemorySegment getFn, MemorySegment setFn) {
        try {
            MemorySegment g = getFn != null ? getFn : MemorySegment.NULL;
            MemorySegment s = setFn != null ? setFn : MemorySegment.NULL;
            requireSymbols().setWorldSpawnHooks.invoke(g, s);
        } catch (Throwable t) {
            throw new IllegalStateException(
                    "leaf_bridge_set_world_spawn_hooks failed", t);
        }
    }

    public static void setPlayerOpHook(MemorySegment fn) {
        try {
            MemorySegment f = fn != null ? fn : MemorySegment.NULL;
            requireSymbols().setPlayerOpHook.invoke(f);
        } catch (Throwable t) {
            throw new IllegalStateException(
                    "leaf_bridge_set_player_op_hook failed", t);
        }
    }

    public static void setPlayerUuidHook(MemorySegment fn) {
        try {
            MemorySegment f = fn != null ? fn : MemorySegment.NULL;
            requireSymbols().setPlayerUuidHook.invoke(f);
        } catch (Throwable t) {
            throw new IllegalStateException(
                    "leaf_bridge_set_player_uuid_hook failed", t);
        }
    }

    public static void setPlayerPermissionHook(MemorySegment fn) {
        try {
            MemorySegment f = fn != null ? fn : MemorySegment.NULL;
            requireSymbols().setPlayerPermissionHook.invoke(f);
        } catch (Throwable t) {
            throw new IllegalStateException(
                    "leaf_bridge_set_player_permission_hook failed", t);
        }
    }

    public static void setFindPlayerUuidHook(MemorySegment fn) {
        try {
            MemorySegment f = fn != null ? fn : MemorySegment.NULL;
            requireSymbols().setFindPlayerUuidHook.invoke(f);
        } catch (Throwable t) {
            throw new IllegalStateException(
                    "leaf_bridge_set_find_player_uuid_hook failed", t);
        }
    }

    public static void setFindPlayerNameHook(MemorySegment fn) {
        try {
            MemorySegment f = fn != null ? fn : MemorySegment.NULL;
            requireSymbols().setFindPlayerNameHook.invoke(f);
        } catch (Throwable t) {
            throw new IllegalStateException(
                    "leaf_bridge_set_find_player_name_hook failed", t);
        }
    }

    public static void setBlockRegistryHooks(MemorySegment getFn, MemorySegment setFn) {
        try {
            MemorySegment g = getFn != null ? getFn : MemorySegment.NULL;
            MemorySegment s = setFn != null ? setFn : MemorySegment.NULL;
            requireSymbols().setBlockRegistryHooks.invoke(g, s);
        } catch (Throwable t) {
            throw new IllegalStateException(
                    "leaf_bridge_set_block_registry_hooks failed", t);
        }
    }

    public static void setGiveItemRegistryHook(MemorySegment fn) {
        try {
            MemorySegment f = fn != null ? fn : MemorySegment.NULL;
            requireSymbols().setGiveItemRegistryHook.invoke(f);
        } catch (Throwable t) {
            throw new IllegalStateException(
                    "leaf_bridge_set_give_item_registry_hook failed", t);
        }
    }

    public static void setInventoryRegistryHooks(MemorySegment getFn, MemorySegment setFn) {
        try {
            MemorySegment g = getFn != null ? getFn : MemorySegment.NULL;
            MemorySegment s = setFn != null ? setFn : MemorySegment.NULL;
            requireSymbols().setInventoryRegistryHooks.invoke(g, s);
        } catch (Throwable t) {
            throw new IllegalStateException(
                    "leaf_bridge_set_inventory_registry_hooks failed", t);
        }
    }

    public static void setTeleportHook(MemorySegment fn) {
        try {
            MemorySegment f = fn != null ? fn : MemorySegment.NULL;
            requireSymbols().setTeleportHook.invoke(f);
        } catch (Throwable t) {
            throw new IllegalStateException(
                    "leaf_bridge_set_teleport_hook failed", t);
        }
    }

    public static void setSelectedSlotHooks(MemorySegment getFn, MemorySegment setFn) {
        try {
            MemorySegment g = getFn != null ? getFn : MemorySegment.NULL;
            MemorySegment s = setFn != null ? setFn : MemorySegment.NULL;
            requireSymbols().setSelectedSlotHooks.invoke(g, s);
        } catch (Throwable t) {
            throw new IllegalStateException(
                    "leaf_bridge_set_selected_slot_hooks failed", t);
        }
    }

    public static void setGetWorldSeedHook(MemorySegment fn) {
        try {
            MemorySegment f = fn != null ? fn : MemorySegment.NULL;
            requireSymbols().setGetWorldSeedHook.invoke(f);
        } catch (Throwable t) {
            throw new IllegalStateException(
                    "leaf_bridge_set_get_world_seed_hook failed", t);
        }
    }

    public static void setPlayerAbsorptionHooks(MemorySegment getFn, MemorySegment setFn) {
        try {
            MemorySegment g = getFn != null ? getFn : MemorySegment.NULL;
            MemorySegment s = setFn != null ? setFn : MemorySegment.NULL;
            requireSymbols().setPlayerAbsorptionHooks.invoke(g, s);
        } catch (Throwable t) {
            throw new IllegalStateException(
                    "leaf_bridge_set_player_absorption_hooks failed", t);
        }
    }

    public static void setPlayerInvulnerableHooks(MemorySegment getFn, MemorySegment setFn) {
        try {
            MemorySegment g = getFn != null ? getFn : MemorySegment.NULL;
            MemorySegment s = setFn != null ? setFn : MemorySegment.NULL;
            requireSymbols().setPlayerInvulnerableHooks.invoke(g, s);
        } catch (Throwable t) {
            throw new IllegalStateException(
                    "leaf_bridge_set_player_invulnerable_hooks failed", t);
        }
    }

    public static void setPlayerAirHooks(MemorySegment getFn, MemorySegment setFn) {
        try {
            MemorySegment g = getFn != null ? getFn : MemorySegment.NULL;
            MemorySegment s = setFn != null ? setFn : MemorySegment.NULL;
            requireSymbols().setPlayerAirHooks.invoke(g, s);
        } catch (Throwable t) {
            throw new IllegalStateException(
                    "leaf_bridge_set_player_air_hooks failed", t);
        }
    }

    public static void setPlayerFireTicksHooks(MemorySegment getFn, MemorySegment setFn) {
        try {
            MemorySegment g = getFn != null ? getFn : MemorySegment.NULL;
            MemorySegment s = setFn != null ? setFn : MemorySegment.NULL;
            requireSymbols().setPlayerFireTicksHooks.invoke(g, s);
        } catch (Throwable t) {
            throw new IllegalStateException(
                    "leaf_bridge_set_player_fire_ticks_hooks failed", t);
        }
    }

    public static void setPlayerFrozenTicksHooks(MemorySegment getFn, MemorySegment setFn) {
        try {
            MemorySegment g = getFn != null ? getFn : MemorySegment.NULL;
            MemorySegment s = setFn != null ? setFn : MemorySegment.NULL;
            requireSymbols().setPlayerFrozenTicksHooks.invoke(g, s);
        } catch (Throwable t) {
            throw new IllegalStateException(
                    "leaf_bridge_set_player_frozen_ticks_hooks failed", t);
        }
    }

    public static void setPlayerNoGravityHooks(MemorySegment getFn, MemorySegment setFn) {
        try {
            MemorySegment g = getFn != null ? getFn : MemorySegment.NULL;
            MemorySegment s = setFn != null ? setFn : MemorySegment.NULL;
            requireSymbols().setPlayerNoGravityHooks.invoke(g, s);
        } catch (Throwable t) {
            throw new IllegalStateException(
                    "leaf_bridge_set_player_no_gravity_hooks failed", t);
        }
    }

    public static void setPlayerSilentHooks(MemorySegment getFn, MemorySegment setFn) {
        try {
            MemorySegment g = getFn != null ? getFn : MemorySegment.NULL;
            MemorySegment s = setFn != null ? setFn : MemorySegment.NULL;
            requireSymbols().setPlayerSilentHooks.invoke(g, s);
        } catch (Throwable t) {
            throw new IllegalStateException(
                    "leaf_bridge_set_player_silent_hooks failed", t);
        }
    }

    public static void setPlayerGlowingHooks(MemorySegment getFn, MemorySegment setFn) {
        try {
            MemorySegment g = getFn != null ? getFn : MemorySegment.NULL;
            MemorySegment s = setFn != null ? setFn : MemorySegment.NULL;
            requireSymbols().setPlayerGlowingHooks.invoke(g, s);
        } catch (Throwable t) {
            throw new IllegalStateException(
                    "leaf_bridge_set_player_glowing_hooks failed", t);
        }
    }

    public static void setPlayerInvisibleHooks(MemorySegment getFn, MemorySegment setFn) {
        try {
            MemorySegment g = getFn != null ? getFn : MemorySegment.NULL;
            MemorySegment s = setFn != null ? setFn : MemorySegment.NULL;
            requireSymbols().setPlayerInvisibleHooks.invoke(g, s);
        } catch (Throwable t) {
            throw new IllegalStateException(
                    "leaf_bridge_set_player_invisible_hooks failed", t);
        }
    }

    public static void setPlayerPortalCooldownHooks(MemorySegment getFn, MemorySegment setFn) {
        try {
            MemorySegment g = getFn != null ? getFn : MemorySegment.NULL;
            MemorySegment s = setFn != null ? setFn : MemorySegment.NULL;
            requireSymbols().setPlayerPortalCooldownHooks.invoke(g, s);
        } catch (Throwable t) {
            throw new IllegalStateException(
                    "leaf_bridge_set_player_portal_cooldown_hooks failed", t);
        }
    }

    public static void setGetPlayerMaxAirHook(MemorySegment fn) {
        try {
            MemorySegment f = fn != null ? fn : MemorySegment.NULL;
            requireSymbols().setGetPlayerMaxAirHook.invoke(f);
        } catch (Throwable t) {
            throw new IllegalStateException(
                    "leaf_bridge_set_get_player_max_air_hook failed", t);
        }
    }

    public static int registerPlayer(long playerHandle, String name) {
        try (Arena arena = Arena.ofConfined()) {
            MemorySegment nameSeg = arena.allocateFrom(
                    name != null ? name : "", StandardCharsets.UTF_8);
            return (int) requireSymbols().registerPlayer.invokeExact(
                    playerHandle, nameSeg);
        } catch (Throwable t) {
            throw new IllegalStateException("leaf_bridge_register_player failed", t);
        }
    }

    public static int unregisterPlayer(long playerHandle) {
        try {
            return (int) requireSymbols().unregisterPlayer.invokeExact(playerHandle);
        } catch (Throwable t) {
            throw new IllegalStateException("leaf_bridge_unregister_player failed", t);
        }
    }

    public static boolean isOk(int code) {
        return code == 0;
    }

    public static String formatError(int code) {
        return String.format("0x%08X", code);
    }

    public static void requireOk(String step, int code) {
        if (!isOk(code)) {
            throw new IllegalStateException(step + " failed: " + formatError(code));
        }
    }
}
