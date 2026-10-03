package dev.leafmc.bridge;

import java.lang.foreign.Arena;
import java.lang.foreign.FunctionDescriptor;
import java.lang.foreign.Linker;
import java.lang.foreign.MemorySegment;
import java.lang.foreign.ValueLayout;
import java.lang.invoke.MethodHandle;
import java.lang.invoke.MethodHandles;
import java.lang.invoke.MethodType;
import java.nio.charset.StandardCharsets;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.function.BiConsumer;
import java.util.function.Consumer;

/**
 * Registers FFM upcalls so native Leaf Mods can reach the live game
 * (chat, broadcast, inventory, world block queries).
 */
public final class LeafMinecraftHooks {
    @FunctionalInterface
    public interface InventoryGet {
        /** @return 0 on success */
        int get(long playerHandle, int slot, int[] itemIdOut, int[] countOut);
    }

    @FunctionalInterface
    public interface InventorySet {
        /** @return 0 on success */
        int set(long playerHandle, int slot, int itemId, int count);
    }

    @FunctionalInterface
    public interface InventoryStackGet {
        /** @return 0 on success; damageOut[0] = -1 when unset */
        int get(long playerHandle, int slot, int[] itemIdOut, int[] countOut, int[] damageOut);
    }

    @FunctionalInterface
    public interface InventoryStackSet {
        int set(long playerHandle, int slot, int itemId, int count, int damage);
    }

    @FunctionalInterface
    public interface InventoryRegistryGet {
        /** @return 0 on success; write registry id into outId[0] */
        int get(
                long playerHandle,
                int slot,
                String[] outId,
                int[] countOut,
                int[] damageOut);
    }

    @FunctionalInterface
    public interface InventoryRegistrySet {
        /** @return 0 on success */
        int set(long playerHandle, int slot, String itemId, int count, int damage);
    }

    @FunctionalInterface
    public interface TeleportPlayer {
        /** @return 0 on success */
        int teleport(
                long playerHandle,
                int x,
                int y,
                int z,
                int dimension,
                float yaw,
                float pitch);
    }

    @FunctionalInterface
    public interface SelectedSlotGet {
        /** @return 0 on success; write 0–8 into slotOut[0] */
        int get(long playerHandle, int[] slotOut);
    }

    @FunctionalInterface
    public interface SelectedSlotSet {
        /** @return 0 on success; slot 0–8 */
        int set(long playerHandle, int slot);
    }

    @FunctionalInterface
    public interface GetWorldSeed {
        /** @return 0 on success; write seed into seedOut[0] */
        int get(int dimension, long[] seedOut);
    }

    @FunctionalInterface
    public interface PlayerAbsorptionGet {
        /** @return 0 on success; write absorption into absOut[0] */
        int get(long playerHandle, float[] absOut);
    }

    @FunctionalInterface
    public interface PlayerAbsorptionSet {
        /** @return 0 on success */
        int set(long playerHandle, float absorption);
    }

    @FunctionalInterface
    public interface PlayerInvulnerableGet {
        /** @return 0 on success; write 0/1 into out[0] */
        int get(long playerHandle, int[] out);
    }

    @FunctionalInterface
    public interface PlayerInvulnerableSet {
        /** @return 0 on success; invulnerable 0/1 */
        int set(long playerHandle, int invulnerable);
    }

    @FunctionalInterface
    public interface PlayerAirGet {
        /** @return 0 on success; write air ticks into out[0] */
        int get(long playerHandle, int[] out);
    }

    @FunctionalInterface
    public interface PlayerAirSet {
        /** @return 0 on success */
        int set(long playerHandle, int air);
    }

    @FunctionalInterface
    public interface PlayerFireTicksGet {
        /** @return 0 on success; write remaining fire ticks into out[0] */
        int get(long playerHandle, int[] out);
    }

    @FunctionalInterface
    public interface PlayerFireTicksSet {
        /** @return 0 on success */
        int set(long playerHandle, int ticks);
    }

    @FunctionalInterface
    public interface PlayerFrozenTicksGet {
        /** @return 0 on success; write frozen ticks into out[0] */
        int get(long playerHandle, int[] out);
    }

    @FunctionalInterface
    public interface PlayerFrozenTicksSet {
        /** @return 0 on success */
        int set(long playerHandle, int ticks);
    }

    @FunctionalInterface
    public interface PlayerNoGravityGet {
        /** @return 0 on success; write 0/1 into out[0] */
        int get(long playerHandle, int[] out);
    }

    @FunctionalInterface
    public interface PlayerNoGravitySet {
        /** @return 0 on success; no_gravity 0/1 */
        int set(long playerHandle, int noGravity);
    }

    @FunctionalInterface
    public interface PlayerSilentGet {
        /** @return 0 on success; write 0/1 into out[0] */
        int get(long playerHandle, int[] out);
    }

    @FunctionalInterface
    public interface PlayerSilentSet {
        /** @return 0 on success; silent 0/1 */
        int set(long playerHandle, int silent);
    }

    @FunctionalInterface
    public interface PlayerGlowingGet {
        /** @return 0 on success; write 0/1 into out[0] */
        int get(long playerHandle, int[] out);
    }

    @FunctionalInterface
    public interface PlayerGlowingSet {
        /** @return 0 on success; glowing 0/1 */
        int set(long playerHandle, int glowing);
    }

    @FunctionalInterface
    public interface PlayerInvisibleGet {
        /** @return 0 on success; write 0/1 into out[0] */
        int get(long playerHandle, int[] out);
    }

    @FunctionalInterface
    public interface PlayerInvisibleSet {
        /** @return 0 on success; invisible 0/1 */
        int set(long playerHandle, int invisible);
    }

    @FunctionalInterface
    public interface PlayerPortalCooldownGet {
        /** @return 0 on success; write portal cooldown ticks into out[0] */
        int get(long playerHandle, int[] out);
    }

    @FunctionalInterface
    public interface PlayerPortalCooldownSet {
        /** @return 0 on success; portal cooldown ticks */
        int set(long playerHandle, int ticks);
    }

    @FunctionalInterface
    public interface GetPlayerMaxAir {
        /** @return 0 on success; write max air ticks into out[0] */
        int get(long playerHandle, int[] out);
    }

    @FunctionalInterface
    public interface BlockGet {
        /** @return 0 on success; dimension 0=overworld, 1=nether, 2=end */
        int get(int dimension, int x, int y, int z, int[] blockIdOut);
    }

    @FunctionalInterface
    public interface BlockSet {
        /** @return 0 on success */
        int set(int dimension, int x, int y, int z, int blockId);
    }

    @FunctionalInterface
    public interface PlayerPosGet {
        /** @return 0 on success; dimension 0=overworld, 1=nether, 2=end */
        int get(long playerHandle, int[] xOut, int[] yOut, int[] zOut, int[] dimOut);
    }

    @FunctionalInterface
    public interface PlayerPosSet {
        /** @return 0 on success */
        int set(long playerHandle, int x, int y, int z, int dimension);
    }

    @FunctionalInterface
    public interface PlayerHealthGet {
        /** @return 0 on success */
        int get(long playerHandle, float[] healthOut, float[] maxHealthOut);
    }

    @FunctionalInterface
    public interface PlayerHealthSet {
        /** @return 0 on success */
        int set(long playerHandle, float health);
    }

    @FunctionalInterface
    public interface PlayerFoodGet {
        /** @return 0 on success */
        int get(long playerHandle, int[] foodOut, float[] saturationOut);
    }

    @FunctionalInterface
    public interface PlayerFoodSet {
        /** @return 0 on success */
        int set(long playerHandle, int food, float saturation);
    }

    @FunctionalInterface
    public interface PlayerGamemodeGet {
        /** @return 0 on success; mode 0–3 */
        int get(long playerHandle, int[] modeOut);
    }

    @FunctionalInterface
    public interface PlayerGamemodeSet {
        /** @return 0 on success */
        int set(long playerHandle, int mode);
    }

    @FunctionalInterface
    public interface PlayerXpGet {
        /** @return 0 on success */
        int get(long playerHandle, int[] levelOut, float[] progressOut);
    }

    @FunctionalInterface
    public interface PlayerXpSet {
        /** @return 0 on success */
        int set(long playerHandle, int level);
    }

    @FunctionalInterface
    public interface PlayerLookGet {
        /** @return 0 on success */
        int get(long playerHandle, float[] yawOut, float[] pitchOut);
    }

    @FunctionalInterface
    public interface PlayerLookSet {
        /** @return 0 on success */
        int set(long playerHandle, float yaw, float pitch);
    }

    @FunctionalInterface
    public interface PlaySound {
        /**
         * Play {@code soundId} for {@code playerHandle} (0 = world at coords).
         * @return 0 on success
         */
        int play(
                long playerHandle,
                String soundId,
                float volume,
                float pitch,
                int x,
                int y,
                int z,
                int dimension);
    }

    @FunctionalInterface
    public interface ActionbarSend {
        /** @return 0 on success */
        int send(long playerHandle, String message);
    }

    @FunctionalInterface
    public interface TitleSend {
        /** @return 0 on success */
        int send(
                long playerHandle,
                String title,
                String subtitle,
                int fadeInTicks,
                int stayTicks,
                int fadeOutTicks);
    }

    @FunctionalInterface
    public interface KickPlayer {
        /** @return 0 on success */
        int kick(long playerHandle, String reason);
    }

    @FunctionalInterface
    public interface GiveItem {
        /** @return 0 on success */
        int give(long playerHandle, int itemId, int count, int damage);
    }

    @FunctionalInterface
    public interface GiveItemRegistry {
        /** @return 0 on success */
        int give(long playerHandle, String itemId, int count, int damage);
    }

    @FunctionalInterface
    public interface ApplyEffect {
        /** @return 0 on success */
        int apply(
                long playerHandle,
                String effectId,
                int durationTicks,
                int amplifier,
                int flags);
    }

    @FunctionalInterface
    public interface ClearEffects {
        /** @return 0 on success */
        int clear(long playerHandle);
    }

    @FunctionalInterface
    public interface SpawnParticle {
        /** @return 0 on success */
        int spawn(
                String particleId,
                double x,
                double y,
                double z,
                int dimension,
                int count,
                double dx,
                double dy,
                double dz,
                double speed);
    }

    @FunctionalInterface
    public interface WorldTimeGet {
        /** @return 0 on success */
        int get(int dimension, long[] timeOut);
    }

    @FunctionalInterface
    public interface WorldTimeSet {
        /** @return 0 on success */
        int set(int dimension, long time);
    }

    @FunctionalInterface
    public interface PlayerVelocityGet {
        /** @return 0 on success */
        int get(long playerHandle, double[] vxOut, double[] vyOut, double[] vzOut);
    }

    @FunctionalInterface
    public interface PlayerVelocitySet {
        /** @return 0 on success */
        int set(long playerHandle, double vx, double vy, double vz);
    }

    @FunctionalInterface
    public interface PlayerFlagsGet {
        /** @return 0 on success; flags use LEAF_PLAYER_FLAG_* bits */
        int get(long playerHandle, int[] flagsOut);
    }

    @FunctionalInterface
    public interface RunCommand {
        /** playerHandle 0 = console. @return 0 on success */
        int run(long playerHandle, String command);
    }

    @FunctionalInterface
    public interface ClearInventory {
        /** @return 0 on success */
        int clear(long playerHandle);
    }

    @FunctionalInterface
    public interface PlayerFlight {
        /** allowFlight/flying: 0=off, 1=on, -1=unchanged. @return 0 on success */
        int set(long playerHandle, int allowFlight, int flying);
    }

    @FunctionalInterface
    public interface GetBiome {
        /** @return 0 on success; write registry id into outId[0] */
        int get(int dimension, int x, int y, int z, String[] outId);
    }

    @FunctionalInterface
    public interface DifficultyGet {
        /** @return 0 on success; 0 peaceful .. 3 hard */
        int get(int[] difficultyOut);
    }

    @FunctionalInterface
    public interface DifficultySet {
        /** @return 0 on success; 0 peaceful .. 3 hard */
        int set(int difficulty);
    }

    @FunctionalInterface
    public interface WeatherGet {
        /** @return 0 on success; 0 clear, 1 rain, 2 thunder */
        int get(int dimension, int[] weatherOut);
    }

    @FunctionalInterface
    public interface WeatherSet {
        /** @return 0 on success; durationTicks 0 = default */
        int set(int dimension, int weather, int durationTicks);
    }

    @FunctionalInterface
    public interface GetLightLevel {
        /** @return 0 on success; write block/sky 0–15 into out arrays */
        int get(
                int dimension,
                int x,
                int y,
                int z,
                int[] blockLightOut,
                int[] skyLightOut);
    }

    @FunctionalInterface
    public interface PlayerLatency {
        /** @return 0 on success; write latency ms into latencyOut[0] */
        int get(long playerHandle, int[] latencyOut);
    }

    @FunctionalInterface
    public interface WorldSpawnGet {
        /** @return 0 on success */
        int get(int dimension, int[] xOut, int[] yOut, int[] zOut);
    }

    @FunctionalInterface
    public interface WorldSpawnSet {
        /** @return 0 on success */
        int set(int dimension, int x, int y, int z);
    }

    @FunctionalInterface
    public interface PlayerOp {
        /** @return 0 on success; write 1/0 into opOut[0] */
        int get(long playerHandle, int[] opOut);
    }

    @FunctionalInterface
    public interface PlayerUuid {
        /** @return 0 on success; write hyphenated UUID into outUuid[0] */
        int get(long playerHandle, String[] outUuid);
    }

    @FunctionalInterface
    public interface PlayerPermission {
        /** @return 0 on success; write 0–4 into levelOut[0] */
        int get(long playerHandle, int[] levelOut);
    }

    @FunctionalInterface
    public interface FindPlayerUuid {
        /** @return 0 on success; write handle into handleOut[0] */
        int find(String uuid, long[] handleOut);
    }

    @FunctionalInterface
    public interface FindPlayerName {
        /** @return 0 on success; write handle into handleOut[0] */
        int find(String name, long[] handleOut);
    }

    @FunctionalInterface
    public interface GetBlockRegistry {
        /** @return 0 on success; write registry id into outId[0] */
        int get(int dimension, int x, int y, int z, String[] outId);
    }

