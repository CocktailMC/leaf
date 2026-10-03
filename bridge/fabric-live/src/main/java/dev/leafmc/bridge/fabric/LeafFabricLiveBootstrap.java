package dev.leafmc.bridge.fabric;

import dev.leafmc.bridge.LeafBridge;
import dev.leafmc.bridge.LeafMinecraftHooks;
import dev.leafmc.bridge.LeafPlayerHandles;
import dev.leafmc.bridge.LeafRuntime;
import net.fabricmc.api.ModInitializer;
import net.fabricmc.fabric.api.event.lifecycle.v1.ServerEntityEvents;
import net.fabricmc.fabric.api.event.lifecycle.v1.ServerLifecycleEvents;
import net.fabricmc.fabric.api.event.lifecycle.v1.ServerTickEvents;
import net.fabricmc.fabric.api.event.lifecycle.v1.ServerWorldEvents;
import net.fabricmc.fabric.api.entity.event.v1.ServerLivingEntityEvents;
import net.fabricmc.fabric.api.event.player.PlayerBlockBreakEvents;
import net.fabricmc.fabric.api.event.player.UseBlockCallback;
import net.fabricmc.fabric.api.message.v1.ServerMessageEvents;
import net.fabricmc.fabric.api.networking.v1.ServerPlayConnectionEvents;
import net.minecraft.entity.Entity;
import net.minecraft.entity.effect.StatusEffectInstance;
import net.minecraft.item.BlockItem;
import net.minecraft.item.ItemStack;
import net.minecraft.network.packet.s2c.play.SubtitleS2CPacket;
import net.minecraft.network.packet.s2c.play.TitleFadeS2CPacket;
import net.minecraft.network.packet.s2c.play.TitleS2CPacket;
import net.minecraft.particle.SimpleParticleType;
import net.minecraft.registry.Registries;
import net.minecraft.server.MinecraftServer;
import net.minecraft.server.network.ServerPlayerEntity;
import net.minecraft.server.world.ServerWorld;
import net.minecraft.sound.SoundCategory;
import net.minecraft.sound.SoundEvent;
import net.minecraft.text.Text;
import net.minecraft.util.ActionResult;
import net.minecraft.util.Hand;
import net.minecraft.util.Identifier;
import net.minecraft.util.math.BlockPos;
import net.minecraft.world.GameMode;
import net.minecraft.world.World;

import java.util.Map;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicReference;

/**
 * Live Fabric bootstrap compiled against real Fabric API + Yarn (Loom).
 */
public final class LeafFabricLiveBootstrap implements ModInitializer {
    private static final Map<Long, ServerPlayerEntity> PLAYERS = new ConcurrentHashMap<>();
    private static final AtomicReference<MinecraftServer> SERVER = new AtomicReference<>();