    @FunctionalInterface
    public interface SetBlockRegistry {
        /** @return 0 on success */
        int set(int dimension, int x, int y, int z, String blockId);
    }

    private static final Map<Long, BiConsumer<Long, String>> SEND_HANDLERS =
            new ConcurrentHashMap<>();
    private static volatile boolean sendInstalled;
    private static volatile boolean broadcastInstalled;
    private static volatile boolean inventoryInstalled;
    private static volatile boolean inventoryStackInstalled;
    private static volatile boolean inventoryRegistryInstalled;
    private static volatile boolean teleportInstalled;
    private static volatile boolean selectedSlotInstalled;
    private static volatile boolean worldSeedInstalled;
    private static volatile boolean playerAbsorptionInstalled;
    private static volatile boolean playerInvulnerableInstalled;
    private static volatile boolean playerAirInstalled;
    private static volatile boolean playerFireTicksInstalled;
    private static volatile boolean playerFrozenTicksInstalled;
    private static volatile boolean playerNoGravityInstalled;
    private static volatile boolean playerSilentInstalled;
    private static volatile boolean playerGlowingInstalled;
    private static volatile boolean playerInvisibleInstalled;
    private static volatile boolean playerPortalCooldownInstalled;
    private static volatile boolean playerMaxAirInstalled;
    private static volatile boolean worldInstalled;
    private static volatile boolean playerPosInstalled;
    private static volatile boolean playerHealthInstalled;
    private static volatile boolean playerFoodInstalled;
    private static volatile boolean playerGamemodeInstalled;
    private static volatile boolean playerXpInstalled;
    private static volatile boolean playerLookInstalled;
    private static volatile boolean playSoundInstalled;
    private static volatile boolean actionbarInstalled;
    private static volatile boolean titleInstalled;
    private static volatile boolean kickInstalled;
    private static volatile boolean giveItemInstalled;
    private static volatile boolean giveItemRegistryInstalled;
    private static volatile boolean effectInstalled;
    private static volatile boolean spawnParticleInstalled;
    private static volatile boolean worldTimeInstalled;
    private static volatile boolean playerVelocityInstalled;
    private static volatile boolean playerFlagsInstalled;
    private static volatile boolean runCommandInstalled;
    private static volatile boolean clearInventoryInstalled;
    private static volatile boolean playerFlightInstalled;
    private static volatile boolean getBiomeInstalled;
    private static volatile boolean difficultyInstalled;
    private static volatile boolean weatherInstalled;
    private static volatile boolean getLightLevelInstalled;
    private static volatile boolean playerLatencyInstalled;
    private static volatile boolean worldSpawnInstalled;
    private static volatile boolean playerOpInstalled;
    private static volatile boolean playerUuidInstalled;
    private static volatile boolean playerPermissionInstalled;
    private static volatile boolean findPlayerUuidInstalled;
    private static volatile boolean findPlayerNameInstalled;
    private static volatile boolean blockRegistryInstalled;
    private static volatile BiConsumer<Long, String> defaultSend;
    private static volatile Consumer<String> broadcast;
    private static volatile InventoryGet inventoryGet;
    private static volatile InventorySet inventorySet;
    private static volatile InventoryStackGet inventoryStackGet;
    private static volatile InventoryStackSet inventoryStackSet;
    private static volatile InventoryRegistryGet inventoryRegistryGet;
    private static volatile InventoryRegistrySet inventoryRegistrySet;
    private static volatile TeleportPlayer teleportPlayer;
    private static volatile SelectedSlotGet selectedSlotGet;
    private static volatile SelectedSlotSet selectedSlotSet;
    private static volatile GetWorldSeed getWorldSeed;
    private static volatile PlayerAbsorptionGet playerAbsorptionGet;
    private static volatile PlayerAbsorptionSet playerAbsorptionSet;
    private static volatile PlayerInvulnerableGet playerInvulnerableGet;
    private static volatile PlayerInvulnerableSet playerInvulnerableSet;
    private static volatile PlayerAirGet playerAirGet;
    private static volatile PlayerAirSet playerAirSet;
    private static volatile PlayerFireTicksGet playerFireTicksGet;
    private static volatile PlayerFireTicksSet playerFireTicksSet;
    private static volatile PlayerFrozenTicksGet playerFrozenTicksGet;
    private static volatile PlayerFrozenTicksSet playerFrozenTicksSet;
    private static volatile PlayerNoGravityGet playerNoGravityGet;
    private static volatile PlayerNoGravitySet playerNoGravitySet;
    private static volatile PlayerSilentGet playerSilentGet;
    private static volatile PlayerSilentSet playerSilentSet;
    private static volatile PlayerGlowingGet playerGlowingGet;
    private static volatile PlayerGlowingSet playerGlowingSet;
    private static volatile PlayerInvisibleGet playerInvisibleGet;
    private static volatile PlayerInvisibleSet playerInvisibleSet;
    private static volatile PlayerPortalCooldownGet playerPortalCooldownGet;
    private static volatile PlayerPortalCooldownSet playerPortalCooldownSet;
    private static volatile GetPlayerMaxAir getPlayerMaxAir;
    private static volatile BlockGet blockGet;
    private static volatile BlockSet blockSet;
    private static volatile PlayerPosGet playerPosGet;
    private static volatile PlayerPosSet playerPosSet;
    private static volatile PlayerHealthGet playerHealthGet;
    private static volatile PlayerHealthSet playerHealthSet;
    private static volatile PlayerFoodGet playerFoodGet;
    private static volatile PlayerFoodSet playerFoodSet;
    private static volatile PlayerGamemodeGet playerGamemodeGet;
    private static volatile PlayerGamemodeSet playerGamemodeSet;
    private static volatile PlayerXpGet playerXpGet;
    private static volatile PlayerXpSet playerXpSet;
    private static volatile PlayerLookGet playerLookGet;
    private static volatile PlayerLookSet playerLookSet;
    private static volatile PlaySound playSound;
    private static volatile ActionbarSend actionbarSend;
    private static volatile TitleSend titleSend;
    private static volatile KickPlayer kickPlayer;
    private static volatile GiveItem giveItem;
    private static volatile GiveItemRegistry giveItemRegistry;
    private static volatile ApplyEffect applyEffect;
    private static volatile ClearEffects clearEffects;
    private static volatile SpawnParticle spawnParticle;
    private static volatile WorldTimeGet worldTimeGet;
    private static volatile WorldTimeSet worldTimeSet;
    private static volatile PlayerVelocityGet playerVelocityGet;
    private static volatile PlayerVelocitySet playerVelocitySet;
    private static volatile PlayerFlagsGet playerFlagsGet;
    private static volatile RunCommand runCommand;
    private static volatile ClearInventory clearInventory;
    private static volatile PlayerFlight playerFlight;
    private static volatile GetBiome getBiome;
    private static volatile DifficultyGet difficultyGet;
    private static volatile DifficultySet difficultySet;
    private static volatile WeatherGet weatherGet;
    private static volatile WeatherSet weatherSet;
    private static volatile GetLightLevel getLightLevel;
    private static volatile PlayerLatency playerLatency;
    private static volatile WorldSpawnGet worldSpawnGet;
    private static volatile WorldSpawnSet worldSpawnSet;
    private static volatile PlayerOp playerOp;
    private static volatile PlayerUuid playerUuid;
    private static volatile PlayerPermission playerPermission;
    private static volatile FindPlayerUuid findPlayerUuid;
    private static volatile FindPlayerName findPlayerName;
    private static volatile GetBlockRegistry getBlockRegistry;
    private static volatile SetBlockRegistry setBlockRegistry;

    private LeafMinecraftHooks() {}

    /** Install a process-wide chat sender used when no per-handle handler exists. */
    public static synchronized void installDefaultSend(BiConsumer<Long, String> sender) {
        defaultSend = sender;
        ensureSendHook();
    }

    /** Install a process-wide broadcast handler (server chat to everyone). */
    public static synchronized void installBroadcast(Consumer<String> sender) {
        broadcast = sender;
        ensureBroadcastHook();
    }

    public static synchronized void installInventory(InventoryGet get, InventorySet set) {
        inventoryGet = get;
        inventorySet = set;
        ensureInventoryHooks();
    }

    public static synchronized void installInventoryStack(
            InventoryStackGet get, InventoryStackSet set) {
        inventoryStackGet = get;
        inventoryStackSet = set;
        ensureInventoryStackHooks();
    }

    public static synchronized void installInventoryRegistry(
            InventoryRegistryGet get, InventoryRegistrySet set) {
        inventoryRegistryGet = get;
        inventoryRegistrySet = set;
        ensureInventoryRegistryHooks();
    }

    public static synchronized void installTeleport(TeleportPlayer handler) {
        teleportPlayer = handler;
        ensureTeleportHook();
    }

    public static synchronized void installSelectedSlot(
            SelectedSlotGet get, SelectedSlotSet set) {
        selectedSlotGet = get;
        selectedSlotSet = set;
        ensureSelectedSlotHooks();
    }

    public static synchronized void installGetWorldSeed(GetWorldSeed handler) {
        getWorldSeed = handler;
        ensureGetWorldSeedHook();
    }

    public static synchronized void installPlayerAbsorption(
            PlayerAbsorptionGet get, PlayerAbsorptionSet set) {
        playerAbsorptionGet = get;
        playerAbsorptionSet = set;
        ensurePlayerAbsorptionHooks();
    }

    public static synchronized void installPlayerInvulnerable(
            PlayerInvulnerableGet get, PlayerInvulnerableSet set) {
        playerInvulnerableGet = get;
        playerInvulnerableSet = set;
        ensurePlayerInvulnerableHooks();
    }

    public static synchronized void installPlayerAir(
            PlayerAirGet get, PlayerAirSet set) {
        playerAirGet = get;
        playerAirSet = set;
        ensurePlayerAirHooks();
    }

    public static synchronized void installPlayerFireTicks(
            PlayerFireTicksGet get, PlayerFireTicksSet set) {
        playerFireTicksGet = get;
        playerFireTicksSet = set;
        ensurePlayerFireTicksHooks();
    }

    public static synchronized void installPlayerFrozenTicks(
            PlayerFrozenTicksGet get, PlayerFrozenTicksSet set) {
        playerFrozenTicksGet = get;
        playerFrozenTicksSet = set;
        ensurePlayerFrozenTicksHooks();
    }

    public static synchronized void installPlayerNoGravity(
            PlayerNoGravityGet get, PlayerNoGravitySet set) {
        playerNoGravityGet = get;
        playerNoGravitySet = set;
        ensurePlayerNoGravityHooks();
    }

    public static synchronized void installPlayerSilent(
            PlayerSilentGet get, PlayerSilentSet set) {
        playerSilentGet = get;
        playerSilentSet = set;
        ensurePlayerSilentHooks();
    }

    public static synchronized void installPlayerGlowing(
            PlayerGlowingGet get, PlayerGlowingSet set) {
        playerGlowingGet = get;
        playerGlowingSet = set;
        ensurePlayerGlowingHooks();
    }

    public static synchronized void installPlayerInvisible(
            PlayerInvisibleGet get, PlayerInvisibleSet set) {
        playerInvisibleGet = get;
        playerInvisibleSet = set;
        ensurePlayerInvisibleHooks();
    }

    public static synchronized void installPlayerPortalCooldown(
            PlayerPortalCooldownGet get, PlayerPortalCooldownSet set) {
        playerPortalCooldownGet = get;
        playerPortalCooldownSet = set;
        ensurePlayerPortalCooldownHooks();
    }

    public static synchronized void installGetPlayerMaxAir(GetPlayerMaxAir handler) {
        getPlayerMaxAir = handler;
        ensureGetPlayerMaxAirHook();
    }

    public static synchronized void installGetBlock(BlockGet get) {
        blockGet = get;
        ensureWorldHooks();
    }

    public static synchronized void installWorld(BlockGet get, BlockSet set) {
        blockGet = get;
        blockSet = set;
        ensureWorldHooks();
    }

    public static synchronized void installPlayerPos(PlayerPosGet get, PlayerPosSet set) {
        playerPosGet = get;
        playerPosSet = set;
        ensurePlayerPosHooks();
    }

    public static synchronized void installPlayerHealth(
            PlayerHealthGet get, PlayerHealthSet set) {
        playerHealthGet = get;
        playerHealthSet = set;
        ensurePlayerHealthHooks();
    }

    public static synchronized void installPlayerFood(PlayerFoodGet get, PlayerFoodSet set) {
        playerFoodGet = get;
        playerFoodSet = set;
        ensurePlayerFoodHooks();
    }

    public static synchronized void installPlayerGamemode(
            PlayerGamemodeGet get, PlayerGamemodeSet set) {
        playerGamemodeGet = get;
        playerGamemodeSet = set;
        ensurePlayerGamemodeHooks();
    }

    public static synchronized void installPlayerXp(PlayerXpGet get, PlayerXpSet set) {
        playerXpGet = get;
        playerXpSet = set;
        ensurePlayerXpHooks();
    }

    public static synchronized void installPlayerLook(PlayerLookGet get, PlayerLookSet set) {
        playerLookGet = get;
        playerLookSet = set;
        ensurePlayerLookHooks();
    }

    public static synchronized void installPlaySound(PlaySound handler) {
        playSound = handler;
        ensurePlaySoundHook();
    }

    public static synchronized void installActionbar(ActionbarSend handler) {
        actionbarSend = handler;
        ensureActionbarHook();
    }

    public static synchronized void installTitle(TitleSend handler) {
        titleSend = handler;
        ensureTitleHook();
    }

    public static synchronized void installKick(KickPlayer handler) {
        kickPlayer = handler;
        ensureKickHook();
    }

    public static synchronized void installGiveItem(GiveItem handler) {
        giveItem = handler;
        ensureGiveItemHook();
    }

    public static synchronized void installGiveItemRegistry(GiveItemRegistry handler) {
        giveItemRegistry = handler;
        ensureGiveItemRegistryHook();
    }

    public static synchronized void installEffects(ApplyEffect apply, ClearEffects clear) {
        applyEffect = apply;
        clearEffects = clear;
        ensureEffectHooks();
    }

    public static synchronized void installSpawnParticle(SpawnParticle handler) {
        spawnParticle = handler;
        ensureSpawnParticleHook();
    }

    public static synchronized void installWorldTime(WorldTimeGet get, WorldTimeSet set) {
        worldTimeGet = get;
        worldTimeSet = set;
        ensureWorldTimeHooks();
    }