    @Override
    public void onInitialize() {
        LeafRuntime.start(LeafBridge.LOADER_FABRIC, LeafBridge.MAPPING_INTERMEDIARY);

        LeafMinecraftHooks.installDefaultSend((handle, message) -> {
            ServerPlayerEntity player = PLAYERS.get(handle);
            if (player != null) {
                player.sendMessage(Text.literal(message));
            }
        });
        LeafMinecraftHooks.installBroadcast(message -> {
            MinecraftServer server = SERVER.get();
            if (server != null) {
                server.getPlayerManager().broadcast(Text.literal(message), false);
            }
        });
        LeafMinecraftHooks.installInventory(
                (handle, slot, itemIdOut, countOut) -> {
                    ServerPlayerEntity player = PLAYERS.get(handle);
                    if (player == null || slot < 0 || slot > 40) {
                        return 1;
                    }
                    ItemStack stack = player.getInventory().getStack(slot);
                    itemIdOut[0] = stack.isEmpty()
                            ? 0
                            : Registries.ITEM.getRawId(stack.getItem());
                    countOut[0] = stack.isEmpty() ? 0 : stack.getCount();
                    return 0;
                },
                (handle, slot, itemId, count) -> {
                    ServerPlayerEntity player = PLAYERS.get(handle);
                    if (player == null || slot < 0 || slot > 40) {
                        return 1;
                    }
                    if (itemId == 0 || count <= 0) {
                        player.getInventory().setStack(slot, ItemStack.EMPTY);
                        return 0;
                    }
                    var item = Registries.ITEM.get(itemId);
                    if (item == null) {
                        return 1;
                    }
                    player.getInventory().setStack(slot, new ItemStack(item, count));
                    return 0;
                });
        LeafMinecraftHooks.installInventoryStack(
                (handle, slot, itemIdOut, countOut, damageOut) -> {
                    ServerPlayerEntity player = PLAYERS.get(handle);
                    if (player == null || slot < 0 || slot > 40) {
                        return 1;
                    }
                    ItemStack stack = player.getInventory().getStack(slot);
                    if (stack.isEmpty()) {
                        itemIdOut[0] = 0;
                        countOut[0] = 0;
                        damageOut[0] = -1;
                        return 0;
                    }
                    itemIdOut[0] = Registries.ITEM.getRawId(stack.getItem());
                    countOut[0] = stack.getCount();
                    damageOut[0] = stack.isDamageable() ? stack.getDamage() : -1;
                    return 0;
                },
                (handle, slot, itemId, count, damage) -> {
                    ServerPlayerEntity player = PLAYERS.get(handle);
                    if (player == null || slot < 0 || slot > 40) {
                        return 1;
                    }
                    if (itemId == 0 || count <= 0) {
                        player.getInventory().setStack(slot, ItemStack.EMPTY);
                        return 0;
                    }
                    var item = Registries.ITEM.get(itemId);
                    if (item == null) {
                        return 1;
                    }
                    ItemStack stack = new ItemStack(item, count);
                    if (damage >= 0 && stack.isDamageable()) {
                        stack.setDamage(damage);
                    }
                    player.getInventory().setStack(slot, stack);
                    return 0;
                });
        LeafMinecraftHooks.installInventoryRegistry(
                (handle, slot, outId, countOut, damageOut) -> {
                    ServerPlayerEntity player = PLAYERS.get(handle);
                    if (player == null || slot < 0 || slot > 40) {
                        return 1;
                    }
                    ItemStack stack = player.getInventory().getStack(slot);
                    if (stack.isEmpty()) {
                        outId[0] = "minecraft:air";
                        countOut[0] = 0;
                        damageOut[0] = -1;
                        return 0;
                    }
                    Identifier id = Registries.ITEM.getId(stack.getItem());
                    outId[0] = id != null ? id.toString() : "minecraft:air";
                    countOut[0] = stack.getCount();
                    damageOut[0] = stack.isDamageable() ? stack.getDamage() : -1;
                    return 0;
                },
                (handle, slot, itemId, count, damage) -> {
                    ServerPlayerEntity player = PLAYERS.get(handle);
                    if (player == null || slot < 0 || slot > 40) {
                        return 1;
                    }
                    if (itemId == null || itemId.isEmpty() || itemId.equals("minecraft:air")
                            || itemId.equals("air") || count <= 0) {
                        player.getInventory().setStack(slot, ItemStack.EMPTY);
                        return 0;
                    }
                    Identifier id = Identifier.tryParse(itemId);
                    if (id == null) {
                        return 1;
                    }
                    var item = Registries.ITEM.get(id);
                    if (item == null) {
                        return 1;
                    }
                    ItemStack stack = new ItemStack(item, count);
                    if (damage >= 0 && stack.isDamageable()) {
                        stack.setDamage(damage);
                    }
                    player.getInventory().setStack(slot, stack);
                    return 0;
                });
        LeafMinecraftHooks.installWorld(
                (dimension, x, y, z, blockIdOut) -> {
                    MinecraftServer server = SERVER.get();
                    if (server == null) {
                        return 1;
                    }
                    var world = switch (dimension) {
                        case 1 -> server.getWorld(net.minecraft.world.World.NETHER);
                        case 2 -> server.getWorld(net.minecraft.world.World.END);
                        default -> server.getOverworld();
                    };
                    if (world == null) {
                        return 1;
                    }
                    var state = world.getBlockState(new BlockPos(x, y, z));
                    blockIdOut[0] = Registries.BLOCK.getRawId(state.getBlock());
                    return 0;
                },
                (dimension, x, y, z, blockId) -> {
                    MinecraftServer server = SERVER.get();
                    if (server == null) {
                        return 1;
                    }
                    var world = switch (dimension) {
                        case 1 -> server.getWorld(net.minecraft.world.World.NETHER);
                        case 2 -> server.getWorld(net.minecraft.world.World.END);
                        default -> server.getOverworld();
                    };
                    if (world == null) {
                        return 1;
                    }
                    var block = Registries.BLOCK.get(blockId);
                    if (block == null) {
                        return 1;
                    }
                    world.setBlockState(new BlockPos(x, y, z), block.getDefaultState());
                    return 0;
                });
        LeafMinecraftHooks.installPlayerPos(
                (handle, xOut, yOut, zOut, dimOut) -> {
                    ServerPlayerEntity player = PLAYERS.get(handle);
                    if (player == null) {
                        return 1;
                    }
                    xOut[0] = player.getBlockX();
                    yOut[0] = player.getBlockY();
                    zOut[0] = player.getBlockZ();
                    var key = player.getServerWorld().getRegistryKey();
                    if (key == World.NETHER) {
                        dimOut[0] = 1;
                    } else if (key == World.END) {
                        dimOut[0] = 2;
                    } else {
                        dimOut[0] = 0;
                    }
                    return 0;
                },
                (handle, x, y, z, dimension) -> {
                    ServerPlayerEntity player = PLAYERS.get(handle);
                    MinecraftServer server = SERVER.get();
                    if (player == null || server == null) {
                        return 1;
                    }
                    ServerWorld world = switch (dimension) {
                        case 1 -> server.getWorld(World.NETHER);
                        case 2 -> server.getWorld(World.END);
                        default -> server.getOverworld();
                    };
                    if (world == null) {
                        return 1;
                    }
                    player.teleport(
                            world,
                            x + 0.5,
                            y,
                            z + 0.5,
                            Set.of(),
                            player.getYaw(),
                            player.getPitch());
                    return 0;
                });
        LeafMinecraftHooks.installTeleport((handle, x, y, z, dimension, yaw, pitch) -> {
            ServerPlayerEntity player = PLAYERS.get(handle);
            MinecraftServer server = SERVER.get();
            if (player == null || server == null) {
                return 1;
            }
            ServerWorld world = switch (dimension) {
                case 1 -> server.getWorld(World.NETHER);
                case 2 -> server.getWorld(World.END);
                default -> server.getOverworld();
            };
            if (world == null) {
                return 1;
            }
            player.teleport(world, x + 0.5, y, z + 0.5, Set.of(), yaw, pitch);
            return 0;
        });
        LeafMinecraftHooks.installSelectedSlot(
                (handle, slotOut) -> {
                    ServerPlayerEntity player = PLAYERS.get(handle);
                    if (player == null) {
                        return 1;
                    }
                    slotOut[0] = player.getInventory().selectedSlot;
                    return 0;
                },
                (handle, slot) -> {
                    ServerPlayerEntity player = PLAYERS.get(handle);
                    if (player == null || slot < 0 || slot > 8) {
                        return 1;
                    }
                    player.getInventory().selectedSlot = slot;
                    return 0;
                });
        LeafMinecraftHooks.installGetWorldSeed((dimension, seedOut) -> {
            MinecraftServer server = SERVER.get();
            if (server == null) {
                return 1;
            }
            ServerWorld world = switch (dimension) {
                case 1 -> server.getWorld(World.NETHER);
                case 2 -> server.getWorld(World.END);
                default -> server.getWorld(World.OVERWORLD);
            };
            if (world == null) {
                return 1;
            }
            seedOut[0] = world.getSeed();
            return 0;
        });
        LeafMinecraftHooks.installPlayerHealth(
                (handle, healthOut, maxOut) -> {
                    ServerPlayerEntity player = PLAYERS.get(handle);
                    if (player == null) {
                        return 1;
                    }
                    healthOut[0] = player.getHealth();
                    maxOut[0] = player.getMaxHealth();
                    return 0;
                },
                (handle, health) -> {
                    ServerPlayerEntity player = PLAYERS.get(handle);
                    if (player == null) {
                        return 1;
                    }
                    player.setHealth(health);
                    return 0;
                });
        LeafMinecraftHooks.installPlayerAbsorption(
                (handle, absOut) -> {
                    ServerPlayerEntity player = PLAYERS.get(handle);
                    if (player == null) {
                        return 1;
                    }
                    absOut[0] = player.getAbsorptionAmount();
                    return 0;
                },
                (handle, absorption) -> {
                    ServerPlayerEntity player = PLAYERS.get(handle);
                    if (player == null || absorption < 0.0f) {
                        return 1;
                    }
                    player.setAbsorptionAmount(absorption);
                    return 0;
                });
        LeafMinecraftHooks.installPlayerInvulnerable(
                (handle, out) -> {
                    ServerPlayerEntity player = PLAYERS.get(handle);
                    if (player == null) {
                        return 1;
                    }
                    out[0] = player.isInvulnerable() ? 1 : 0;
                    return 0;
                },
                (handle, invulnerable) -> {
                    ServerPlayerEntity player = PLAYERS.get(handle);
                    if (player == null) {
                        return 1;
                    }
                    player.setInvulnerable(invulnerable != 0);
                    return 0;
                });
        LeafMinecraftHooks.installPlayerAir(
                (handle, out) -> {
                    ServerPlayerEntity player = PLAYERS.get(handle);
                    if (player == null) {
                        return 1;
                    }
                    out[0] = player.getAir();
                    return 0;
                },
                (handle, air) -> {
                    ServerPlayerEntity player = PLAYERS.get(handle);
                    if (player == null) {
                        return 1;
                    }
                    player.setAir(air);
                    return 0;
                });
        LeafMinecraftHooks.installPlayerFireTicks(
                (handle, out) -> {
                    ServerPlayerEntity player = PLAYERS.get(handle);
                    if (player == null) {
                        return 1;
                    }
                    out[0] = player.getFireTicks();
                    return 0;
                },
                (handle, ticks) -> {
                    ServerPlayerEntity player = PLAYERS.get(handle);
                    if (player == null) {
                        return 1;
                    }
                    player.setFireTicks(ticks);
                    return 0;
                });
        LeafMinecraftHooks.installPlayerFrozenTicks(
                (handle, out) -> {
                    ServerPlayerEntity player = PLAYERS.get(handle);
                    if (player == null) {
                        return 1;
                    }
                    out[0] = player.getFrozenTicks();
                    return 0;
                },
                (handle, ticks) -> {
                    ServerPlayerEntity player = PLAYERS.get(handle);
                    if (player == null) {
                        return 1;
                    }
                    player.setFrozenTicks(ticks);
                    return 0;
                });
        LeafMinecraftHooks.installPlayerNoGravity(
                (handle, out) -> {
                    ServerPlayerEntity player = PLAYERS.get(handle);
                    if (player == null) {
                        return 1;
                    }
                    out[0] = player.hasNoGravity() ? 1 : 0;
                    return 0;
                },
                (handle, noGravity) -> {
                    ServerPlayerEntity player = PLAYERS.get(handle);
                    if (player == null) {
                        return 1;
                    }
                    player.setNoGravity(noGravity != 0);
                    return 0;
                });
        LeafMinecraftHooks.installPlayerSilent(
                (handle, out) -> {
                    ServerPlayerEntity player = PLAYERS.get(handle);
                    if (player == null) {
                        return 1;
                    }
                    out[0] = player.isSilent() ? 1 : 0;
                    return 0;
                },
                (handle, silent) -> {
                    ServerPlayerEntity player = PLAYERS.get(handle);
                    if (player == null) {
                        return 1;
                    }
                    player.setSilent(silent != 0);
                    return 0;
                });
        LeafMinecraftHooks.installPlayerGlowing(
                (handle, out) -> {
                    ServerPlayerEntity player = PLAYERS.get(handle);
                    if (player == null) {
                        return 1;
                    }
                    out[0] = player.isGlowing() ? 1 : 0;
                    return 0;
                },
                (handle, glowing) -> {
                    ServerPlayerEntity player = PLAYERS.get(handle);
                    if (player == null) {
                        return 1;
                    }
                    player.setGlowing(glowing != 0);
                    return 0;
                });
        LeafMinecraftHooks.installPlayerInvisible(
                (handle, out) -> {
                    ServerPlayerEntity player = PLAYERS.get(handle);
                    if (player == null) {
                        return 1;
                    }
                    out[0] = player.isInvisible() ? 1 : 0;
                    return 0;
                },
                (handle, invisible) -> {
                    ServerPlayerEntity player = PLAYERS.get(handle);
                    if (player == null) {
                        return 1;
                    }
                    player.setInvisible(invisible != 0);
                    return 0;
                });
        LeafMinecraftHooks.installPlayerPortalCooldown(
                (handle, out) -> {
                    ServerPlayerEntity player = PLAYERS.get(handle);
                    if (player == null) {
                        return 1;
                    }
                    out[0] = player.getPortalCooldown();
                    return 0;
                },
                (handle, ticks) -> {
                    ServerPlayerEntity player = PLAYERS.get(handle);
                    if (player == null) {
                        return 1;
                    }
                    player.setPortalCooldown(ticks);
                    return 0;
                });
        LeafMinecraftHooks.installGetPlayerMaxAir((handle, out) -> {
            ServerPlayerEntity player = PLAYERS.get(handle);
            if (player == null) {
                return 1;
            }
            out[0] = player.getMaxAir();
            return 0;
        });
        LeafMinecraftHooks.installPlayerFood(
                (handle, foodOut, satOut) -> {
                    ServerPlayerEntity player = PLAYERS.get(handle);
                    if (player == null) {
                        return 1;
                    }
                    var hunger = player.getHungerManager();
                    foodOut[0] = hunger.getFoodLevel();
                    satOut[0] = hunger.getSaturationLevel();
                    return 0;
                },
                (handle, food, saturation) -> {
                    ServerPlayerEntity player = PLAYERS.get(handle);
                    if (player == null) {
                        return 1;
                    }
                    var hunger = player.getHungerManager();
                    hunger.setFoodLevel(food);
                    hunger.setSaturationLevel(saturation);
                    return 0;
                });
        LeafMinecraftHooks.installPlayerGamemode(
                (handle, modeOut) -> {
                    ServerPlayerEntity player = PLAYERS.get(handle);
                    if (player == null) {
                        return 1;
                    }
                    modeOut[0] = player.interactionManager.getGameMode().getId();
                    return 0;
                },
                (handle, mode) -> {
                    ServerPlayerEntity player = PLAYERS.get(handle);
                    if (player == null || mode < 0 || mode > 3) {
                        return 1;
                    }
                    player.changeGameMode(GameMode.byId(mode));
                    return 0;
                });
        LeafMinecraftHooks.installPlayerXp(
                (handle, levelOut, progressOut) -> {
                    ServerPlayerEntity player = PLAYERS.get(handle);
                    if (player == null) {
                        return 1;
                    }
                    levelOut[0] = player.experienceLevel;
                    progressOut[0] = player.experienceProgress;
                    return 0;
                },
                (handle, level) -> {
                    ServerPlayerEntity player = PLAYERS.get(handle);
                    if (player == null || level < 0) {
                        return 1;
                    }
                    player.experienceLevel = level;
                    player.experienceProgress = 0.0f;
                    return 0;
                });
        LeafMinecraftHooks.installPlayerLook(
                (handle, yawOut, pitchOut) -> {
                    ServerPlayerEntity player = PLAYERS.get(handle);
                    if (player == null) {
                        return 1;
                    }
                    yawOut[0] = player.getYaw();
                    pitchOut[0] = player.getPitch();
                    return 0;
                },
                (handle, yaw, pitch) -> {
                    ServerPlayerEntity player = PLAYERS.get(handle);
                    if (player == null) {
                        return 1;
                    }
                    player.setYaw(yaw);
                    player.setPitch(pitch);
                    return 0;
                });
        LeafMinecraftHooks.installPlaySound(
                (handle, soundId, volume, pitch, x, y, z, dimension) -> {
                    Identifier id = Identifier.tryParse(soundId);
                    if (id == null) {
                        return 1;
                    }
                    var opt = Registries.SOUND_EVENT.getOrEmpty(id);
                    if (opt.isEmpty()) {
                        return 1;
                    }
                    SoundEvent sound = opt.get();
                    if (handle != 0) {
                        ServerPlayerEntity player = PLAYERS.get(handle);
                        if (player == null) {
                            return 1;
                        }
                        player.playSoundToPlayer(sound, SoundCategory.MASTER, volume, pitch);
                        return 0;
                    }
                    MinecraftServer server = SERVER.get();
                    if (server == null) {
                        return 1;
                    }
                    ServerWorld world = switch (dimension) {
                        case 1 -> server.getWorld(World.NETHER);
                        case 2 -> server.getWorld(World.END);
                        default -> server.getWorld(World.OVERWORLD);
                    };
                    if (world == null) {
                        return 1;
                    }
                    world.playSound(
                            null,
                            new BlockPos(x, y, z),
                            sound,
                            SoundCategory.MASTER,
                            volume,
                            pitch);
                    return 0;
                });
        LeafMinecraftHooks.installActionbar((handle, message) -> {
            ServerPlayerEntity player = PLAYERS.get(handle);
            if (player == null) {
                return 1;
            }
            player.sendMessageToClient(Text.literal(message), true);
            return 0;
        });
        LeafMinecraftHooks.installTitle(
                (handle, title, subtitle, fadeIn, stay, fadeOut) -> {
                    ServerPlayerEntity player = PLAYERS.get(handle);
                    if (player == null) {
                        return 1;
                    }
                    player.networkHandler.sendPacket(
                            new TitleFadeS2CPacket(fadeIn, stay, fadeOut));
                    player.networkHandler.sendPacket(
                            new TitleS2CPacket(Text.literal(title != null ? title : "")));
                    player.networkHandler.sendPacket(
                            new SubtitleS2CPacket(
                                    Text.literal(subtitle != null ? subtitle : "")));
                    return 0;
                });
        LeafMinecraftHooks.installKick((handle, reason) -> {
            ServerPlayerEntity player = PLAYERS.get(handle);
            if (player == null) {
                return 1;
            }
            player.networkHandler.disconnect(
                    Text.literal(reason != null ? reason : "Kicked by LEAFMC"));
            return 0;
        });
        LeafMinecraftHooks.installGiveItem((handle, itemId, count, damage) -> {
            ServerPlayerEntity player = PLAYERS.get(handle);
            if (player == null || itemId <= 0 || count <= 0) {
                return 1;
            }
            var item = Registries.ITEM.get(itemId);
            if (item == null) {
                return 1;
            }
            ItemStack stack = new ItemStack(item, count);
            if (damage >= 0 && stack.isDamageable()) {
                stack.setDamage(damage);
            }
            if (!player.getInventory().insertStack(stack)) {
                return 1;
            }
            return 0;
        });
        LeafMinecraftHooks.installGiveItemRegistry((handle, itemId, count, damage) -> {
            ServerPlayerEntity player = PLAYERS.get(handle);
            if (player == null || itemId == null || itemId.isEmpty() || count <= 0) {
                return 1;
            }
            Identifier id = Identifier.tryParse(itemId);
            if (id == null) {
                return 1;
            }
            var item = Registries.ITEM.get(id);
            if (item == null) {
                return 1;
            }
            ItemStack stack = new ItemStack(item, count);
            if (damage >= 0 && stack.isDamageable()) {
                stack.setDamage(damage);
            }
            if (!player.getInventory().insertStack(stack)) {
                return 1;
            }
            return 0;
        });
        LeafMinecraftHooks.installEffects(
                (handle, effectId, durationTicks, amplifier, flags) -> {
                    ServerPlayerEntity player = PLAYERS.get(handle);
                    if (player == null) {
                        return 1;
                    }
                    Identifier id = Identifier.tryParse(effectId);
                    if (id == null) {
                        return 1;
                    }
                    var entry = Registries.STATUS_EFFECT.getEntry(id);
                    if (entry.isEmpty()) {
                        return 1;
                    }
                    boolean ambient = (flags & 1) != 0;
                    boolean particles = (flags & 2) != 0 || flags == 0;
                    boolean icon = (flags & 4) != 0 || flags == 0;
                    player.addStatusEffect(new StatusEffectInstance(
                            entry.get(),
                            durationTicks,
                            amplifier,
                            ambient,
                            particles,
                            icon));
                    return 0;
                },
                handle -> {
                    ServerPlayerEntity player = PLAYERS.get(handle);
                    if (player == null) {
                        return 1;
                    }
                    player.clearStatusEffects();
                    return 0;
                });
        LeafMinecraftHooks.installSpawnParticle(
                (particleId, x, y, z, dimension, count, dx, dy, dz, speed) -> {
                    MinecraftServer server = SERVER.get();
                    if (server == null) {
                        return 1;
                    }
                    Identifier id = Identifier.tryParse(particleId);
                    if (id == null) {
                        return 1;
                    }
                    var type = Registries.PARTICLE_TYPE.get(id);
                    if (!(type instanceof SimpleParticleType simple)) {
                        return 1;
                    }
                    ServerWorld world = switch (dimension) {
                        case 1 -> server.getWorld(World.NETHER);
                        case 2 -> server.getWorld(World.END);
                        default -> server.getWorld(World.OVERWORLD);
                    };
                    if (world == null) {
                        return 1;
                    }
                    world.spawnParticles(simple, x, y, z, count, dx, dy, dz, speed);
                    return 0;
                });
        LeafMinecraftHooks.installWorldTime(
                (dimension, timeOut) -> {
                    MinecraftServer server = SERVER.get();
                    if (server == null) {
                        return 1;
                    }
                    ServerWorld world = switch (dimension) {
                        case 1 -> server.getWorld(World.NETHER);
                        case 2 -> server.getWorld(World.END);
                        default -> server.getWorld(World.OVERWORLD);
                    };
                    if (world == null) {
                        return 1;
                    }
                    timeOut[0] = world.getTimeOfDay();
                    return 0;
                },
                (dimension, time) -> {
                    MinecraftServer server = SERVER.get();
                    if (server == null) {
                        return 1;
                    }
                    ServerWorld world = switch (dimension) {
                        case 1 -> server.getWorld(World.NETHER);
                        case 2 -> server.getWorld(World.END);
                        default -> server.getWorld(World.OVERWORLD);
                    };
                    if (world == null) {
                        return 1;
                    }
                    world.setTimeOfDay(time);
                    return 0;
                });
        LeafMinecraftHooks.installPlayerVelocity(
                (handle, vxOut, vyOut, vzOut) -> {
                    ServerPlayerEntity player = PLAYERS.get(handle);
                    if (player == null) {
                        return 1;
                    }
                    var v = player.getVelocity();
                    vxOut[0] = v.x;
                    vyOut[0] = v.y;
                    vzOut[0] = v.z;
                    return 0;
                },
                (handle, vx, vy, vz) -> {
                    ServerPlayerEntity player = PLAYERS.get(handle);
                    if (player == null) {
                        return 1;
                    }
                    player.setVelocity(vx, vy, vz);
                    player.velocityModified = true;
                    return 0;
                });
        LeafMinecraftHooks.installPlayerFlags((handle, flagsOut) -> {
            ServerPlayerEntity player = PLAYERS.get(handle);
            if (player == null) {
                return 1;
            }
            int flags = 0;
            if (player.isSneaking()) {
                flags |= 1;
            }
            if (player.isSprinting()) {
                flags |= 2;
            }
            if (player.isSwimming()) {
                flags |= 4;
            }
            if (player.getAbilities().flying) {
                flags |= 8;
            }
            if (player.isOnGround()) {
                flags |= 16;
            }
            if (player.getAbilities().allowFlying) {
                flags |= 32;
            }
            flagsOut[0] = flags;
            return 0;
        });
        LeafMinecraftHooks.installRunCommand((handle, command) -> {
            MinecraftServer server = SERVER.get();
            if (server == null || command == null || command.isEmpty()) {
                return 1;
            }
            String cmd = command.startsWith("/") ? command.substring(1) : command;
            try {
                if (handle == 0) {
                    server.getCommandManager().executeWithPrefix(
                            server.getCommandSource(), cmd);
                } else {
                    ServerPlayerEntity player = PLAYERS.get(handle);
                    if (player == null) {
                        return 1;
                    }
                    server.getCommandManager().executeWithPrefix(
                            player.getCommandSource(), cmd);
                }
                return 0;
            } catch (Exception e) {
                return 1;
            }
        });
        LeafMinecraftHooks.installClearInventory(handle -> {
            ServerPlayerEntity player = PLAYERS.get(handle);
            if (player == null) {
                return 1;
            }
            player.getInventory().clear();
            return 0;
        });
        LeafMinecraftHooks.installPlayerFlight((handle, allowFlight, flying) -> {
            ServerPlayerEntity player = PLAYERS.get(handle);
            if (player == null) {
                return 1;
            }
            if (allowFlight >= 0) {
                player.getAbilities().allowFlying = allowFlight != 0;
                if (allowFlight == 0) {
                    player.getAbilities().flying = false;
                }
            }
            if (flying >= 0) {
                if (flying != 0) {
                    player.getAbilities().allowFlying = true;
                    player.getAbilities().flying = true;
                } else {
                    player.getAbilities().flying = false;
                }
            }
            player.sendAbilitiesUpdate();
            return 0;
        });
        LeafMinecraftHooks.installGetBiome((dimension, x, y, z, outId) -> {
            MinecraftServer server = SERVER.get();
            if (server == null) {
                return 1;
            }
            ServerWorld world = switch (dimension) {
                case 1 -> server.getWorld(World.NETHER);
                case 2 -> server.getWorld(World.END);
                default -> server.getOverworld();
            };
            if (world == null) {
                return 1;
            }
            var entry = world.getBiome(new BlockPos(x, y, z));
            Identifier id = world.getRegistryManager()
                    .get(net.minecraft.registry.RegistryKeys.BIOME)
                    .getId(entry.value());
            outId[0] = id != null ? id.toString() : "minecraft:plains";
            return 0;
        });
        LeafMinecraftHooks.installDifficulty(
                difficultyOut -> {
                    MinecraftServer server = SERVER.get();
                    if (server == null) {
                        return 1;
                    }
                    difficultyOut[0] = server.getOverworld().getDifficulty().getId();
                    return 0;
                },
                difficulty -> {
                    MinecraftServer server = SERVER.get();
                    if (server == null || difficulty < 0 || difficulty > 3) {
                        return 1;
                    }
                    server.setDifficulty(
                            net.minecraft.world.Difficulty.byId(difficulty), true);
                    return 0;
                });
        LeafMinecraftHooks.installWeather(
                (dimension, weatherOut) -> {
                    MinecraftServer server = SERVER.get();
                    if (server == null) {
                        return 1;
                    }
                    ServerWorld world = switch (dimension) {
                        case 1 -> server.getWorld(World.NETHER);
                        case 2 -> server.getWorld(World.END);
                        default -> server.getOverworld();
                    };
                    if (world == null) {
                        return 1;
                    }
                    if (world.isThundering()) {
                        weatherOut[0] = 2;
                    } else if (world.isRaining()) {
                        weatherOut[0] = 1;
                    } else {
                        weatherOut[0] = 0;
                    }
                    return 0;
                },
                (dimension, weather, durationTicks) -> {
                    MinecraftServer server = SERVER.get();
                    if (server == null || weather < 0 || weather > 2) {
                        return 1;
                    }
                    ServerWorld world = switch (dimension) {
                        case 1 -> server.getWorld(World.NETHER);
                        case 2 -> server.getWorld(World.END);
                        default -> server.getOverworld();
                    };
                    if (world == null) {
                        return 1;
                    }
                    int duration = durationTicks > 0 ? durationTicks : 6000;
                    switch (weather) {
                        case 1 -> world.setWeather(0, duration, true, false);
                        case 2 -> world.setWeather(0, duration, true, true);
                        default -> world.setWeather(duration, 0, false, false);
                    }
                    return 0;
                });
        LeafMinecraftHooks.installGetLightLevel(
                (dimension, x, y, z, blockOut, skyOut) -> {
                    MinecraftServer server = SERVER.get();
                    if (server == null) {
                        return 1;
                    }
                    ServerWorld world = switch (dimension) {
                        case 1 -> server.getWorld(World.NETHER);
                        case 2 -> server.getWorld(World.END);
                        default -> server.getOverworld();
                    };
                    if (world == null) {
                        return 1;
                    }
                    BlockPos pos = new BlockPos(x, y, z);
                    blockOut[0] = world.getLightLevel(
                            net.minecraft.world.LightType.BLOCK, pos);
                    skyOut[0] = world.getLightLevel(
                            net.minecraft.world.LightType.SKY, pos);
                    return 0;
                });
        LeafMinecraftHooks.installPlayerLatency((handle, latencyOut) -> {
            ServerPlayerEntity player = PLAYERS.get(handle);
            if (player == null) {
                return 1;
            }
            latencyOut[0] = player.networkHandler.getLatency();
            return 0;
        });
        LeafMinecraftHooks.installWorldSpawn(
                (dimension, xOut, yOut, zOut) -> {
                    MinecraftServer server = SERVER.get();
                    if (server == null) {
                        return 1;
                    }
                    ServerWorld world = switch (dimension) {
                        case 1 -> server.getWorld(World.NETHER);
                        case 2 -> server.getWorld(World.END);
                        default -> server.getOverworld();
                    };
                    if (world == null) {
                        return 1;
                    }
                    BlockPos spawn = world.getSpawnPos();
                    xOut[0] = spawn.getX();
                    yOut[0] = spawn.getY();
                    zOut[0] = spawn.getZ();
                    return 0;
                },
                (dimension, x, y, z) -> {
                    MinecraftServer server = SERVER.get();
                    if (server == null) {
                        return 1;
                    }
                    ServerWorld world = switch (dimension) {
                        case 1 -> server.getWorld(World.NETHER);
                        case 2 -> server.getWorld(World.END);
                        default -> server.getOverworld();
                    };
                    if (world == null) {
                        return 1;
                    }
                    world.setSpawnPos(new BlockPos(x, y, z), 0.0f);
                    return 0;
                });
        LeafMinecraftHooks.installPlayerOp((handle, opOut) -> {
            ServerPlayerEntity player = PLAYERS.get(handle);
            MinecraftServer server = SERVER.get();
            if (player == null || server == null) {
                return 1;
            }
            opOut[0] = server.getPlayerManager().isOperator(player.getGameProfile())
                    ? 1
                    : 0;
            return 0;
        });
        LeafMinecraftHooks.installPlayerUuid((handle, outUuid) -> {
            java.util.UUID uuid = LeafPlayerHandles.uuidOf(handle);
            if (uuid == null) {
                return 1;
            }
            outUuid[0] = uuid.toString();
            return 0;
        });
        LeafMinecraftHooks.installPlayerPermission((handle, levelOut) -> {
            ServerPlayerEntity player = PLAYERS.get(handle);
            MinecraftServer server = SERVER.get();
            if (player == null || server == null) {
                return 1;
            }
            levelOut[0] = server.getPermissionLevel(player.getGameProfile());
            return 0;
        });
        LeafMinecraftHooks.installFindPlayerUuid((uuid, handleOut) -> {
            try {
                Long handle = LeafPlayerHandles.handleOf(java.util.UUID.fromString(uuid));
                if (handle == null) {
                    return 1;
                }
                handleOut[0] = handle;
                return 0;
            } catch (IllegalArgumentException e) {
                return 1;
            }
        });
        LeafMinecraftHooks.installFindPlayerName((name, handleOut) -> {
            if (name == null || name.isEmpty()) {
                return 1;
            }
            for (var entry : PLAYERS.entrySet()) {
                ServerPlayerEntity player = entry.getValue();
                if (player != null && name.equalsIgnoreCase(player.getName().getString())) {
                    handleOut[0] = entry.getKey();
                    return 0;
                }
            }
            return 1;
        });
        LeafMinecraftHooks.installBlockRegistry(
                (dimension, x, y, z, outId) -> {
                    MinecraftServer server = SERVER.get();
                    if (server == null) {
                        return 1;
                    }
                    ServerWorld world = switch (dimension) {
                        case 1 -> server.getWorld(World.NETHER);
                        case 2 -> server.getWorld(World.END);
                        default -> server.getOverworld();
                    };
                    if (world == null) {
                        return 1;
                    }
                    var state = world.getBlockState(new BlockPos(x, y, z));
                    Identifier id = Registries.BLOCK.getId(state.getBlock());
                    outId[0] = id != null ? id.toString() : "minecraft:air";
                    return 0;
                },
                (dimension, x, y, z, blockId) -> {
                    MinecraftServer server = SERVER.get();
                    if (server == null || blockId == null || blockId.isEmpty()) {
                        return 1;
                    }
                    ServerWorld world = switch (dimension) {
                        case 1 -> server.getWorld(World.NETHER);
                        case 2 -> server.getWorld(World.END);
                        default -> server.getOverworld();
                    };
                    if (world == null) {
                        return 1;
                    }
                    Identifier id = Identifier.tryParse(blockId);
                    if (id == null) {
                        return 1;
                    }
                    var block = Registries.BLOCK.get(id);
                    if (block == null) {
                        return 1;
                    }
                    world.setBlockState(new BlockPos(x, y, z), block.getDefaultState());
                    return 0;
                });

        ServerLifecycleEvents.SERVER_STARTING.register(server -> {
            SERVER.set(server);
            onServerStarting();
        });
        ServerLifecycleEvents.SERVER_STARTED.register(server -> {
            SERVER.set(server);
            onServerStarted();
        });
        ServerLifecycleEvents.SERVER_STOPPING.register(server -> {
            onServerStopping();
            SERVER.set(null);
        });
        ServerTickEvents.END_SERVER_TICK.register(server -> onEndServerTick());

        ServerWorldEvents.LOAD.register((server, world) -> {
            int dimension;
            var key = world.getRegistryKey();
            if (key == World.NETHER) {
                dimension = 1;
            } else if (key == World.END) {
                dimension = 2;
            } else {
                dimension = 0;
            }
            if (LeafBridge.isInitialized()) {
                LeafBridge.emitWorldLoad(dimension);
            }
        });

        ServerPlayConnectionEvents.JOIN.register((handler, sender, server) -> {
            ServerPlayerEntity player = handler.player;
            long handle = LeafPlayerHandles.allocate(player.getUuid());
            PLAYERS.put(handle, player);
            String name = player.getName().getString();
            LeafBridge.requireOk(
                    "fabric register player",
                    LeafBridge.registerPlayer(handle, name));
            LeafMinecraftHooks.putPlayerSender(handle, (h, msg) -> {
                ServerPlayerEntity p = PLAYERS.get(h);
                if (p != null) {
                    p.sendMessage(Text.literal(msg));
                }
            });
            onPlayerJoin(handle);
        });
        ServerPlayConnectionEvents.DISCONNECT.register((handler, server) -> {
            ServerPlayerEntity player = handler.player;
            Long existing = LeafPlayerHandles.handleOf(player.getUuid());
            long handle = existing != null
                    ? existing
                    : LeafPlayerHandles.allocate(player.getUuid());
            onPlayerLeave(handle);
            LeafBridge.unregisterPlayer(handle);
            LeafMinecraftHooks.removePlayer(handle);
            PLAYERS.remove(handle);
            LeafPlayerHandles.release(player.getUuid());
        });

        ServerMessageEvents.ALLOW_CHAT_MESSAGE.register((message, sender, params) -> {
            Long handle = LeafPlayerHandles.handleOf(sender.getUuid());
            if (handle == null) {
                return true;
            }
            String text = message.getContent().getString();
            int[] decision = new int[1];
            int code = LeafBridge.emitPlayerChat(handle, text, decision);
            if (code != 0) {
                return true;
            }
            return decision[0] != LeafBridge.DECISION_DENY;
        });

        ServerLivingEntityEvents.AFTER_DEATH.register((entity, damageSource) -> {
            if (!(entity instanceof ServerPlayerEntity player)) {
                return;
            }
            Long handle = LeafPlayerHandles.handleOf(player.getUuid());
            if (handle != null) {
                LeafBridge.emitPlayerDeath(handle);
            }
        });

        ServerEntityEvents.ENTITY_LOAD.register((entity, world) -> {
            if (entity instanceof ServerPlayerEntity) {
                return;
            }
            emitEntityLifecycle(true, entity, world);
        });
        ServerEntityEvents.ENTITY_UNLOAD.register((entity, world) -> {
            if (entity instanceof ServerPlayerEntity) {
                return;
            }
            emitEntityLifecycle(false, entity, world);
        });

        PlayerBlockBreakEvents.BEFORE.register((world, player, pos, state, blockEntity) -> {
            if (!(player instanceof ServerPlayerEntity serverPlayer)) {
                return true;
            }
            Long handle = LeafPlayerHandles.handleOf(serverPlayer.getUuid());
            long h = handle != null ? handle : 0L;
            int blockId = Registries.BLOCK.getRawId(state.getBlock());
            int[] decision = new int[1];
            int code = LeafBridge.emitBlockBreak(
                    h, pos.getX(), pos.getY(), pos.getZ(), blockId, decision);
            if (code != 0) {
                return true;
            }
            return decision[0] != LeafBridge.DECISION_DENY;
        });

        UseBlockCallback.EVENT.register((player, world, hand, hitResult) -> {
            if (world.isClient() || hand != Hand.MAIN_HAND) {
                return ActionResult.PASS;
            }
            if (!(player instanceof ServerPlayerEntity serverPlayer)) {
                return ActionResult.PASS;
            }
            ItemStack stack = serverPlayer.getStackInHand(hand);
            if (!(stack.getItem() instanceof BlockItem blockItem)) {
                return ActionResult.PASS;
            }
            Long handle = LeafPlayerHandles.handleOf(serverPlayer.getUuid());
            long h = handle != null ? handle : 0L;
            BlockPos placePos = hitResult.getBlockPos().offset(hitResult.getSide());
            int blockId = Registries.BLOCK.getRawId(blockItem.getBlock());
            int[] decision = new int[1];
            int code = LeafBridge.emitBlockPlace(
                    h, placePos.getX(), placePos.getY(), placePos.getZ(), blockId, decision);
            if (code == 0 && decision[0] == LeafBridge.DECISION_DENY) {
                return ActionResult.FAIL;
            }
            return ActionResult.PASS;
        });
    }

    private static void onServerStarting() {
        LeafBridge.requireOk("fabric starting", LeafBridge.onServerStarting());
    }

    private static void onServerStarted() {
        LeafBridge.requireOk("fabric started", LeafBridge.onServerStarted());
    }

    private static void onServerStopping() {
        LeafBridge.requireOk("fabric stopping", LeafBridge.onServerStopping());
        LeafBridge.requireOk("fabric shutdown", LeafBridge.shutdown());
    }

    private static void onEndServerTick() {
        if (LeafBridge.isInitialized()) {
            LeafBridge.requireOk("fabric pump", LeafBridge.pumpMain(64));
        }
    }

    private static void onPlayerJoin(long playerHandle) {
        if (LeafBridge.isInitialized()) {
            LeafBridge.requireOk("fabric join", LeafBridge.emitPlayerJoin(playerHandle));
        }
    }

    private static void onPlayerLeave(long playerHandle) {
        if (LeafBridge.isInitialized()) {
            LeafBridge.requireOk("fabric leave", LeafBridge.emitPlayerLeave(playerHandle));
        }
    }

    private static void emitEntityLifecycle(boolean spawn, Entity entity, ServerWorld world) {
        if (!LeafBridge.isInitialized() || entity == null || world == null) {
            return;
        }
        long handle = Integer.toUnsignedLong(entity.getId());
        int typeId = Registries.ENTITY_TYPE.getRawId(entity.getType());
        int x = entity.getBlockX();
        int y = entity.getBlockY();
        int z = entity.getBlockZ();
        int dimension;
        var key = world.getRegistryKey();
        if (key == World.NETHER) {
            dimension = 1;
        } else if (key == World.END) {
            dimension = 2;
        } else {
            dimension = 0;
        }
        if (spawn) {
            LeafBridge.emitEntitySpawn(handle, typeId, x, y, z, dimension);
        } else {
            LeafBridge.emitEntityRemove(handle, typeId, x, y, z, dimension);
        }
    }
}