    public static synchronized void installPlayerVelocity(
            PlayerVelocityGet get, PlayerVelocitySet set) {
        playerVelocityGet = get;
        playerVelocitySet = set;
        ensurePlayerVelocityHooks();
    }

    public static synchronized void installPlayerFlags(PlayerFlagsGet get) {
        playerFlagsGet = get;
        ensurePlayerFlagsHook();
    }

    public static synchronized void installRunCommand(RunCommand handler) {
        runCommand = handler;
        ensureRunCommandHook();
    }

    public static synchronized void installClearInventory(ClearInventory handler) {
        clearInventory = handler;
        ensureClearInventoryHook();
    }

    public static synchronized void installPlayerFlight(PlayerFlight handler) {
        playerFlight = handler;
        ensurePlayerFlightHook();
    }

    public static synchronized void installGetBiome(GetBiome handler) {
        getBiome = handler;
        ensureGetBiomeHook();
    }

    public static synchronized void installDifficulty(DifficultyGet get, DifficultySet set) {
        difficultyGet = get;
        difficultySet = set;
        ensureDifficultyHooks();
    }

    public static synchronized void installWeather(WeatherGet get, WeatherSet set) {
        weatherGet = get;
        weatherSet = set;
        ensureWeatherHooks();
    }

    public static synchronized void installGetLightLevel(GetLightLevel handler) {
        getLightLevel = handler;
        ensureGetLightLevelHook();
    }

    public static synchronized void installPlayerLatency(PlayerLatency handler) {
        playerLatency = handler;
        ensurePlayerLatencyHook();
    }

    public static synchronized void installWorldSpawn(WorldSpawnGet get, WorldSpawnSet set) {
        worldSpawnGet = get;
        worldSpawnSet = set;
        ensureWorldSpawnHooks();
    }

    public static synchronized void installPlayerOp(PlayerOp handler) {
        playerOp = handler;
        ensurePlayerOpHook();
    }

    public static synchronized void installPlayerUuid(PlayerUuid handler) {
        playerUuid = handler;
        ensurePlayerUuidHook();
    }

    public static synchronized void installPlayerPermission(PlayerPermission handler) {
        playerPermission = handler;
        ensurePlayerPermissionHook();
    }

    public static synchronized void installFindPlayerUuid(FindPlayerUuid handler) {
        findPlayerUuid = handler;
        ensureFindPlayerUuidHook();
    }

    public static synchronized void installFindPlayerName(FindPlayerName handler) {
        findPlayerName = handler;
        ensureFindPlayerNameHook();
    }

    public static synchronized void installBlockRegistry(
            GetBlockRegistry get, SetBlockRegistry set) {
        getBlockRegistry = get;
        setBlockRegistry = set;
        ensureBlockRegistryHooks();
    }

    public static void putPlayerSender(long handle, BiConsumer<Long, String> sender) {
        if (sender == null) {
            SEND_HANDLERS.remove(handle);
        } else {
            SEND_HANDLERS.put(handle, sender);
        }
        ensureSendHook();
    }

    public static void removePlayer(long handle) {
        SEND_HANDLERS.remove(handle);
    }

    private static synchronized void ensureSendHook() {
        if (sendInstalled) {
            return;
        }
        try {
            MethodHandle target = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeSendPlayerMessage",
                    MethodType.methodType(void.class, long.class, MemorySegment.class));
            FunctionDescriptor desc = FunctionDescriptor.ofVoid(
                    ValueLayout.JAVA_LONG, ValueLayout.ADDRESS);
            MemorySegment stub =
                    Linker.nativeLinker().upcallStub(target, desc, Arena.global());
            LeafBridge.setSendPlayerMessageHook(stub);
            sendInstalled = true;
        } catch (Throwable t) {
            throw new IllegalStateException("failed to install send hook", t);
        }
    }

    private static synchronized void ensureBroadcastHook() {
        if (broadcastInstalled) {
            return;
        }
        try {
            MethodHandle target = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeBroadcastMessage",
                    MethodType.methodType(void.class, MemorySegment.class));
            FunctionDescriptor desc = FunctionDescriptor.ofVoid(ValueLayout.ADDRESS);
            MemorySegment stub =
                    Linker.nativeLinker().upcallStub(target, desc, Arena.global());
            LeafBridge.setBroadcastMessageHook(stub);
            broadcastInstalled = true;
        } catch (Throwable t) {
            throw new IllegalStateException("failed to install broadcast hook", t);
        }
    }

    private static synchronized void ensureInventoryHooks() {
        if (inventoryInstalled) {
            return;
        }
        try {
            MethodHandle getTarget = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeGetInventorySlot",
                    MethodType.methodType(
                            int.class,
                            long.class,
                            int.class,
                            MemorySegment.class,
                            MemorySegment.class));
            MethodHandle setTarget = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeSetInventorySlot",
                    MethodType.methodType(
                            int.class,
                            long.class,
                            int.class,
                            int.class,
                            int.class));
            FunctionDescriptor getDesc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_LONG,
                    ValueLayout.JAVA_INT,
                    ValueLayout.ADDRESS,
                    ValueLayout.ADDRESS);
            FunctionDescriptor setDesc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_LONG,
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_INT);
            Linker linker = Linker.nativeLinker();
            MemorySegment getStub = linker.upcallStub(getTarget, getDesc, Arena.global());
            MemorySegment setStub = linker.upcallStub(setTarget, setDesc, Arena.global());
            LeafBridge.setInventoryHooks(getStub, setStub);
            inventoryInstalled = true;
        } catch (Throwable t) {
            throw new IllegalStateException("failed to install inventory hooks", t);
        }
    }

    private static synchronized void ensureInventoryStackHooks() {
        if (inventoryStackInstalled) {
            return;
        }
        try {
            MethodHandle getTarget = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeGetInventoryStack",
                    MethodType.methodType(
                            int.class,
                            long.class,
                            int.class,
                            MemorySegment.class,
                            MemorySegment.class,
                            MemorySegment.class));
            MethodHandle setTarget = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeSetInventoryStack",
                    MethodType.methodType(
                            int.class,
                            long.class,
                            int.class,
                            int.class,
                            int.class,
                            int.class));
            FunctionDescriptor getDesc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_LONG,
                    ValueLayout.JAVA_INT,
                    ValueLayout.ADDRESS,
                    ValueLayout.ADDRESS,
                    ValueLayout.ADDRESS);
            FunctionDescriptor setDesc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_LONG,
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_INT);
            Linker linker = Linker.nativeLinker();
            MemorySegment getStub = linker.upcallStub(getTarget, getDesc, Arena.global());
            MemorySegment setStub = linker.upcallStub(setTarget, setDesc, Arena.global());
            LeafBridge.setInventoryStackHooks(getStub, setStub);
            inventoryStackInstalled = true;
        } catch (Throwable t) {
            throw new IllegalStateException("failed to install inventory stack hooks", t);
        }
    }

    private static synchronized void ensureInventoryRegistryHooks() {
        if (inventoryRegistryInstalled) {
            return;
        }
        try {
            MethodHandle getTarget = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeGetInventoryRegistry",
                    MethodType.methodType(
                            int.class,
                            long.class,
                            int.class,
                            MemorySegment.class,
                            int.class,
                            MemorySegment.class,
                            MemorySegment.class));
            MethodHandle setTarget = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeSetInventoryRegistry",
                    MethodType.methodType(
                            int.class,
                            long.class,
                            int.class,
                            MemorySegment.class,
                            int.class,
                            int.class));
            FunctionDescriptor getDesc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_LONG,
                    ValueLayout.JAVA_INT,
                    ValueLayout.ADDRESS,
                    ValueLayout.JAVA_INT,
                    ValueLayout.ADDRESS,
                    ValueLayout.ADDRESS);
            FunctionDescriptor setDesc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_LONG,
                    ValueLayout.JAVA_INT,
                    ValueLayout.ADDRESS,
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_INT);
            Linker linker = Linker.nativeLinker();
            MemorySegment getStub = linker.upcallStub(getTarget, getDesc, Arena.global());
            MemorySegment setStub = linker.upcallStub(setTarget, setDesc, Arena.global());
            LeafBridge.setInventoryRegistryHooks(getStub, setStub);
            inventoryRegistryInstalled = true;
        } catch (Throwable t) {
            throw new IllegalStateException(
                    "failed to install inventory registry hooks", t);
        }
    }

    private static synchronized void ensureTeleportHook() {
        if (teleportInstalled) {
            return;
        }
        try {
            MethodHandle target = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeTeleportPlayer",
                    MethodType.methodType(
                            int.class,
                            long.class,
                            int.class,
                            int.class,
                            int.class,
                            int.class,
                            float.class,
                            float.class));
            FunctionDescriptor desc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_LONG,
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_FLOAT,
                    ValueLayout.JAVA_FLOAT);
            Linker linker = Linker.nativeLinker();
            MemorySegment stub = linker.upcallStub(target, desc, Arena.global());
            LeafBridge.setTeleportHook(stub);
            teleportInstalled = true;
        } catch (Throwable t) {
            throw new IllegalStateException("failed to install teleport hook", t);
        }
    }

    private static synchronized void ensureSelectedSlotHooks() {
        if (selectedSlotInstalled) {
            return;
        }
        try {
            MethodHandle getTarget = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeGetSelectedSlot",
                    MethodType.methodType(int.class, long.class, MemorySegment.class));
            MethodHandle setTarget = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeSetSelectedSlot",
                    MethodType.methodType(int.class, long.class, int.class));
            FunctionDescriptor getDesc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT, ValueLayout.JAVA_LONG, ValueLayout.ADDRESS);
            FunctionDescriptor setDesc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT, ValueLayout.JAVA_LONG, ValueLayout.JAVA_INT);
            Linker linker = Linker.nativeLinker();
            MemorySegment getStub = linker.upcallStub(getTarget, getDesc, Arena.global());
            MemorySegment setStub = linker.upcallStub(setTarget, setDesc, Arena.global());
            LeafBridge.setSelectedSlotHooks(getStub, setStub);
            selectedSlotInstalled = true;
        } catch (Throwable t) {
            throw new IllegalStateException(
                    "failed to install selected_slot hooks", t);
        }
    }

    private static synchronized void ensureGetWorldSeedHook() {
        if (worldSeedInstalled) {
            return;
        }
        try {
            MethodHandle target = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeGetWorldSeed",
                    MethodType.methodType(
                            int.class, int.class, MemorySegment.class));
            FunctionDescriptor desc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT, ValueLayout.JAVA_INT, ValueLayout.ADDRESS);
            Linker linker = Linker.nativeLinker();
            MemorySegment stub = linker.upcallStub(target, desc, Arena.global());
            LeafBridge.setGetWorldSeedHook(stub);
            worldSeedInstalled = true;
        } catch (Throwable t) {
            throw new IllegalStateException(
                    "failed to install get_world_seed hook", t);
        }
    }

    private static synchronized void ensurePlayerAbsorptionHooks() {
        if (playerAbsorptionInstalled) {
            return;
        }
        try {
            MethodHandle getTarget = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeGetPlayerAbsorption",
                    MethodType.methodType(
                            int.class, long.class, MemorySegment.class));
            MethodHandle setTarget = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeSetPlayerAbsorption",
                    MethodType.methodType(int.class, long.class, float.class));
            FunctionDescriptor getDesc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT, ValueLayout.JAVA_LONG, ValueLayout.ADDRESS);
            FunctionDescriptor setDesc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT, ValueLayout.JAVA_LONG, ValueLayout.JAVA_FLOAT);
            Linker linker = Linker.nativeLinker();
            MemorySegment getStub = linker.upcallStub(getTarget, getDesc, Arena.global());
            MemorySegment setStub = linker.upcallStub(setTarget, setDesc, Arena.global());
            LeafBridge.setPlayerAbsorptionHooks(getStub, setStub);
            playerAbsorptionInstalled = true;
        } catch (Throwable t) {
            throw new IllegalStateException(
                    "failed to install player_absorption hooks", t);
        }
    }

    private static synchronized void ensurePlayerInvulnerableHooks() {
        if (playerInvulnerableInstalled) {
            return;
        }
        try {
            MethodHandle getTarget = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeGetPlayerInvulnerable",
                    MethodType.methodType(
                            int.class, long.class, MemorySegment.class));
            MethodHandle setTarget = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeSetPlayerInvulnerable",
                    MethodType.methodType(int.class, long.class, int.class));
            FunctionDescriptor getDesc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT, ValueLayout.JAVA_LONG, ValueLayout.ADDRESS);
            FunctionDescriptor setDesc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT, ValueLayout.JAVA_LONG, ValueLayout.JAVA_INT);
            Linker linker = Linker.nativeLinker();
            MemorySegment getStub = linker.upcallStub(getTarget, getDesc, Arena.global());
            MemorySegment setStub = linker.upcallStub(setTarget, setDesc, Arena.global());
            LeafBridge.setPlayerInvulnerableHooks(getStub, setStub);
            playerInvulnerableInstalled = true;
        } catch (Throwable t) {
            throw new IllegalStateException(
                    "failed to install player_invulnerable hooks", t);
        }
    }

    private static synchronized void ensurePlayerAirHooks() {
        if (playerAirInstalled) {
            return;
        }
        try {
            MethodHandle getTarget = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeGetPlayerAir",
                    MethodType.methodType(
                            int.class, long.class, MemorySegment.class));
            MethodHandle setTarget = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeSetPlayerAir",
                    MethodType.methodType(int.class, long.class, int.class));
            FunctionDescriptor getDesc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT, ValueLayout.JAVA_LONG, ValueLayout.ADDRESS);
            FunctionDescriptor setDesc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT, ValueLayout.JAVA_LONG, ValueLayout.JAVA_INT);
            Linker linker = Linker.nativeLinker();
            MemorySegment getStub = linker.upcallStub(getTarget, getDesc, Arena.global());
            MemorySegment setStub = linker.upcallStub(setTarget, setDesc, Arena.global());
            LeafBridge.setPlayerAirHooks(getStub, setStub);
            playerAirInstalled = true;
        } catch (Throwable t) {
            throw new IllegalStateException(
                    "failed to install player_air hooks", t);
        }
    }

    private static synchronized void ensurePlayerFireTicksHooks() {
        if (playerFireTicksInstalled) {
            return;
        }
        try {
            MethodHandle getTarget = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeGetPlayerFireTicks",
                    MethodType.methodType(
                            int.class, long.class, MemorySegment.class));
            MethodHandle setTarget = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeSetPlayerFireTicks",
                    MethodType.methodType(int.class, long.class, int.class));
            FunctionDescriptor getDesc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT, ValueLayout.JAVA_LONG, ValueLayout.ADDRESS);
            FunctionDescriptor setDesc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT, ValueLayout.JAVA_LONG, ValueLayout.JAVA_INT);
            Linker linker = Linker.nativeLinker();
            MemorySegment getStub = linker.upcallStub(getTarget, getDesc, Arena.global());
            MemorySegment setStub = linker.upcallStub(setTarget, setDesc, Arena.global());
            LeafBridge.setPlayerFireTicksHooks(getStub, setStub);
            playerFireTicksInstalled = true;
        } catch (Throwable t) {
            throw new IllegalStateException(
                    "failed to install player_fire_ticks hooks", t);
        }
    }

    private static synchronized void ensurePlayerFrozenTicksHooks() {
        if (playerFrozenTicksInstalled) {
            return;
        }
        try {
            MethodHandle getTarget = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeGetPlayerFrozenTicks",
                    MethodType.methodType(
                            int.class, long.class, MemorySegment.class));
            MethodHandle setTarget = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeSetPlayerFrozenTicks",
                    MethodType.methodType(int.class, long.class, int.class));
            FunctionDescriptor getDesc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT, ValueLayout.JAVA_LONG, ValueLayout.ADDRESS);
            FunctionDescriptor setDesc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT, ValueLayout.JAVA_LONG, ValueLayout.JAVA_INT);
            Linker linker = Linker.nativeLinker();
            MemorySegment getStub = linker.upcallStub(getTarget, getDesc, Arena.global());
            MemorySegment setStub = linker.upcallStub(setTarget, setDesc, Arena.global());
            LeafBridge.setPlayerFrozenTicksHooks(getStub, setStub);
            playerFrozenTicksInstalled = true;
        } catch (Throwable t) {
            throw new IllegalStateException(
                    "failed to install player_frozen_ticks hooks", t);
        }
    }

    private static synchronized void ensurePlayerNoGravityHooks() {
        if (playerNoGravityInstalled) {
            return;
        }
        try {
            MethodHandle getTarget = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeGetPlayerNoGravity",
                    MethodType.methodType(
                            int.class, long.class, MemorySegment.class));
            MethodHandle setTarget = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeSetPlayerNoGravity",
                    MethodType.methodType(int.class, long.class, int.class));
            FunctionDescriptor getDesc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT, ValueLayout.JAVA_LONG, ValueLayout.ADDRESS);
            FunctionDescriptor setDesc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT, ValueLayout.JAVA_LONG, ValueLayout.JAVA_INT);
            Linker linker = Linker.nativeLinker();
            MemorySegment getStub = linker.upcallStub(getTarget, getDesc, Arena.global());
            MemorySegment setStub = linker.upcallStub(setTarget, setDesc, Arena.global());
            LeafBridge.setPlayerNoGravityHooks(getStub, setStub);
            playerNoGravityInstalled = true;
        } catch (Throwable t) {
            throw new IllegalStateException(
                    "failed to install player_no_gravity hooks", t);
        }
    }

    private static synchronized void ensurePlayerSilentHooks() {
        if (playerSilentInstalled) {
            return;
        }
        try {
            MethodHandle getTarget = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeGetPlayerSilent",
                    MethodType.methodType(
                            int.class, long.class, MemorySegment.class));
            MethodHandle setTarget = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeSetPlayerSilent",
                    MethodType.methodType(int.class, long.class, int.class));
            FunctionDescriptor getDesc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT, ValueLayout.JAVA_LONG, ValueLayout.ADDRESS);
            FunctionDescriptor setDesc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT, ValueLayout.JAVA_LONG, ValueLayout.JAVA_INT);
            Linker linker = Linker.nativeLinker();
            MemorySegment getStub = linker.upcallStub(getTarget, getDesc, Arena.global());
            MemorySegment setStub = linker.upcallStub(setTarget, setDesc, Arena.global());
            LeafBridge.setPlayerSilentHooks(getStub, setStub);
            playerSilentInstalled = true;
        } catch (Throwable t) {
            throw new IllegalStateException(
                    "failed to install player_silent hooks", t);
        }
    }

    private static synchronized void ensurePlayerGlowingHooks() {
        if (playerGlowingInstalled) {
            return;
        }
        try {
            MethodHandle getTarget = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeGetPlayerGlowing",
                    MethodType.methodType(
                            int.class, long.class, MemorySegment.class));
            MethodHandle setTarget = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeSetPlayerGlowing",
                    MethodType.methodType(int.class, long.class, int.class));
            FunctionDescriptor getDesc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT, ValueLayout.JAVA_LONG, ValueLayout.ADDRESS);
            FunctionDescriptor setDesc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT, ValueLayout.JAVA_LONG, ValueLayout.JAVA_INT);
            Linker linker = Linker.nativeLinker();
            MemorySegment getStub = linker.upcallStub(getTarget, getDesc, Arena.global());
            MemorySegment setStub = linker.upcallStub(setTarget, setDesc, Arena.global());
            LeafBridge.setPlayerGlowingHooks(getStub, setStub);
            playerGlowingInstalled = true;
        } catch (Throwable t) {
            throw new IllegalStateException(
                    "failed to install player_glowing hooks", t);
        }
    }

    private static synchronized void ensurePlayerInvisibleHooks() {
        if (playerInvisibleInstalled) {
            return;
        }
        try {
            MethodHandle getTarget = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeGetPlayerInvisible",
                    MethodType.methodType(
                            int.class, long.class, MemorySegment.class));
            MethodHandle setTarget = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeSetPlayerInvisible",
                    MethodType.methodType(int.class, long.class, int.class));
            FunctionDescriptor getDesc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT, ValueLayout.JAVA_LONG, ValueLayout.ADDRESS);
            FunctionDescriptor setDesc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT, ValueLayout.JAVA_LONG, ValueLayout.JAVA_INT);
            Linker linker = Linker.nativeLinker();
            MemorySegment getStub = linker.upcallStub(getTarget, getDesc, Arena.global());
            MemorySegment setStub = linker.upcallStub(setTarget, setDesc, Arena.global());
            LeafBridge.setPlayerInvisibleHooks(getStub, setStub);
            playerInvisibleInstalled = true;
        } catch (Throwable t) {
            throw new IllegalStateException(
                    "failed to install player_invisible hooks", t);
        }
    }

    private static synchronized void ensurePlayerPortalCooldownHooks() {
        if (playerPortalCooldownInstalled) {
            return;
        }
        try {
            MethodHandle getTarget = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeGetPlayerPortalCooldown",
                    MethodType.methodType(
                            int.class, long.class, MemorySegment.class));
            MethodHandle setTarget = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeSetPlayerPortalCooldown",
                    MethodType.methodType(int.class, long.class, int.class));
            FunctionDescriptor getDesc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT, ValueLayout.JAVA_LONG, ValueLayout.ADDRESS);
            FunctionDescriptor setDesc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT, ValueLayout.JAVA_LONG, ValueLayout.JAVA_INT);
            Linker linker = Linker.nativeLinker();
            MemorySegment getStub = linker.upcallStub(getTarget, getDesc, Arena.global());
            MemorySegment setStub = linker.upcallStub(setTarget, setDesc, Arena.global());
            LeafBridge.setPlayerPortalCooldownHooks(getStub, setStub);
            playerPortalCooldownInstalled = true;
        } catch (Throwable t) {
            throw new IllegalStateException(
                    "failed to install player_portal_cooldown hooks", t);
        }
    }

    private static synchronized void ensureGetPlayerMaxAirHook() {
        if (playerMaxAirInstalled) {
            return;
        }
        try {
            MethodHandle target = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeGetPlayerMaxAir",
                    MethodType.methodType(
                            int.class, long.class, MemorySegment.class));
            FunctionDescriptor desc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT, ValueLayout.JAVA_LONG, ValueLayout.ADDRESS);
            Linker linker = Linker.nativeLinker();
            MemorySegment stub = linker.upcallStub(target, desc, Arena.global());
            LeafBridge.setGetPlayerMaxAirHook(stub);
            playerMaxAirInstalled = true;
        } catch (Throwable t) {
            throw new IllegalStateException(
                    "failed to install get_player_max_air hook", t);
        }
    }

    private static synchronized void ensureWorldHooks() {
        if (worldInstalled) {
            return;
        }
        try {
            MethodHandle getTarget = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeGetBlock",
                    MethodType.methodType(
                            int.class,
                            int.class,
                            int.class,
                            int.class,
                            int.class,
                            MemorySegment.class));
            MethodHandle setTarget = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeSetBlock",
                    MethodType.methodType(
                            int.class,
                            int.class,
                            int.class,
                            int.class,
                            int.class,
                            int.class));
            FunctionDescriptor getDesc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_INT,
                    ValueLayout.ADDRESS);
            FunctionDescriptor setDesc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_INT);
            Linker linker = Linker.nativeLinker();
            MemorySegment getStub = linker.upcallStub(getTarget, getDesc, Arena.global());
            MemorySegment setStub = linker.upcallStub(setTarget, setDesc, Arena.global());
            LeafBridge.setWorldHooks(getStub, setStub);
            worldInstalled = true;
        } catch (Throwable t) {
            throw new IllegalStateException("failed to install world hooks", t);
        }
    }

    private static synchronized void ensurePlayerPosHooks() {
        if (playerPosInstalled) {
            return;
        }
        try {
            MethodHandle getTarget = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeGetPlayerPos",
                    MethodType.methodType(
                            int.class,
                            long.class,
                            MemorySegment.class,
                            MemorySegment.class,
                            MemorySegment.class,
                            MemorySegment.class));
            MethodHandle setTarget = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeSetPlayerPos",
                    MethodType.methodType(
                            int.class,
                            long.class,
                            int.class,
                            int.class,
                            int.class,
                            int.class));
            FunctionDescriptor getDesc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_LONG,
                    ValueLayout.ADDRESS,
                    ValueLayout.ADDRESS,
                    ValueLayout.ADDRESS,
                    ValueLayout.ADDRESS);
            FunctionDescriptor setDesc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_LONG,
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_INT);
            Linker linker = Linker.nativeLinker();
            MemorySegment getStub = linker.upcallStub(getTarget, getDesc, Arena.global());
            MemorySegment setStub = linker.upcallStub(setTarget, setDesc, Arena.global());
            LeafBridge.setPlayerPosHooks(getStub, setStub);
            playerPosInstalled = true;
        } catch (Throwable t) {
            throw new IllegalStateException("failed to install player pos hooks", t);
        }
    }

    private static synchronized void ensurePlayerHealthHooks() {
        if (playerHealthInstalled) {
            return;
        }
        try {
            MethodHandle getTarget = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeGetPlayerHealth",
                    MethodType.methodType(
                            int.class,
                            long.class,
                            MemorySegment.class,
                            MemorySegment.class));
            MethodHandle setTarget = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeSetPlayerHealth",
                    MethodType.methodType(int.class, long.class, float.class));
            FunctionDescriptor getDesc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_LONG,
                    ValueLayout.ADDRESS,
                    ValueLayout.ADDRESS);
            FunctionDescriptor setDesc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_LONG,
                    ValueLayout.JAVA_FLOAT);
            Linker linker = Linker.nativeLinker();
            MemorySegment getStub = linker.upcallStub(getTarget, getDesc, Arena.global());
            MemorySegment setStub = linker.upcallStub(setTarget, setDesc, Arena.global());
            LeafBridge.setPlayerHealthHooks(getStub, setStub);
            playerHealthInstalled = true;
        } catch (Throwable t) {
            throw new IllegalStateException("failed to install player health hooks", t);
        }
    }

    private static synchronized void ensurePlayerFoodHooks() {
        if (playerFoodInstalled) {
            return;
        }
        try {
            MethodHandle getTarget = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeGetPlayerFood",
                    MethodType.methodType(
                            int.class,
                            long.class,
                            MemorySegment.class,
                            MemorySegment.class));
            MethodHandle setTarget = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeSetPlayerFood",
                    MethodType.methodType(
                            int.class, long.class, int.class, float.class));
            FunctionDescriptor getDesc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_LONG,
                    ValueLayout.ADDRESS,
                    ValueLayout.ADDRESS);
            FunctionDescriptor setDesc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_LONG,
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_FLOAT);
            Linker linker = Linker.nativeLinker();
            MemorySegment getStub = linker.upcallStub(getTarget, getDesc, Arena.global());
            MemorySegment setStub = linker.upcallStub(setTarget, setDesc, Arena.global());
            LeafBridge.setPlayerFoodHooks(getStub, setStub);
            playerFoodInstalled = true;
        } catch (Throwable t) {
            throw new IllegalStateException("failed to install player food hooks", t);
        }
    }

    private static synchronized void ensurePlayerGamemodeHooks() {
        if (playerGamemodeInstalled) {
            return;
        }
        try {
            MethodHandle getTarget = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeGetPlayerGamemode",
                    MethodType.methodType(int.class, long.class, MemorySegment.class));
            MethodHandle setTarget = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeSetPlayerGamemode",
                    MethodType.methodType(int.class, long.class, int.class));
            FunctionDescriptor getDesc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_LONG,
                    ValueLayout.ADDRESS);
            FunctionDescriptor setDesc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_LONG,
                    ValueLayout.JAVA_INT);
            Linker linker = Linker.nativeLinker();
            MemorySegment getStub = linker.upcallStub(getTarget, getDesc, Arena.global());
            MemorySegment setStub = linker.upcallStub(setTarget, setDesc, Arena.global());
            LeafBridge.setPlayerGamemodeHooks(getStub, setStub);
            playerGamemodeInstalled = true;
        } catch (Throwable t) {
            throw new IllegalStateException("failed to install player gamemode hooks", t);
        }
    }

    private static synchronized void ensurePlayerXpHooks() {
        if (playerXpInstalled) {
            return;
        }
        try {
            MethodHandle getTarget = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeGetPlayerXp",
                    MethodType.methodType(
                            int.class,
                            long.class,
                            MemorySegment.class,
                            MemorySegment.class));
            MethodHandle setTarget = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeSetPlayerXpLevel",
                    MethodType.methodType(int.class, long.class, int.class));
            FunctionDescriptor getDesc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_LONG,
                    ValueLayout.ADDRESS,
                    ValueLayout.ADDRESS);
            FunctionDescriptor setDesc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_LONG,
                    ValueLayout.JAVA_INT);
            Linker linker = Linker.nativeLinker();
            MemorySegment getStub = linker.upcallStub(getTarget, getDesc, Arena.global());
            MemorySegment setStub = linker.upcallStub(setTarget, setDesc, Arena.global());
            LeafBridge.setPlayerXpHooks(getStub, setStub);
            playerXpInstalled = true;
        } catch (Throwable t) {
            throw new IllegalStateException("failed to install player xp hooks", t);
        }
    }

    private static synchronized void ensurePlayerLookHooks() {
        if (playerLookInstalled) {
            return;
        }
        try {
            MethodHandle getTarget = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeGetPlayerLook",
                    MethodType.methodType(
                            int.class,
                            long.class,
                            MemorySegment.class,
                            MemorySegment.class));
            MethodHandle setTarget = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeSetPlayerLook",
                    MethodType.methodType(
                            int.class, long.class, float.class, float.class));
            FunctionDescriptor getDesc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_LONG,
                    ValueLayout.ADDRESS,
                    ValueLayout.ADDRESS);
            FunctionDescriptor setDesc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_LONG,
                    ValueLayout.JAVA_FLOAT,
                    ValueLayout.JAVA_FLOAT);
            Linker linker = Linker.nativeLinker();
            MemorySegment getStub = linker.upcallStub(getTarget, getDesc, Arena.global());
            MemorySegment setStub = linker.upcallStub(setTarget, setDesc, Arena.global());
            LeafBridge.setPlayerLookHooks(getStub, setStub);
            playerLookInstalled = true;
        } catch (Throwable t) {
            throw new IllegalStateException("failed to install player look hooks", t);
        }
    }

    private static synchronized void ensurePlaySoundHook() {
        if (playSoundInstalled) {
            return;
        }
        try {
            MethodHandle target = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativePlaySound",
                    MethodType.methodType(
                            int.class,
                            long.class,
                            MemorySegment.class,
                            float.class,
                            float.class,
                            int.class,
                            int.class,
                            int.class,
                            int.class));
            FunctionDescriptor desc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_LONG,
                    ValueLayout.ADDRESS,
                    ValueLayout.JAVA_FLOAT,
                    ValueLayout.JAVA_FLOAT,
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_INT);
            Linker linker = Linker.nativeLinker();
            MemorySegment stub = linker.upcallStub(target, desc, Arena.global());
            LeafBridge.setPlaySoundHook(stub);
            playSoundInstalled = true;
        } catch (Throwable t) {
            throw new IllegalStateException("failed to install play_sound hook", t);
        }
    }

    private static synchronized void ensureActionbarHook() {
        if (actionbarInstalled) {
            return;
        }
        try {
            MethodHandle target = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeSendActionbar",
                    MethodType.methodType(int.class, long.class, MemorySegment.class));
            FunctionDescriptor desc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT, ValueLayout.JAVA_LONG, ValueLayout.ADDRESS);
            Linker linker = Linker.nativeLinker();
            MemorySegment stub = linker.upcallStub(target, desc, Arena.global());
            LeafBridge.setActionbarHook(stub);
            actionbarInstalled = true;
        } catch (Throwable t) {
            throw new IllegalStateException("failed to install actionbar hook", t);
        }
    }

    private static synchronized void ensureTitleHook() {
        if (titleInstalled) {
            return;
        }
        try {
            MethodHandle target = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeSendTitle",
                    MethodType.methodType(
                            int.class,
                            long.class,
                            MemorySegment.class,
                            MemorySegment.class,
                            int.class,
                            int.class,
                            int.class));
            FunctionDescriptor desc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_LONG,
                    ValueLayout.ADDRESS,
                    ValueLayout.ADDRESS,
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_INT);
            Linker linker = Linker.nativeLinker();
            MemorySegment stub = linker.upcallStub(target, desc, Arena.global());
            LeafBridge.setTitleHook(stub);
            titleInstalled = true;
        } catch (Throwable t) {
            throw new IllegalStateException("failed to install title hook", t);
        }
    }

    private static synchronized void ensureKickHook() {
        if (kickInstalled) {
            return;
        }
        try {
            MethodHandle target = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeKickPlayer",
                    MethodType.methodType(int.class, long.class, MemorySegment.class));
            FunctionDescriptor desc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT, ValueLayout.JAVA_LONG, ValueLayout.ADDRESS);
            Linker linker = Linker.nativeLinker();
            MemorySegment stub = linker.upcallStub(target, desc, Arena.global());
            LeafBridge.setKickPlayerHook(stub);
            kickInstalled = true;
        } catch (Throwable t) {
            throw new IllegalStateException("failed to install kick hook", t);
        }
    }

    private static synchronized void ensureGiveItemHook() {
        if (giveItemInstalled) {
            return;
        }
        try {
            MethodHandle target = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeGiveItem",
                    MethodType.methodType(
                            int.class, long.class, int.class, int.class, int.class));
            FunctionDescriptor desc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_LONG,
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_INT);
            Linker linker = Linker.nativeLinker();
            MemorySegment stub = linker.upcallStub(target, desc, Arena.global());
            LeafBridge.setGiveItemHook(stub);
            giveItemInstalled = true;
        } catch (Throwable t) {
            throw new IllegalStateException("failed to install give_item hook", t);
        }
    }

    private static synchronized void ensureGiveItemRegistryHook() {
        if (giveItemRegistryInstalled) {
            return;
        }
        try {
            MethodHandle target = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeGiveItemRegistry",
                    MethodType.methodType(
                            int.class,
                            long.class,
                            MemorySegment.class,
                            int.class,
                            int.class));
            FunctionDescriptor desc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_LONG,
                    ValueLayout.ADDRESS,
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_INT);
            Linker linker = Linker.nativeLinker();
            MemorySegment stub = linker.upcallStub(target, desc, Arena.global());
            LeafBridge.setGiveItemRegistryHook(stub);
            giveItemRegistryInstalled = true;
        } catch (Throwable t) {
            throw new IllegalStateException(
                    "failed to install give_item_registry hook", t);
        }
    }

    private static synchronized void ensureEffectHooks() {
        if (effectInstalled) {
            return;
        }
        try {
            MethodHandle applyTarget = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeApplyEffect",
                    MethodType.methodType(
                            int.class,
                            long.class,
                            MemorySegment.class,
                            int.class,
                            int.class,
                            int.class));
            MethodHandle clearTarget = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeClearEffects",
                    MethodType.methodType(int.class, long.class));
            FunctionDescriptor applyDesc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_LONG,
                    ValueLayout.ADDRESS,
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_INT);
            FunctionDescriptor clearDesc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT, ValueLayout.JAVA_LONG);
            Linker linker = Linker.nativeLinker();
            MemorySegment applyStub =
                    linker.upcallStub(applyTarget, applyDesc, Arena.global());
            MemorySegment clearStub =
                    linker.upcallStub(clearTarget, clearDesc, Arena.global());
            LeafBridge.setEffectHooks(applyStub, clearStub);
            effectInstalled = true;
        } catch (Throwable t) {
            throw new IllegalStateException("failed to install effect hooks", t);
        }
    }

    private static synchronized void ensureSpawnParticleHook() {
        if (spawnParticleInstalled) {
            return;
        }
        try {
            MethodHandle target = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeSpawnParticle",
                    MethodType.methodType(
                            int.class,
                            MemorySegment.class,
                            double.class,
                            double.class,
                            double.class,
                            int.class,
                            int.class,
                            double.class,
                            double.class,
                            double.class,
                            double.class));
            FunctionDescriptor desc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT,
                    ValueLayout.ADDRESS,
                    ValueLayout.JAVA_DOUBLE,
                    ValueLayout.JAVA_DOUBLE,
                    ValueLayout.JAVA_DOUBLE,
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_DOUBLE,
                    ValueLayout.JAVA_DOUBLE,
                    ValueLayout.JAVA_DOUBLE,
                    ValueLayout.JAVA_DOUBLE);
            Linker linker = Linker.nativeLinker();
            MemorySegment stub = linker.upcallStub(target, desc, Arena.global());
            LeafBridge.setSpawnParticleHook(stub);
            spawnParticleInstalled = true;
        } catch (Throwable t) {
            throw new IllegalStateException("failed to install spawn_particle hook", t);
        }
    }

    private static synchronized void ensureWorldTimeHooks() {
        if (worldTimeInstalled) {
            return;
        }
        try {
            MethodHandle getTarget = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeGetWorldTime",
                    MethodType.methodType(int.class, int.class, MemorySegment.class));
            MethodHandle setTarget = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeSetWorldTime",
                    MethodType.methodType(int.class, int.class, long.class));
            FunctionDescriptor getDesc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT, ValueLayout.JAVA_INT, ValueLayout.ADDRESS);
            FunctionDescriptor setDesc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT, ValueLayout.JAVA_INT, ValueLayout.JAVA_LONG);
            Linker linker = Linker.nativeLinker();
            MemorySegment getStub = linker.upcallStub(getTarget, getDesc, Arena.global());
            MemorySegment setStub = linker.upcallStub(setTarget, setDesc, Arena.global());
            LeafBridge.setWorldTimeHooks(getStub, setStub);
            worldTimeInstalled = true;
        } catch (Throwable t) {
            throw new IllegalStateException("failed to install world time hooks", t);
        }
    }

    private static synchronized void ensurePlayerVelocityHooks() {
        if (playerVelocityInstalled) {
            return;
        }
        try {
            MethodHandle getTarget = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeGetPlayerVelocity",
                    MethodType.methodType(
                            int.class,
                            long.class,
                            MemorySegment.class,
                            MemorySegment.class,
                            MemorySegment.class));
            MethodHandle setTarget = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeSetPlayerVelocity",
                    MethodType.methodType(
                            int.class,
                            long.class,
                            double.class,
                            double.class,
                            double.class));
            FunctionDescriptor getDesc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_LONG,
                    ValueLayout.ADDRESS,
                    ValueLayout.ADDRESS,
                    ValueLayout.ADDRESS);
            FunctionDescriptor setDesc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_LONG,
                    ValueLayout.JAVA_DOUBLE,
                    ValueLayout.JAVA_DOUBLE,
                    ValueLayout.JAVA_DOUBLE);
            Linker linker = Linker.nativeLinker();
            MemorySegment getStub = linker.upcallStub(getTarget, getDesc, Arena.global());
            MemorySegment setStub = linker.upcallStub(setTarget, setDesc, Arena.global());
            LeafBridge.setPlayerVelocityHooks(getStub, setStub);
            playerVelocityInstalled = true;
        } catch (Throwable t) {
            throw new IllegalStateException("failed to install player velocity hooks", t);
        }
    }

    private static synchronized void ensurePlayerFlagsHook() {
        if (playerFlagsInstalled) {
            return;
        }
        try {
            MethodHandle target = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeGetPlayerFlags",
                    MethodType.methodType(int.class, long.class, MemorySegment.class));
            FunctionDescriptor desc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT, ValueLayout.JAVA_LONG, ValueLayout.ADDRESS);
            Linker linker = Linker.nativeLinker();
            MemorySegment stub = linker.upcallStub(target, desc, Arena.global());
            LeafBridge.setPlayerFlagsHook(stub);
            playerFlagsInstalled = true;
        } catch (Throwable t) {
            throw new IllegalStateException("failed to install player flags hook", t);
        }
    }

    private static synchronized void ensureRunCommandHook() {
        if (runCommandInstalled) {
            return;
        }
        try {
            MethodHandle target = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeRunCommand",
                    MethodType.methodType(int.class, long.class, MemorySegment.class));
            FunctionDescriptor desc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT, ValueLayout.JAVA_LONG, ValueLayout.ADDRESS);
            Linker linker = Linker.nativeLinker();
            MemorySegment stub = linker.upcallStub(target, desc, Arena.global());
            LeafBridge.setRunCommandHook(stub);
            runCommandInstalled = true;
        } catch (Throwable t) {
            throw new IllegalStateException("failed to install run_command hook", t);
        }
    }

    private static synchronized void ensureClearInventoryHook() {
        if (clearInventoryInstalled) {
            return;
        }
        try {
            MethodHandle target = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeClearInventory",
                    MethodType.methodType(int.class, long.class));
            FunctionDescriptor desc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT, ValueLayout.JAVA_LONG);
            Linker linker = Linker.nativeLinker();
            MemorySegment stub = linker.upcallStub(target, desc, Arena.global());
            LeafBridge.setClearInventoryHook(stub);
            clearInventoryInstalled = true;
        } catch (Throwable t) {
            throw new IllegalStateException("failed to install clear_inventory hook", t);
        }
    }

    private static synchronized void ensurePlayerFlightHook() {
        if (playerFlightInstalled) {
            return;
        }
        try {
            MethodHandle target = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeSetPlayerFlight",
                    MethodType.methodType(int.class, long.class, int.class, int.class));
            FunctionDescriptor desc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_LONG,
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_INT);
            Linker linker = Linker.nativeLinker();
            MemorySegment stub = linker.upcallStub(target, desc, Arena.global());
            LeafBridge.setPlayerFlightHook(stub);
            playerFlightInstalled = true;
        } catch (Throwable t) {
            throw new IllegalStateException("failed to install player_flight hook", t);
        }
    }

    private static synchronized void ensureGetBiomeHook() {
        if (getBiomeInstalled) {
            return;
        }
        try {
            MethodHandle target = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeGetBiome",
                    MethodType.methodType(
                            int.class,
                            int.class,
                            int.class,
                            int.class,
                            int.class,
                            MemorySegment.class,
                            int.class));
            FunctionDescriptor desc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_INT,
                    ValueLayout.ADDRESS,
                    ValueLayout.JAVA_INT);
            Linker linker = Linker.nativeLinker();
            MemorySegment stub = linker.upcallStub(target, desc, Arena.global());
            LeafBridge.setGetBiomeHook(stub);
            getBiomeInstalled = true;
        } catch (Throwable t) {
            throw new IllegalStateException("failed to install get_biome hook", t);
        }
    }

    private static synchronized void ensureDifficultyHooks() {
        if (difficultyInstalled) {
            return;
        }
        try {
            MethodHandle getTarget = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeGetDifficulty",
                    MethodType.methodType(int.class, MemorySegment.class));
            MethodHandle setTarget = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeSetDifficulty",
                    MethodType.methodType(int.class, int.class));
            FunctionDescriptor getDesc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT, ValueLayout.ADDRESS);
            FunctionDescriptor setDesc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT, ValueLayout.JAVA_INT);
            Linker linker = Linker.nativeLinker();
            MemorySegment getStub = linker.upcallStub(getTarget, getDesc, Arena.global());
            MemorySegment setStub = linker.upcallStub(setTarget, setDesc, Arena.global());
            LeafBridge.setDifficultyHooks(getStub, setStub);
            difficultyInstalled = true;
        } catch (Throwable t) {
            throw new IllegalStateException("failed to install difficulty hooks", t);
        }
    }

    private static synchronized void ensureWeatherHooks() {
        if (weatherInstalled) {
            return;
        }
        try {
            MethodHandle getTarget = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeGetWeather",
                    MethodType.methodType(int.class, int.class, MemorySegment.class));
            MethodHandle setTarget = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeSetWeather",
                    MethodType.methodType(int.class, int.class, int.class, int.class));
            FunctionDescriptor getDesc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT, ValueLayout.JAVA_INT, ValueLayout.ADDRESS);
            FunctionDescriptor setDesc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_INT);
            Linker linker = Linker.nativeLinker();
            MemorySegment getStub = linker.upcallStub(getTarget, getDesc, Arena.global());
            MemorySegment setStub = linker.upcallStub(setTarget, setDesc, Arena.global());
            LeafBridge.setWeatherHooks(getStub, setStub);
            weatherInstalled = true;
        } catch (Throwable t) {
            throw new IllegalStateException("failed to install weather hooks", t);
        }
    }

    private static synchronized void ensureGetLightLevelHook() {
        if (getLightLevelInstalled) {
            return;
        }
        try {
            MethodHandle target = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeGetLightLevel",
                    MethodType.methodType(
                            int.class,
                            int.class,
                            int.class,
                            int.class,
                            int.class,
                            MemorySegment.class,
                            MemorySegment.class));
            FunctionDescriptor desc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_INT,
                    ValueLayout.ADDRESS,
                    ValueLayout.ADDRESS);
            Linker linker = Linker.nativeLinker();
            MemorySegment stub = linker.upcallStub(target, desc, Arena.global());
            LeafBridge.setGetLightLevelHook(stub);
            getLightLevelInstalled = true;
        } catch (Throwable t) {
            throw new IllegalStateException("failed to install get_light_level hook", t);
        }
    }

    private static synchronized void ensurePlayerLatencyHook() {
        if (playerLatencyInstalled) {
            return;
        }
        try {
            MethodHandle target = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeGetPlayerLatency",
                    MethodType.methodType(int.class, long.class, MemorySegment.class));
            FunctionDescriptor desc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT, ValueLayout.JAVA_LONG, ValueLayout.ADDRESS);
            Linker linker = Linker.nativeLinker();
            MemorySegment stub = linker.upcallStub(target, desc, Arena.global());
            LeafBridge.setPlayerLatencyHook(stub);
            playerLatencyInstalled = true;
        } catch (Throwable t) {
            throw new IllegalStateException("failed to install player_latency hook", t);
        }
    }

    private static synchronized void ensureWorldSpawnHooks() {
        if (worldSpawnInstalled) {
            return;
        }
        try {
            MethodHandle getTarget = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeGetWorldSpawn",
                    MethodType.methodType(
                            int.class,
                            int.class,
                            MemorySegment.class,
                            MemorySegment.class,
                            MemorySegment.class));
            MethodHandle setTarget = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeSetWorldSpawn",
                    MethodType.methodType(
                            int.class, int.class, int.class, int.class, int.class));
            FunctionDescriptor getDesc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_INT,
                    ValueLayout.ADDRESS,
                    ValueLayout.ADDRESS,
                    ValueLayout.ADDRESS);
            FunctionDescriptor setDesc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_INT);
            Linker linker = Linker.nativeLinker();
            MemorySegment getStub = linker.upcallStub(getTarget, getDesc, Arena.global());
            MemorySegment setStub = linker.upcallStub(setTarget, setDesc, Arena.global());
            LeafBridge.setWorldSpawnHooks(getStub, setStub);
            worldSpawnInstalled = true;
        } catch (Throwable t) {
            throw new IllegalStateException("failed to install world_spawn hooks", t);
        }
    }

    private static synchronized void ensurePlayerOpHook() {
        if (playerOpInstalled) {
            return;
        }
        try {
            MethodHandle target = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeIsPlayerOp",
                    MethodType.methodType(int.class, long.class, MemorySegment.class));
            FunctionDescriptor desc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT, ValueLayout.JAVA_LONG, ValueLayout.ADDRESS);
            Linker linker = Linker.nativeLinker();
            MemorySegment stub = linker.upcallStub(target, desc, Arena.global());
            LeafBridge.setPlayerOpHook(stub);
            playerOpInstalled = true;
        } catch (Throwable t) {
            throw new IllegalStateException("failed to install player_op hook", t);
        }
    }

    private static synchronized void ensurePlayerUuidHook() {
        if (playerUuidInstalled) {
            return;
        }
        try {
            MethodHandle target = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeGetPlayerUuid",
                    MethodType.methodType(
                            int.class, long.class, MemorySegment.class, int.class));
            FunctionDescriptor desc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_LONG,
                    ValueLayout.ADDRESS,
                    ValueLayout.JAVA_INT);
            Linker linker = Linker.nativeLinker();
            MemorySegment stub = linker.upcallStub(target, desc, Arena.global());
            LeafBridge.setPlayerUuidHook(stub);
            playerUuidInstalled = true;
        } catch (Throwable t) {
            throw new IllegalStateException("failed to install player_uuid hook", t);
        }
    }

    private static synchronized void ensurePlayerPermissionHook() {
        if (playerPermissionInstalled) {
            return;
        }
        try {
            MethodHandle target = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeGetPlayerPermission",
                    MethodType.methodType(int.class, long.class, MemorySegment.class));
            FunctionDescriptor desc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT, ValueLayout.JAVA_LONG, ValueLayout.ADDRESS);
            Linker linker = Linker.nativeLinker();
            MemorySegment stub = linker.upcallStub(target, desc, Arena.global());
            LeafBridge.setPlayerPermissionHook(stub);
            playerPermissionInstalled = true;
        } catch (Throwable t) {
            throw new IllegalStateException(
                    "failed to install player_permission hook", t);
        }
    }

    private static synchronized void ensureFindPlayerUuidHook() {
        if (findPlayerUuidInstalled) {
            return;
        }
        try {
            MethodHandle target = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeFindPlayerUuid",
                    MethodType.methodType(
                            int.class, MemorySegment.class, MemorySegment.class));
            FunctionDescriptor desc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT, ValueLayout.ADDRESS, ValueLayout.ADDRESS);
            Linker linker = Linker.nativeLinker();
            MemorySegment stub = linker.upcallStub(target, desc, Arena.global());
            LeafBridge.setFindPlayerUuidHook(stub);
            findPlayerUuidInstalled = true;
        } catch (Throwable t) {
            throw new IllegalStateException(
                    "failed to install find_player_uuid hook", t);
        }
    }

    private static synchronized void ensureFindPlayerNameHook() {
        if (findPlayerNameInstalled) {
            return;
        }
        try {
            MethodHandle target = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeFindPlayerName",
                    MethodType.methodType(
                            int.class, MemorySegment.class, MemorySegment.class));
            FunctionDescriptor desc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT, ValueLayout.ADDRESS, ValueLayout.ADDRESS);
            Linker linker = Linker.nativeLinker();
            MemorySegment stub = linker.upcallStub(target, desc, Arena.global());
            LeafBridge.setFindPlayerNameHook(stub);
            findPlayerNameInstalled = true;
        } catch (Throwable t) {
            throw new IllegalStateException(
                    "failed to install find_player_name hook", t);
        }
    }

    private static synchronized void ensureBlockRegistryHooks() {
        if (blockRegistryInstalled) {
            return;
        }
        try {
            MethodHandle getTarget = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeGetBlockRegistry",
                    MethodType.methodType(
                            int.class,
                            int.class,
                            int.class,
                            int.class,
                            int.class,
                            MemorySegment.class,
                            int.class));
            MethodHandle setTarget = MethodHandles.lookup().findStatic(
                    LeafMinecraftHooks.class,
                    "nativeSetBlockRegistry",
                    MethodType.methodType(
                            int.class,
                            int.class,
                            int.class,
                            int.class,
                            int.class,
                            MemorySegment.class));
            FunctionDescriptor getDesc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_INT,
                    ValueLayout.ADDRESS,
                    ValueLayout.JAVA_INT);
            FunctionDescriptor setDesc = FunctionDescriptor.of(
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_INT,
                    ValueLayout.JAVA_INT,
                    ValueLayout.ADDRESS);
            Linker linker = Linker.nativeLinker();
            MemorySegment getStub = linker.upcallStub(getTarget, getDesc, Arena.global());
            MemorySegment setStub = linker.upcallStub(setTarget, setDesc, Arena.global());
            LeafBridge.setBlockRegistryHooks(getStub, setStub);
            blockRegistryInstalled = true;
        } catch (Throwable t) {
            throw new IllegalStateException(
                    "failed to install block_registry hooks", t);
        }
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static void nativeSendPlayerMessage(long player, MemorySegment message) {
        String text = message == null || message.equals(MemorySegment.NULL)
                ? ""
                : message.getString(0, StandardCharsets.UTF_8);
        BiConsumer<Long, String> specific = SEND_HANDLERS.get(player);
        if (specific != null) {
            specific.accept(player, text);
            return;
        }
        BiConsumer<Long, String> fallback = defaultSend;
        if (fallback != null) {
            fallback.accept(player, text);
        }
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static void nativeBroadcastMessage(MemorySegment message) {
        String text = message == null || message.equals(MemorySegment.NULL)
                ? ""
                : message.getString(0, StandardCharsets.UTF_8);
        Consumer<String> handler = broadcast;
        if (handler != null) {
            handler.accept(text);
            return;
        }
        BiConsumer<Long, String> fallback = defaultSend;
        if (fallback != null) {
            for (Long handle : SEND_HANDLERS.keySet()) {
                fallback.accept(handle, text);
            }
        }
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeGetInventorySlot(
            long player, int slot, MemorySegment outItem, MemorySegment outCount) {
        InventoryGet handler = inventoryGet;
        if (handler == null || outItem == null || outCount == null
                || outItem.equals(MemorySegment.NULL)
                || outCount.equals(MemorySegment.NULL)) {
            return 1;
        }
        int[] item = new int[1];
        int[] count = new int[1];
        int code = handler.get(player, slot, item, count);
        if (code == 0) {
            outItem.set(ValueLayout.JAVA_INT, 0, item[0]);
            outCount.set(ValueLayout.JAVA_INT, 0, count[0]);
        }
        return code;
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeSetInventorySlot(long player, int slot, int itemId, int count) {
        InventorySet handler = inventorySet;
        if (handler == null) {
            return 1;
        }
        return handler.set(player, slot, itemId, count);
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeGetInventoryStack(
            long player,
            int slot,
            MemorySegment outItem,
            MemorySegment outCount,
            MemorySegment outDamage) {
        InventoryStackGet handler = inventoryStackGet;
        if (handler == null || outItem == null || outCount == null || outDamage == null
                || outItem.equals(MemorySegment.NULL)
                || outCount.equals(MemorySegment.NULL)
                || outDamage.equals(MemorySegment.NULL)) {
            return 1;
        }
        int[] item = new int[1];
        int[] count = new int[1];
        int[] damage = new int[1];
        int code = handler.get(player, slot, item, count, damage);
        if (code == 0) {
            outItem.set(ValueLayout.JAVA_INT, 0, item[0]);
            outCount.set(ValueLayout.JAVA_INT, 0, count[0]);
            outDamage.set(ValueLayout.JAVA_INT, 0, damage[0]);
        }
        return code;
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeSetInventoryStack(
            long player, int slot, int itemId, int count, int damage) {
        InventoryStackSet handler = inventoryStackSet;
        if (handler == null) {
            return 1;
        }
        return handler.set(player, slot, itemId, count, damage);
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeGetInventoryRegistry(
            long player,
            int slot,
            MemorySegment outBuf,
            int outSize,
            MemorySegment outCount,
            MemorySegment outDamage) {
        InventoryRegistryGet handler = inventoryRegistryGet;
        if (handler == null || outBuf == null || outBuf.equals(MemorySegment.NULL)
                || outSize <= 0 || outCount == null || outCount.equals(MemorySegment.NULL)
                || outDamage == null || outDamage.equals(MemorySegment.NULL)) {
            return 1;
        }
        String[] outId = new String[1];
        int[] count = new int[1];
        int[] damage = new int[1];
        int code = handler.get(player, slot, outId, count, damage);
        if (code != 0) {
            return code;
        }
        String id = outId[0] != null ? outId[0] : "minecraft:air";
        byte[] utf8 = id.getBytes(StandardCharsets.UTF_8);
        int n = Math.min(utf8.length, outSize - 1);
        for (int i = 0; i < n; i++) {
            outBuf.set(ValueLayout.JAVA_BYTE, i, utf8[i]);
        }
        outBuf.set(ValueLayout.JAVA_BYTE, n, (byte) 0);
        outCount.set(ValueLayout.JAVA_INT, 0, count[0]);
        outDamage.set(ValueLayout.JAVA_INT, 0, damage[0]);
        return 0;
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeSetInventoryRegistry(
            long player, int slot, MemorySegment itemIdSeg, int count, int damage) {
        InventoryRegistrySet handler = inventoryRegistrySet;
        if (handler == null || itemIdSeg == null || itemIdSeg.equals(MemorySegment.NULL)) {
            return 1;
        }
        String itemId = itemIdSeg.getString(0, StandardCharsets.UTF_8);
        return handler.set(player, slot, itemId, count, damage);
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeTeleportPlayer(
            long player, int x, int y, int z, int dimension, float yaw, float pitch) {
        TeleportPlayer handler = teleportPlayer;
        if (handler == null) {
            return 1;
        }
        return handler.teleport(player, x, y, z, dimension, yaw, pitch);
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeGetSelectedSlot(long player, MemorySegment outSlot) {
        SelectedSlotGet handler = selectedSlotGet;
        if (handler == null || outSlot == null || outSlot.equals(MemorySegment.NULL)) {
            return 1;
        }
        int[] value = new int[1];
        int code = handler.get(player, value);
        if (code == 0) {
            outSlot.set(ValueLayout.JAVA_INT, 0, value[0]);
        }
        return code;
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeSetSelectedSlot(long player, int slot) {
        SelectedSlotSet handler = selectedSlotSet;
        if (handler == null) {
            return 1;
        }
        return handler.set(player, slot);
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeGetWorldSeed(int dimension, MemorySegment outSeed) {
        GetWorldSeed handler = getWorldSeed;
        if (handler == null || outSeed == null || outSeed.equals(MemorySegment.NULL)) {
            return 1;
        }
        long[] value = new long[1];
        int code = handler.get(dimension, value);
        if (code == 0) {
            outSeed.set(ValueLayout.JAVA_LONG, 0, value[0]);
        }
        return code;
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeGetPlayerAbsorption(long player, MemorySegment outAbs) {
        PlayerAbsorptionGet handler = playerAbsorptionGet;
        if (handler == null || outAbs == null || outAbs.equals(MemorySegment.NULL)) {
            return 1;
        }
        float[] value = new float[1];
        int code = handler.get(player, value);
        if (code == 0) {
            outAbs.set(ValueLayout.JAVA_FLOAT, 0, value[0]);
        }
        return code;
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeSetPlayerAbsorption(long player, float absorption) {
        PlayerAbsorptionSet handler = playerAbsorptionSet;
        if (handler == null) {
            return 1;
        }
        return handler.set(player, absorption);
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeGetPlayerInvulnerable(long player, MemorySegment out) {
        PlayerInvulnerableGet handler = playerInvulnerableGet;
        if (handler == null || out == null || out.equals(MemorySegment.NULL)) {
            return 1;
        }
        int[] value = new int[1];
        int code = handler.get(player, value);
        if (code == 0) {
            out.set(ValueLayout.JAVA_INT, 0, value[0]);
        }
        return code;
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeSetPlayerInvulnerable(long player, int invulnerable) {
        PlayerInvulnerableSet handler = playerInvulnerableSet;
        if (handler == null) {
            return 1;
        }
        return handler.set(player, invulnerable);
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeGetPlayerAir(long player, MemorySegment out) {
        PlayerAirGet handler = playerAirGet;
        if (handler == null || out == null || out.equals(MemorySegment.NULL)) {
            return 1;
        }
        int[] value = new int[1];
        int code = handler.get(player, value);
        if (code == 0) {
            out.set(ValueLayout.JAVA_INT, 0, value[0]);
        }
        return code;
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeSetPlayerAir(long player, int air) {
        PlayerAirSet handler = playerAirSet;
        if (handler == null) {
            return 1;
        }
        return handler.set(player, air);
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeGetPlayerFireTicks(long player, MemorySegment out) {
        PlayerFireTicksGet handler = playerFireTicksGet;
        if (handler == null || out == null || out.equals(MemorySegment.NULL)) {
            return 1;
        }
        int[] value = new int[1];
        int code = handler.get(player, value);
        if (code == 0) {
            out.set(ValueLayout.JAVA_INT, 0, value[0]);
        }
        return code;
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeSetPlayerFireTicks(long player, int ticks) {
        PlayerFireTicksSet handler = playerFireTicksSet;
        if (handler == null) {
            return 1;
        }
        return handler.set(player, ticks);
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeGetPlayerFrozenTicks(long player, MemorySegment out) {
        PlayerFrozenTicksGet handler = playerFrozenTicksGet;
        if (handler == null || out == null || out.equals(MemorySegment.NULL)) {
            return 1;
        }
        int[] value = new int[1];
        int code = handler.get(player, value);
        if (code == 0) {
            out.set(ValueLayout.JAVA_INT, 0, value[0]);
        }
        return code;
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeSetPlayerFrozenTicks(long player, int ticks) {
        PlayerFrozenTicksSet handler = playerFrozenTicksSet;
        if (handler == null) {
            return 1;
        }
        return handler.set(player, ticks);
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeGetPlayerNoGravity(long player, MemorySegment out) {
        PlayerNoGravityGet handler = playerNoGravityGet;
        if (handler == null || out == null || out.equals(MemorySegment.NULL)) {
            return 1;
        }
        int[] value = new int[1];
        int code = handler.get(player, value);
        if (code == 0) {
            out.set(ValueLayout.JAVA_INT, 0, value[0]);
        }
        return code;
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeSetPlayerNoGravity(long player, int noGravity) {
        PlayerNoGravitySet handler = playerNoGravitySet;
        if (handler == null) {
            return 1;
        }
        return handler.set(player, noGravity);
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeGetPlayerSilent(long player, MemorySegment out) {
        PlayerSilentGet handler = playerSilentGet;
        if (handler == null || out == null || out.equals(MemorySegment.NULL)) {
            return 1;
        }
        int[] value = new int[1];
        int code = handler.get(player, value);
        if (code == 0) {
            out.set(ValueLayout.JAVA_INT, 0, value[0]);
        }
        return code;
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeSetPlayerSilent(long player, int silent) {
        PlayerSilentSet handler = playerSilentSet;
        if (handler == null) {
            return 1;
        }
        return handler.set(player, silent);
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeGetPlayerGlowing(long player, MemorySegment out) {
        PlayerGlowingGet handler = playerGlowingGet;
        if (handler == null || out == null || out.equals(MemorySegment.NULL)) {
            return 1;
        }
        int[] value = new int[1];
        int code = handler.get(player, value);
        if (code == 0) {
            out.set(ValueLayout.JAVA_INT, 0, value[0]);
        }
        return code;
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeSetPlayerGlowing(long player, int glowing) {
        PlayerGlowingSet handler = playerGlowingSet;
        if (handler == null) {
            return 1;
        }
        return handler.set(player, glowing);
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeGetPlayerInvisible(long player, MemorySegment out) {
        PlayerInvisibleGet handler = playerInvisibleGet;
        if (handler == null || out == null || out.equals(MemorySegment.NULL)) {
            return 1;
        }
        int[] value = new int[1];
        int code = handler.get(player, value);
        if (code == 0) {
            out.set(ValueLayout.JAVA_INT, 0, value[0]);
        }
        return code;
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeSetPlayerInvisible(long player, int invisible) {
        PlayerInvisibleSet handler = playerInvisibleSet;
        if (handler == null) {
            return 1;
        }
        return handler.set(player, invisible);
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeGetPlayerPortalCooldown(long player, MemorySegment out) {
        PlayerPortalCooldownGet handler = playerPortalCooldownGet;
        if (handler == null || out == null || out.equals(MemorySegment.NULL)) {
            return 1;
        }
        int[] value = new int[1];
        int code = handler.get(player, value);
        if (code == 0) {
            out.set(ValueLayout.JAVA_INT, 0, value[0]);
        }
        return code;
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeSetPlayerPortalCooldown(long player, int ticks) {
        PlayerPortalCooldownSet handler = playerPortalCooldownSet;
        if (handler == null) {
            return 1;
        }
        return handler.set(player, ticks);
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeGetPlayerMaxAir(long player, MemorySegment out) {
        GetPlayerMaxAir handler = getPlayerMaxAir;
        if (handler == null || out == null || out.equals(MemorySegment.NULL)) {
            return 1;
        }
        int[] value = new int[1];
        int code = handler.get(player, value);
        if (code == 0) {
            out.set(ValueLayout.JAVA_INT, 0, value[0]);
        }
        return code;
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeGetBlock(
            int dimension, int x, int y, int z, MemorySegment outBlock) {
        BlockGet handler = blockGet;
        if (handler == null || outBlock == null || outBlock.equals(MemorySegment.NULL)) {
            return 1;
        }
        int[] id = new int[1];
        int code = handler.get(dimension, x, y, z, id);
        if (code == 0) {
            outBlock.set(ValueLayout.JAVA_INT, 0, id[0]);
        }
        return code;
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeSetBlock(int dimension, int x, int y, int z, int blockId) {
        BlockSet handler = blockSet;
        if (handler == null) {
            return 1;
        }
        return handler.set(dimension, x, y, z, blockId);
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeGetPlayerPos(
            long player,
            MemorySegment outX,
            MemorySegment outY,
            MemorySegment outZ,
            MemorySegment outDim) {
        PlayerPosGet handler = playerPosGet;
        if (handler == null || outX == null || outY == null || outZ == null || outDim == null
                || outX.equals(MemorySegment.NULL)
                || outY.equals(MemorySegment.NULL)
                || outZ.equals(MemorySegment.NULL)
                || outDim.equals(MemorySegment.NULL)) {
            return 1;
        }
        int[] x = new int[1];
        int[] y = new int[1];
        int[] z = new int[1];
        int[] dim = new int[1];
        int code = handler.get(player, x, y, z, dim);
        if (code == 0) {
            outX.set(ValueLayout.JAVA_INT, 0, x[0]);
            outY.set(ValueLayout.JAVA_INT, 0, y[0]);
            outZ.set(ValueLayout.JAVA_INT, 0, z[0]);
            outDim.set(ValueLayout.JAVA_INT, 0, dim[0]);
        }
        return code;
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeSetPlayerPos(long player, int x, int y, int z, int dimension) {
        PlayerPosSet handler = playerPosSet;
        if (handler == null) {
            return 1;
        }
        return handler.set(player, x, y, z, dimension);
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeGetPlayerHealth(
            long player, MemorySegment outHealth, MemorySegment outMax) {
        PlayerHealthGet handler = playerHealthGet;
        if (handler == null || outHealth == null || outMax == null
                || outHealth.equals(MemorySegment.NULL)
                || outMax.equals(MemorySegment.NULL)) {
            return 1;
        }
        float[] health = new float[1];
        float[] max = new float[1];
        int code = handler.get(player, health, max);
        if (code == 0) {
            outHealth.set(ValueLayout.JAVA_FLOAT, 0, health[0]);
            outMax.set(ValueLayout.JAVA_FLOAT, 0, max[0]);
        }
        return code;
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeSetPlayerHealth(long player, float health) {
        PlayerHealthSet handler = playerHealthSet;
        if (handler == null) {
            return 1;
        }
        return handler.set(player, health);
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeGetPlayerFood(
            long player, MemorySegment outFood, MemorySegment outSat) {
        PlayerFoodGet handler = playerFoodGet;
        if (handler == null || outFood == null || outSat == null
                || outFood.equals(MemorySegment.NULL)
                || outSat.equals(MemorySegment.NULL)) {
            return 1;
        }
        int[] food = new int[1];
        float[] sat = new float[1];
        int code = handler.get(player, food, sat);
        if (code == 0) {
            outFood.set(ValueLayout.JAVA_INT, 0, food[0]);
            outSat.set(ValueLayout.JAVA_FLOAT, 0, sat[0]);
        }
        return code;
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeSetPlayerFood(long player, int food, float saturation) {
        PlayerFoodSet handler = playerFoodSet;
        if (handler == null) {
            return 1;
        }
        return handler.set(player, food, saturation);
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeGetPlayerGamemode(long player, MemorySegment outMode) {
        PlayerGamemodeGet handler = playerGamemodeGet;
        if (handler == null || outMode == null || outMode.equals(MemorySegment.NULL)) {
            return 1;
        }
        int[] mode = new int[1];
        int code = handler.get(player, mode);
        if (code == 0) {
            outMode.set(ValueLayout.JAVA_INT, 0, mode[0]);
        }
        return code;
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeSetPlayerGamemode(long player, int mode) {
        PlayerGamemodeSet handler = playerGamemodeSet;
        if (handler == null) {
            return 1;
        }
        return handler.set(player, mode);
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeGetPlayerXp(
            long player, MemorySegment outLevel, MemorySegment outProgress) {
        PlayerXpGet handler = playerXpGet;
        if (handler == null || outLevel == null || outProgress == null
                || outLevel.equals(MemorySegment.NULL)
                || outProgress.equals(MemorySegment.NULL)) {
            return 1;
        }
        int[] level = new int[1];
        float[] progress = new float[1];
        int code = handler.get(player, level, progress);
        if (code == 0) {
            outLevel.set(ValueLayout.JAVA_INT, 0, level[0]);
            outProgress.set(ValueLayout.JAVA_FLOAT, 0, progress[0]);
        }
        return code;
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeSetPlayerXpLevel(long player, int level) {
        PlayerXpSet handler = playerXpSet;
        if (handler == null) {
            return 1;
        }
        return handler.set(player, level);
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeGetPlayerLook(
            long player, MemorySegment outYaw, MemorySegment outPitch) {
        PlayerLookGet handler = playerLookGet;
        if (handler == null || outYaw == null || outPitch == null
                || outYaw.equals(MemorySegment.NULL)
                || outPitch.equals(MemorySegment.NULL)) {
            return 1;
        }
        float[] yaw = new float[1];
        float[] pitch = new float[1];
        int code = handler.get(player, yaw, pitch);
        if (code == 0) {
            outYaw.set(ValueLayout.JAVA_FLOAT, 0, yaw[0]);
            outPitch.set(ValueLayout.JAVA_FLOAT, 0, pitch[0]);
        }
        return code;
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeSetPlayerLook(long player, float yaw, float pitch) {
        PlayerLookSet handler = playerLookSet;
        if (handler == null) {
            return 1;
        }
        return handler.set(player, yaw, pitch);
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativePlaySound(
            long player,
            MemorySegment soundIdSeg,
            float volume,
            float pitch,
            int x,
            int y,
            int z,
            int dimension) {
        PlaySound handler = playSound;
        if (handler == null || soundIdSeg == null || soundIdSeg.equals(MemorySegment.NULL)) {
            return 1;
        }
        String soundId = soundIdSeg.getString(0, StandardCharsets.UTF_8);
        return handler.play(player, soundId, volume, pitch, x, y, z, dimension);
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeSendActionbar(long player, MemorySegment messageSeg) {
        ActionbarSend handler = actionbarSend;
        if (handler == null || messageSeg == null || messageSeg.equals(MemorySegment.NULL)) {
            return 1;
        }
        String message = messageSeg.getString(0, StandardCharsets.UTF_8);
        return handler.send(player, message);
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeSendTitle(
            long player,
            MemorySegment titleSeg,
            MemorySegment subtitleSeg,
            int fadeIn,
            int stay,
            int fadeOut) {
        TitleSend handler = titleSend;
        if (handler == null) {
            return 1;
        }
        String title = titleSeg == null || titleSeg.equals(MemorySegment.NULL)
                ? ""
                : titleSeg.getString(0, StandardCharsets.UTF_8);
        String subtitle = subtitleSeg == null || subtitleSeg.equals(MemorySegment.NULL)
                ? ""
                : subtitleSeg.getString(0, StandardCharsets.UTF_8);
        return handler.send(player, title, subtitle, fadeIn, stay, fadeOut);
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeKickPlayer(long player, MemorySegment reasonSeg) {
        KickPlayer handler = kickPlayer;
        if (handler == null) {
            return 1;
        }
        String reason = reasonSeg == null || reasonSeg.equals(MemorySegment.NULL)
                ? ""
                : reasonSeg.getString(0, StandardCharsets.UTF_8);
        return handler.kick(player, reason);
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeGiveItem(long player, int itemId, int count, int damage) {
        GiveItem handler = giveItem;
        if (handler == null) {
            return 1;
        }
        return handler.give(player, itemId, count, damage);
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeGiveItemRegistry(
            long player, MemorySegment itemIdSeg, int count, int damage) {
        GiveItemRegistry handler = giveItemRegistry;
        if (handler == null || itemIdSeg == null || itemIdSeg.equals(MemorySegment.NULL)) {
            return 1;
        }
        String itemId = itemIdSeg.getString(0, StandardCharsets.UTF_8);
        return handler.give(player, itemId, count, damage);
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeApplyEffect(
            long player,
            MemorySegment effectIdSeg,
            int durationTicks,
            int amplifier,
            int flags) {
        ApplyEffect handler = applyEffect;
        if (handler == null || effectIdSeg == null || effectIdSeg.equals(MemorySegment.NULL)) {
            return 1;
        }
        String effectId = effectIdSeg.getString(0, StandardCharsets.UTF_8);
        return handler.apply(player, effectId, durationTicks, amplifier, flags);
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeClearEffects(long player) {
        ClearEffects handler = clearEffects;
        if (handler == null) {
            return 1;
        }
        return handler.clear(player);
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeSpawnParticle(
            MemorySegment particleIdSeg,
            double x,
            double y,
            double z,
            int dimension,
            int count,
            double dx,
            double dy,
            double dz,
            double speed) {
        SpawnParticle handler = spawnParticle;
        if (handler == null || particleIdSeg == null
                || particleIdSeg.equals(MemorySegment.NULL)) {
            return 1;
        }
        String particleId = particleIdSeg.getString(0, StandardCharsets.UTF_8);
        return handler.spawn(
                particleId, x, y, z, dimension, count, dx, dy, dz, speed);
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeGetWorldTime(int dimension, MemorySegment outTime) {
        WorldTimeGet handler = worldTimeGet;
        if (handler == null || outTime == null || outTime.equals(MemorySegment.NULL)) {
            return 1;
        }
        long[] time = new long[1];
        int code = handler.get(dimension, time);
        if (code == 0) {
            outTime.set(ValueLayout.JAVA_LONG, 0, time[0]);
        }
        return code;
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeSetWorldTime(int dimension, long time) {
        WorldTimeSet handler = worldTimeSet;
        if (handler == null) {
            return 1;
        }
        return handler.set(dimension, time);
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeGetPlayerVelocity(
            long player,
            MemorySegment outVx,
            MemorySegment outVy,
            MemorySegment outVz) {
        PlayerVelocityGet handler = playerVelocityGet;
        if (handler == null || outVx == null || outVy == null || outVz == null
                || outVx.equals(MemorySegment.NULL)
                || outVy.equals(MemorySegment.NULL)
                || outVz.equals(MemorySegment.NULL)) {
            return 1;
        }
        double[] vx = new double[1];
        double[] vy = new double[1];
        double[] vz = new double[1];
        int code = handler.get(player, vx, vy, vz);
        if (code == 0) {
            outVx.set(ValueLayout.JAVA_DOUBLE, 0, vx[0]);
            outVy.set(ValueLayout.JAVA_DOUBLE, 0, vy[0]);
            outVz.set(ValueLayout.JAVA_DOUBLE, 0, vz[0]);
        }
        return code;
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeSetPlayerVelocity(
            long player, double vx, double vy, double vz) {
        PlayerVelocitySet handler = playerVelocitySet;
        if (handler == null) {
            return 1;
        }
        return handler.set(player, vx, vy, vz);
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeGetPlayerFlags(long player, MemorySegment outFlags) {
        PlayerFlagsGet handler = playerFlagsGet;
        if (handler == null || outFlags == null || outFlags.equals(MemorySegment.NULL)) {
            return 1;
        }
        int[] flags = new int[1];
        int code = handler.get(player, flags);
        if (code == 0) {
            outFlags.set(ValueLayout.JAVA_INT, 0, flags[0]);
        }
        return code;
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeRunCommand(long player, MemorySegment commandSeg) {
        RunCommand handler = runCommand;
        if (handler == null || commandSeg == null || commandSeg.equals(MemorySegment.NULL)) {
            return 1;
        }
        String command = commandSeg.getString(0, StandardCharsets.UTF_8);
        return handler.run(player, command);
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeClearInventory(long player) {
        ClearInventory handler = clearInventory;
        if (handler == null) {
            return 1;
        }
        return handler.clear(player);
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeSetPlayerFlight(long player, int allowFlight, int flying) {
        PlayerFlight handler = playerFlight;
        if (handler == null) {
            return 1;
        }
        return handler.set(player, allowFlight, flying);
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeGetBiome(
            int dimension, int x, int y, int z, MemorySegment outBuf, int outSize) {
        GetBiome handler = getBiome;
        if (handler == null || outBuf == null || outBuf.equals(MemorySegment.NULL)
                || outSize <= 0) {
            return 1;
        }
        String[] outId = new String[1];
        int code = handler.get(dimension, x, y, z, outId);
        if (code != 0) {
            return code;
        }
        String id = outId[0] != null ? outId[0] : "minecraft:plains";
        byte[] utf8 = id.getBytes(StandardCharsets.UTF_8);
        int n = Math.min(utf8.length, outSize - 1);
        for (int i = 0; i < n; i++) {
            outBuf.set(ValueLayout.JAVA_BYTE, i, utf8[i]);
        }
        outBuf.set(ValueLayout.JAVA_BYTE, n, (byte) 0);
        return 0;
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeGetDifficulty(MemorySegment outDifficulty) {
        DifficultyGet handler = difficultyGet;
        if (handler == null || outDifficulty == null
                || outDifficulty.equals(MemorySegment.NULL)) {
            return 1;
        }
        int[] value = new int[1];
        int code = handler.get(value);
        if (code == 0) {
            outDifficulty.set(ValueLayout.JAVA_INT, 0, value[0]);
        }
        return code;
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeSetDifficulty(int difficulty) {
        DifficultySet handler = difficultySet;
        if (handler == null) {
            return 1;
        }
        return handler.set(difficulty);
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeGetWeather(int dimension, MemorySegment outWeather) {
        WeatherGet handler = weatherGet;
        if (handler == null || outWeather == null || outWeather.equals(MemorySegment.NULL)) {
            return 1;
        }
        int[] value = new int[1];
        int code = handler.get(dimension, value);
        if (code == 0) {
            outWeather.set(ValueLayout.JAVA_INT, 0, value[0]);
        }
        return code;
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeSetWeather(int dimension, int weather, int durationTicks) {
        WeatherSet handler = weatherSet;
        if (handler == null) {
            return 1;
        }
        return handler.set(dimension, weather, durationTicks);
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeGetLightLevel(
            int dimension,
            int x,
            int y,
            int z,
            MemorySegment outBlock,
            MemorySegment outSky) {
        GetLightLevel handler = getLightLevel;
        if (handler == null || outBlock == null || outSky == null
                || outBlock.equals(MemorySegment.NULL)
                || outSky.equals(MemorySegment.NULL)) {
            return 1;
        }
        int[] block = new int[1];
        int[] sky = new int[1];
        int code = handler.get(dimension, x, y, z, block, sky);
        if (code == 0) {
            outBlock.set(ValueLayout.JAVA_INT, 0, block[0]);
            outSky.set(ValueLayout.JAVA_INT, 0, sky[0]);
        }
        return code;
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeGetPlayerLatency(long player, MemorySegment outMs) {
        PlayerLatency handler = playerLatency;
        if (handler == null || outMs == null || outMs.equals(MemorySegment.NULL)) {
            return 1;
        }
        int[] value = new int[1];
        int code = handler.get(player, value);
        if (code == 0) {
            outMs.set(ValueLayout.JAVA_INT, 0, value[0]);
        }
        return code;
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeGetWorldSpawn(
            int dimension, MemorySegment outX, MemorySegment outY, MemorySegment outZ) {
        WorldSpawnGet handler = worldSpawnGet;
        if (handler == null || outX == null || outY == null || outZ == null
                || outX.equals(MemorySegment.NULL)
                || outY.equals(MemorySegment.NULL)
                || outZ.equals(MemorySegment.NULL)) {
            return 1;
        }
        int[] x = new int[1];
        int[] y = new int[1];
        int[] z = new int[1];
        int code = handler.get(dimension, x, y, z);
        if (code == 0) {
            outX.set(ValueLayout.JAVA_INT, 0, x[0]);
            outY.set(ValueLayout.JAVA_INT, 0, y[0]);
            outZ.set(ValueLayout.JAVA_INT, 0, z[0]);
        }
        return code;
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeSetWorldSpawn(int dimension, int x, int y, int z) {
        WorldSpawnSet handler = worldSpawnSet;
        if (handler == null) {
            return 1;
        }
        return handler.set(dimension, x, y, z);
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeIsPlayerOp(long player, MemorySegment outOp) {
        PlayerOp handler = playerOp;
        if (handler == null || outOp == null || outOp.equals(MemorySegment.NULL)) {
            return 1;
        }
        int[] value = new int[1];
        int code = handler.get(player, value);
        if (code == 0) {
            outOp.set(ValueLayout.JAVA_INT, 0, value[0]);
        }
        return code;
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeGetPlayerUuid(long player, MemorySegment outBuf, int outSize) {
        PlayerUuid handler = playerUuid;
        if (handler == null || outBuf == null || outBuf.equals(MemorySegment.NULL)
                || outSize <= 0) {
            return 1;
        }
        String[] outUuid = new String[1];
        int code = handler.get(player, outUuid);
        if (code != 0) {
            return code;
        }
        String id = outUuid[0] != null ? outUuid[0] : "";
        byte[] utf8 = id.getBytes(StandardCharsets.UTF_8);
        int n = Math.min(utf8.length, outSize - 1);
        for (int i = 0; i < n; i++) {
            outBuf.set(ValueLayout.JAVA_BYTE, i, utf8[i]);
        }
        outBuf.set(ValueLayout.JAVA_BYTE, n, (byte) 0);
        return 0;
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeGetPlayerPermission(long player, MemorySegment outLevel) {
        PlayerPermission handler = playerPermission;
        if (handler == null || outLevel == null || outLevel.equals(MemorySegment.NULL)) {
            return 1;
        }
        int[] value = new int[1];
        int code = handler.get(player, value);
        if (code == 0) {
            outLevel.set(ValueLayout.JAVA_INT, 0, value[0]);
        }
        return code;
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeFindPlayerUuid(MemorySegment uuidSeg, MemorySegment outHandle) {
        FindPlayerUuid handler = findPlayerUuid;
        if (handler == null || uuidSeg == null || uuidSeg.equals(MemorySegment.NULL)
                || outHandle == null || outHandle.equals(MemorySegment.NULL)) {
            return 1;
        }
        String uuid = uuidSeg.getString(0, StandardCharsets.UTF_8);
        long[] handle = new long[1];
        int code = handler.find(uuid, handle);
        if (code == 0) {
            outHandle.set(ValueLayout.JAVA_LONG, 0, handle[0]);
        }
        return code;
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeFindPlayerName(MemorySegment nameSeg, MemorySegment outHandle) {
        FindPlayerName handler = findPlayerName;
        if (handler == null || nameSeg == null || nameSeg.equals(MemorySegment.NULL)
                || outHandle == null || outHandle.equals(MemorySegment.NULL)) {
            return 1;
        }
        String name = nameSeg.getString(0, StandardCharsets.UTF_8);
        long[] handle = new long[1];
        int code = handler.find(name, handle);
        if (code == 0) {
            outHandle.set(ValueLayout.JAVA_LONG, 0, handle[0]);
        }
        return code;
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeGetBlockRegistry(
            int dimension, int x, int y, int z, MemorySegment outBuf, int outSize) {
        GetBlockRegistry handler = getBlockRegistry;
        if (handler == null || outBuf == null || outBuf.equals(MemorySegment.NULL)
                || outSize <= 0) {
            return 1;
        }
        String[] outId = new String[1];
        int code = handler.get(dimension, x, y, z, outId);
        if (code != 0) {
            return code;
        }
        String id = outId[0] != null ? outId[0] : "minecraft:air";
        byte[] utf8 = id.getBytes(StandardCharsets.UTF_8);
        int n = Math.min(utf8.length, outSize - 1);
        for (int i = 0; i < n; i++) {
            outBuf.set(ValueLayout.JAVA_BYTE, i, utf8[i]);
        }
        outBuf.set(ValueLayout.JAVA_BYTE, n, (byte) 0);
        return 0;
    }

    @SuppressWarnings("unused") // FFM upcall target
    private static int nativeSetBlockRegistry(
            int dimension, int x, int y, int z, MemorySegment blockIdSeg) {
        SetBlockRegistry handler = setBlockRegistry;
        if (handler == null || blockIdSeg == null || blockIdSeg.equals(MemorySegment.NULL)) {
            return 1;
        }
        String blockId = blockIdSeg.getString(0, StandardCharsets.UTF_8);
        return handler.set(dimension, x, y, z, blockId);
    }
}
