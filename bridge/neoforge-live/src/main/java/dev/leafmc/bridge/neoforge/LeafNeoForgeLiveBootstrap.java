package dev.leafmc.bridge.neoforge;

import dev.leafmc.bridge.LeafBridge;
import dev.leafmc.bridge.LeafMinecraftHooks;
import dev.leafmc.bridge.LeafPlayerHandles;
import dev.leafmc.bridge.LeafRuntime;
import net.minecraft.core.BlockPos;
import net.minecraft.core.registries.BuiltInRegistries;
import net.minecraft.network.chat.Component;
import net.minecraft.network.protocol.game.ClientboundSetSubtitleTextPacket;
import net.minecraft.network.protocol.game.ClientboundSetTitleTextPacket;
import net.minecraft.network.protocol.game.ClientboundSetTitlesAnimationPacket;
import net.minecraft.resources.ResourceLocation;
import net.minecraft.server.MinecraftServer;
import net.minecraft.server.level.ServerLevel;
import net.minecraft.server.level.ServerPlayer;
import net.minecraft.sounds.SoundEvent;
import net.minecraft.sounds.SoundSource;
import net.minecraft.core.particles.SimpleParticleType;
import net.minecraft.world.effect.MobEffectInstance;
import net.minecraft.world.item.ItemStack;
import net.minecraft.world.level.GameType;
import net.minecraft.world.level.Level;
import net.neoforged.bus.api.IEventBus;
import net.neoforged.fml.common.Mod;
import net.neoforged.fml.event.lifecycle.FMLCommonSetupEvent;
import net.neoforged.neoforge.common.NeoForge;
import net.neoforged.neoforge.event.ServerChatEvent;
import net.neoforged.neoforge.event.entity.EntityJoinLevelEvent;
import net.neoforged.neoforge.event.entity.EntityLeaveLevelEvent;
import net.neoforged.neoforge.event.entity.living.LivingDeathEvent;
import net.neoforged.neoforge.event.entity.player.PlayerEvent;
import net.neoforged.neoforge.event.level.BlockEvent;
import net.neoforged.neoforge.event.level.LevelEvent;
import net.neoforged.neoforge.event.server.ServerStartedEvent;
import net.neoforged.neoforge.event.server.ServerStartingEvent;
import net.neoforged.neoforge.event.server.ServerStoppingEvent;
import net.neoforged.neoforge.event.tick.ServerTickEvent;

import net.minecraft.world.entity.Entity;

import java.util.Map;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicReference;

@Mod("leaf_bootstrap_neoforge")
public final class LeafNeoForgeLiveBootstrap {
    private static final Map<Long, ServerPlayer> PLAYERS = new ConcurrentHashMap<>();
    private static final AtomicReference<MinecraftServer> SERVER = new AtomicReference<>();

    public LeafNeoForgeLiveBootstrap(IEventBus modBus) {
        modBus.addListener(this::onCommonSetup);
        NeoForge.EVENT_BUS.addListener(this::onServerStarting);
        NeoForge.EVENT_BUS.addListener(this::onServerStarted);
        NeoForge.EVENT_BUS.addListener(this::onServerStopping);
        NeoForge.EVENT_BUS.addListener(this::onServerTick);
        NeoForge.EVENT_BUS.addListener(this::onLogin);
        NeoForge.EVENT_BUS.addListener(this::onLogout);
        NeoForge.EVENT_BUS.addListener(this::onChat);
        NeoForge.EVENT_BUS.addListener(this::onDeath);
        NeoForge.EVENT_BUS.addListener(this::onEntityJoin);
        NeoForge.EVENT_BUS.addListener(this::onEntityLeave);
        NeoForge.EVENT_BUS.addListener(this::onWorldLoad);
        NeoForge.EVENT_BUS.addListener(this::onBlockBreak);
        NeoForge.EVENT_BUS.addListener(this::onBlockPlace);
    }

    private void onCommonSetup(FMLCommonSetupEvent event) {
        LeafRuntime.start(LeafBridge.LOADER_NEOFORGE, LeafBridge.MAPPING_MOJMAP);
        LeafMinecraftHooks.installDefaultSend((handle, message) -> {
            ServerPlayer player = PLAYERS.get(handle);
            if (player != null) {
                player.sendSystemMessage(Component.literal(message));
            }
        });
        LeafMinecraftHooks.installBroadcast(message -> {
            MinecraftServer server = SERVER.get();
            if (server != null) {
                server.getPlayerList().broadcastSystemMessage(
                        Component.literal(message), false);
            }
        });
        LeafMinecraftHooks.installInventory(
                (handle, slot, itemIdOut, countOut) -> {
                    ServerPlayer player = PLAYERS.get(handle);
                    if (player == null || slot < 0 || slot > 40) {
                        return 1;
                    }
                    ItemStack stack = player.getInventory().getItem(slot);
                    itemIdOut[0] = stack.isEmpty()
                            ? 0
                            : BuiltInRegistries.ITEM.getId(stack.getItem());
                    countOut[0] = stack.isEmpty() ? 0 : stack.getCount();
                    return 0;
                },
                (handle, slot, itemId, count) -> {
                    ServerPlayer player = PLAYERS.get(handle);
                    if (player == null || slot < 0 || slot > 40) {
                        return 1;
                    }
                    if (itemId == 0 || count <= 0) {
                        player.getInventory().setItem(slot, ItemStack.EMPTY);
                        return 0;
                    }
                    var item = BuiltInRegistries.ITEM.byId(itemId);
                    if (item == null) {
                        return 1;
                    }
                    player.getInventory().setItem(slot, new ItemStack(item, count));
                    return 0;
                });
        LeafMinecraftHooks.installInventoryStack(
                (handle, slot, itemIdOut, countOut, damageOut) -> {
                    ServerPlayer player = PLAYERS.get(handle);
                    if (player == null || slot < 0 || slot > 40) {
                        return 1;
                    }
                    ItemStack stack = player.getInventory().getItem(slot);
                    if (stack.isEmpty()) {
                        itemIdOut[0] = 0;
                        countOut[0] = 0;
                        damageOut[0] = -1;
                        return 0;
                    }
                    itemIdOut[0] = BuiltInRegistries.ITEM.getId(stack.getItem());
                    countOut[0] = stack.getCount();
                    damageOut[0] = stack.isDamageableItem() ? stack.getDamageValue() : -1;
                    return 0;
                },
                (handle, slot, itemId, count, damage) -> {
                    ServerPlayer player = PLAYERS.get(handle);
                    if (player == null || slot < 0 || slot > 40) {
                        return 1;
                    }
                    if (itemId == 0 || count <= 0) {
                        player.getInventory().setItem(slot, ItemStack.EMPTY);
                        return 0;
                    }
                    var item = BuiltInRegistries.ITEM.byId(itemId);
                    if (item == null) {
                        return 1;
                    }
                    ItemStack stack = new ItemStack(item, count);
                    if (damage >= 0 && stack.isDamageableItem()) {
                        stack.setDamageValue(damage);
                    }
                    player.getInventory().setItem(slot, stack);
                    return 0;
                });
        LeafMinecraftHooks.installInventoryRegistry(
                (handle, slot, outId, countOut, damageOut) -> {
                    ServerPlayer player = PLAYERS.get(handle);
                    if (player == null || slot < 0 || slot > 40) {
                        return 1;
                    }
                    ItemStack stack = player.getInventory().getItem(slot);
                    if (stack.isEmpty()) {
                        outId[0] = "minecraft:air";
                        countOut[0] = 0;
                        damageOut[0] = -1;
                        return 0;
                    }
                    ResourceLocation id = BuiltInRegistries.ITEM.getKey(stack.getItem());
                    outId[0] = id != null ? id.toString() : "minecraft:air";
                    countOut[0] = stack.getCount();
                    damageOut[0] = stack.isDamageableItem() ? stack.getDamageValue() : -1;
                    return 0;
                },
                (handle, slot, itemId, count, damage) -> {
                    ServerPlayer player = PLAYERS.get(handle);
                    if (player == null || slot < 0 || slot > 40) {
                        return 1;
                    }
                    if (itemId == null || itemId.isEmpty() || itemId.equals("minecraft:air")
                            || itemId.equals("air") || count <= 0) {
                        player.getInventory().setItem(slot, ItemStack.EMPTY);
                        return 0;
                    }
                    ResourceLocation id = ResourceLocation.tryParse(itemId);
                    if (id == null) {
                        return 1;
                    }
                    var item = BuiltInRegistries.ITEM.get(id);
                    if (item == null) {
                        return 1;
                    }
                    ItemStack stack = new ItemStack(item, count);
                    if (damage >= 0 && stack.isDamageableItem()) {
                        stack.setDamageValue(damage);
                    }
                    player.getInventory().setItem(slot, stack);
                    return 0;
                });
        LeafMinecraftHooks.installWorld(
                (dimension, x, y, z, blockIdOut) -> {
                    MinecraftServer server = SERVER.get();
                    if (server == null) {
                        return 1;
                    }
                    var level = switch (dimension) {
                        case 1 -> server.getLevel(net.minecraft.world.level.Level.NETHER);
                        case 2 -> server.getLevel(net.minecraft.world.level.Level.END);
                        default -> server.overworld();
                    };
                    if (level == null) {
                        return 1;
                    }
                    var state = level.getBlockState(new BlockPos(x, y, z));
                    blockIdOut[0] = BuiltInRegistries.BLOCK.getId(state.getBlock());
                    return 0;
                },
                (dimension, x, y, z, blockId) -> {
                    MinecraftServer server = SERVER.get();
                    if (server == null) {
                        return 1;
                    }
                    var level = switch (dimension) {
                        case 1 -> server.getLevel(net.minecraft.world.level.Level.NETHER);
                        case 2 -> server.getLevel(net.minecraft.world.level.Level.END);
                        default -> server.overworld();
                    };
                    if (level == null) {
                        return 1;
                    }
                    var block = BuiltInRegistries.BLOCK.byId(blockId);
                    if (block == null) {
                        return 1;
                    }
                    level.setBlockAndUpdate(new BlockPos(x, y, z), block.defaultBlockState());
                    return 0;
                });
        LeafMinecraftHooks.installPlayerPos(
                (handle, xOut, yOut, zOut, dimOut) -> {
                    ServerPlayer player = PLAYERS.get(handle);
                    if (player == null) {
                        return 1;
                    }
                    xOut[0] = player.getBlockX();
                    yOut[0] = player.getBlockY();
                    zOut[0] = player.getBlockZ();
                    var key = player.level().dimension();
                    if (key == Level.NETHER) {
                        dimOut[0] = 1;
                    } else if (key == Level.END) {
                        dimOut[0] = 2;
                    } else {
                        dimOut[0] = 0;
                    }
                    return 0;
                },
                (handle, x, y, z, dimension) -> {
                    ServerPlayer player = PLAYERS.get(handle);
                    MinecraftServer server = SERVER.get();
                    if (player == null || server == null) {
                        return 1;
                    }
                    ServerLevel level = switch (dimension) {
                        case 1 -> server.getLevel(Level.NETHER);
                        case 2 -> server.getLevel(Level.END);
                        default -> server.overworld();
                    };
                    if (level == null) {
                        return 1;
                    }
                    player.teleportTo(
                            level,
                            x + 0.5,
                            y,
                            z + 0.5,
                            Set.of(),
                            player.getYRot(),
                            player.getXRot());
                    return 0;
                });
        LeafMinecraftHooks.installTeleport((handle, x, y, z, dimension, yaw, pitch) -> {
            ServerPlayer player = PLAYERS.get(handle);
            MinecraftServer server = SERVER.get();
            if (player == null || server == null) {
                return 1;
            }
            ServerLevel level = switch (dimension) {
                case 1 -> server.getLevel(Level.NETHER);
                case 2 -> server.getLevel(Level.END);
                default -> server.overworld();
            };
            if (level == null) {
                return 1;
            }
            player.teleportTo(level, x + 0.5, y, z + 0.5, Set.of(), yaw, pitch);
            return 0;
        });
        LeafMinecraftHooks.installSelectedSlot(
                (handle, slotOut) -> {
                    ServerPlayer player = PLAYERS.get(handle);
                    if (player == null) {
                        return 1;
                    }
                    slotOut[0] = player.getInventory().selected;
                    return 0;
                },
                (handle, slot) -> {
                    ServerPlayer player = PLAYERS.get(handle);
                    if (player == null || slot < 0 || slot > 8) {
                        return 1;
                    }
                    player.getInventory().selected = slot;
                    return 0;
                });
        LeafMinecraftHooks.installGetWorldSeed((dimension, seedOut) -> {
            MinecraftServer server = SERVER.get();
            if (server == null) {
                return 1;
            }
            ServerLevel level = switch (dimension) {
                case 1 -> server.getLevel(Level.NETHER);
                case 2 -> server.getLevel(Level.END);
                default -> server.getLevel(Level.OVERWORLD);
            };
            if (level == null) {
                return 1;
            }
            seedOut[0] = level.getSeed();
            return 0;
        });
        LeafMinecraftHooks.installPlayerHealth(
                (handle, healthOut, maxOut) -> {
                    ServerPlayer player = PLAYERS.get(handle);
                    if (player == null) {
                        return 1;
                    }
                    healthOut[0] = player.getHealth();
                    maxOut[0] = player.getMaxHealth();
                    return 0;
                },
                (handle, health) -> {
                    ServerPlayer player = PLAYERS.get(handle);
                    if (player == null) {
                        return 1;
                    }
                    player.setHealth(health);
                    return 0;
                });
        LeafMinecraftHooks.installPlayerAbsorption(
                (handle, absOut) -> {
                    ServerPlayer player = PLAYERS.get(handle);
                    if (player == null) {
                        return 1;
                    }
                    absOut[0] = player.getAbsorptionAmount();
                    return 0;
                },
                (handle, absorption) -> {
                    ServerPlayer player = PLAYERS.get(handle);
                    if (player == null || absorption < 0.0f) {
                        return 1;
                    }
                    player.setAbsorptionAmount(absorption);
                    return 0;
                });
        LeafMinecraftHooks.installPlayerInvulnerable(
                (handle, out) -> {
                    ServerPlayer player = PLAYERS.get(handle);
                    if (player == null) {
                        return 1;
                    }
                    out[0] = player.isInvulnerable() ? 1 : 0;
                    return 0;
                },
                (handle, invulnerable) -> {
                    ServerPlayer player = PLAYERS.get(handle);
                    if (player == null) {
                        return 1;
                    }
                    player.setInvulnerable(invulnerable != 0);
                    return 0;
                });
        LeafMinecraftHooks.installPlayerAir(
                (handle, out) -> {
                    ServerPlayer player = PLAYERS.get(handle);
                    if (player == null) {
                        return 1;
                    }
                    out[0] = player.getAirSupply();
                    return 0;
                },
                (handle, air) -> {
                    ServerPlayer player = PLAYERS.get(handle);
                    if (player == null) {
                        return 1;
                    }
                    player.setAirSupply(air);
                    return 0;
                });
        LeafMinecraftHooks.installPlayerFireTicks(
                (handle, out) -> {
                    ServerPlayer player = PLAYERS.get(handle);
                    if (player == null) {
                        return 1;
                    }
                    out[0] = player.getRemainingFireTicks();
                    return 0;
                },
                (handle, ticks) -> {
                    ServerPlayer player = PLAYERS.get(handle);
                    if (player == null) {
                        return 1;
                    }
                    player.setRemainingFireTicks(ticks);
                    return 0;
                });
        LeafMinecraftHooks.installPlayerFrozenTicks(
                (handle, out) -> {
                    ServerPlayer player = PLAYERS.get(handle);
                    if (player == null) {
                        return 1;
                    }
                    out[0] = player.getTicksFrozen();
                    return 0;
                },
                (handle, ticks) -> {
                    ServerPlayer player = PLAYERS.get(handle);
                    if (player == null) {
                        return 1;
                    }
                    player.setTicksFrozen(ticks);
                    return 0;
                });
        LeafMinecraftHooks.installPlayerNoGravity(
                (handle, out) -> {
                    ServerPlayer player = PLAYERS.get(handle);
                    if (player == null) {
                        return 1;
                    }
                    out[0] = player.isNoGravity() ? 1 : 0;
                    return 0;
                },
                (handle, noGravity) -> {
                    ServerPlayer player = PLAYERS.get(handle);
                    if (player == null) {
                        return 1;
                    }
                    player.setNoGravity(noGravity != 0);
                    return 0;
                });
        LeafMinecraftHooks.installPlayerSilent(
                (handle, out) -> {
                    ServerPlayer player = PLAYERS.get(handle);
                    if (player == null) {
                        return 1;
                    }
                    out[0] = player.isSilent() ? 1 : 0;
                    return 0;
                },
                (handle, silent) -> {
                    ServerPlayer player = PLAYERS.get(handle);
                    if (player == null) {
                        return 1;
                    }
                    player.setSilent(silent != 0);
                    return 0;
                });
        LeafMinecraftHooks.installPlayerGlowing(
                (handle, out) -> {
                    ServerPlayer player = PLAYERS.get(handle);
                    if (player == null) {
                        return 1;
                    }
                    out[0] = player.isCurrentlyGlowing() ? 1 : 0;
                    return 0;
                },
                (handle, glowing) -> {
                    ServerPlayer player = PLAYERS.get(handle);
                    if (player == null) {
                        return 1;
                    }
                    player.setGlowingTag(glowing != 0);
                    return 0;
                });
        LeafMinecraftHooks.installPlayerInvisible(
                (handle, out) -> {
                    ServerPlayer player = PLAYERS.get(handle);
                    if (player == null) {
                        return 1;
                    }
                    out[0] = player.isInvisible() ? 1 : 0;
                    return 0;
                },
                (handle, invisible) -> {
                    ServerPlayer player = PLAYERS.get(handle);
                    if (player == null) {
                        return 1;
                    }
                    player.setInvisible(invisible != 0);
                    return 0;
                });
        LeafMinecraftHooks.installPlayerPortalCooldown(
                (handle, out) -> {
                    ServerPlayer player = PLAYERS.get(handle);
                    if (player == null) {
                        return 1;
                    }
                    out[0] = player.getPortalCooldown();
                    return 0;
                },
                (handle, ticks) -> {
                    ServerPlayer player = PLAYERS.get(handle);
                    if (player == null) {
                        return 1;
                    }
                    player.setPortalCooldown(ticks);
                    return 0;
                });
        LeafMinecraftHooks.installGetPlayerMaxAir((handle, out) -> {
            ServerPlayer player = PLAYERS.get(handle);
            if (player == null) {
                return 1;
            }
            out[0] = player.getMaxAirSupply();
            return 0;
        });
        LeafMinecraftHooks.installPlayerFood(
                (handle, foodOut, satOut) -> {
                    ServerPlayer player = PLAYERS.get(handle);
                    if (player == null) {
                        return 1;
                    }
                    var food = player.getFoodData();
                    foodOut[0] = food.getFoodLevel();
                    satOut[0] = food.getSaturationLevel();
                    return 0;
                },
                (handle, foodLevel, saturation) -> {
                    ServerPlayer player = PLAYERS.get(handle);
                    if (player == null) {
                        return 1;
                    }
                    var food = player.getFoodData();
                    food.setFoodLevel(foodLevel);
                    food.setSaturation(saturation);
                    return 0;
                });
        LeafMinecraftHooks.installPlayerGamemode(
                (handle, modeOut) -> {
                    ServerPlayer player = PLAYERS.get(handle);
                    if (player == null) {
                        return 1;
                    }
                    modeOut[0] = player.gameMode.getGameModeForPlayer().getId();
                    return 0;
                },
                (handle, mode) -> {
                    ServerPlayer player = PLAYERS.get(handle);
                    if (player == null || mode < 0 || mode > 3) {
                        return 1;
                    }
                    player.setGameMode(GameType.byId(mode));
                    return 0;
                });
        LeafMinecraftHooks.installPlayerXp(
                (handle, levelOut, progressOut) -> {
                    ServerPlayer player = PLAYERS.get(handle);
                    if (player == null) {
                        return 1;
                    }
                    levelOut[0] = player.experienceLevel;
                    progressOut[0] = player.experienceProgress;
                    return 0;
                },
                (handle, level) -> {
                    ServerPlayer player = PLAYERS.get(handle);
                    if (player == null || level < 0) {
                        return 1;
                    }
                    player.experienceLevel = level;
                    player.experienceProgress = 0.0f;
                    return 0;
                });
        LeafMinecraftHooks.installPlayerLook(
                (handle, yawOut, pitchOut) -> {
                    ServerPlayer player = PLAYERS.get(handle);
                    if (player == null) {
                        return 1;
                    }
                    yawOut[0] = player.getYRot();
                    pitchOut[0] = player.getXRot();
                    return 0;
                },
                (handle, yaw, pitch) -> {
                    ServerPlayer player = PLAYERS.get(handle);
                    if (player == null) {
                        return 1;
                    }
                    player.setYRot(yaw);
                    player.setXRot(pitch);
                    return 0;
                });
        LeafMinecraftHooks.installPlaySound(
                (handle, soundId, volume, pitch, x, y, z, dimension) -> {
                    ResourceLocation id = ResourceLocation.tryParse(soundId);
                    if (id == null) {
                        return 1;
                    }
                    var opt = BuiltInRegistries.SOUND_EVENT.getOptional(id);
                    if (opt.isEmpty()) {
                        return 1;
                    }
                    SoundEvent sound = opt.get();
                    if (handle != 0) {
                        ServerPlayer player = PLAYERS.get(handle);
                        if (player == null) {
                            return 1;
                        }
                        player.playNotifySound(sound, SoundSource.MASTER, volume, pitch);
                        return 0;
                    }
                    MinecraftServer server = SERVER.get();
                    if (server == null) {
                        return 1;
                    }
                    ServerLevel level = switch (dimension) {
                        case 1 -> server.getLevel(Level.NETHER);
                        case 2 -> server.getLevel(Level.END);
                        default -> server.getLevel(Level.OVERWORLD);
                    };
                    if (level == null) {
                        return 1;
                    }
                    level.playSound(
                            null,
                            new BlockPos(x, y, z),
                            sound,
                            SoundSource.MASTER,
                            volume,
                            pitch);
                    return 0;
                });
        LeafMinecraftHooks.installActionbar((handle, message) -> {
            ServerPlayer player = PLAYERS.get(handle);
            if (player == null) {
                return 1;
            }
            player.displayClientMessage(Component.literal(message), true);
            return 0;
        });
        LeafMinecraftHooks.installTitle(
                (handle, title, subtitle, fadeIn, stay, fadeOut) -> {
                    ServerPlayer player = PLAYERS.get(handle);
                    if (player == null) {
                        return 1;
                    }
                    player.connection.send(
                            new ClientboundSetTitlesAnimationPacket(fadeIn, stay, fadeOut));
                    player.connection.send(
                            new ClientboundSetTitleTextPacket(
                                    Component.literal(title != null ? title : "")));
                    player.connection.send(
                            new ClientboundSetSubtitleTextPacket(
                                    Component.literal(subtitle != null ? subtitle : "")));
                    return 0;
                });
        LeafMinecraftHooks.installKick((handle, reason) -> {
            ServerPlayer player = PLAYERS.get(handle);
            if (player == null) {
                return 1;
            }
            player.connection.disconnect(
                    Component.literal(reason != null ? reason : "Kicked by LEAFMC"));
            return 0;
        });
        LeafMinecraftHooks.installGiveItem((handle, itemId, count, damage) -> {
            ServerPlayer player = PLAYERS.get(handle);
            if (player == null || itemId <= 0 || count <= 0) {
                return 1;
            }
            var item = BuiltInRegistries.ITEM.byId(itemId);
            if (item == null) {
                return 1;
            }
            ItemStack stack = new ItemStack(item, count);
            if (damage >= 0 && stack.isDamageableItem()) {
                stack.setDamageValue(damage);
            }
            if (!player.getInventory().add(stack)) {
                return 1;
            }
            return 0;
        });
        LeafMinecraftHooks.installGiveItemRegistry((handle, itemId, count, damage) -> {
            ServerPlayer player = PLAYERS.get(handle);
            if (player == null || itemId == null || itemId.isEmpty() || count <= 0) {
                return 1;
            }
            ResourceLocation id = ResourceLocation.tryParse(itemId);
            if (id == null) {
                return 1;
            }
            var item = BuiltInRegistries.ITEM.get(id);
            if (item == null) {
                return 1;
            }
            ItemStack stack = new ItemStack(item, count);
            if (damage >= 0 && stack.isDamageableItem()) {
                stack.setDamageValue(damage);
            }
            if (!player.getInventory().add(stack)) {
                return 1;
            }
            return 0;
        });
        LeafMinecraftHooks.installEffects(
                (handle, effectId, durationTicks, amplifier, flags) -> {
                    ServerPlayer player = PLAYERS.get(handle);
                    if (player == null) {
                        return 1;
                    }
                    ResourceLocation id = ResourceLocation.tryParse(effectId);
                    if (id == null) {
                        return 1;
                    }
                    var holder = BuiltInRegistries.MOB_EFFECT.getHolder(id);
                    if (holder.isEmpty()) {
                        return 1;
                    }
                    boolean ambient = (flags & 1) != 0;
                    boolean particles = (flags & 2) != 0 || flags == 0;
                    boolean icon = (flags & 4) != 0 || flags == 0;
                    player.addEffect(new MobEffectInstance(
                            holder.get(),
                            durationTicks,
                            amplifier,
                            ambient,
                            particles,
                            icon));
                    return 0;
                },
                handle -> {
                    ServerPlayer player = PLAYERS.get(handle);
                    if (player == null) {
                        return 1;
                    }
                    player.removeAllEffects();
                    return 0;
                });
        LeafMinecraftHooks.installSpawnParticle(
                (particleId, x, y, z, dimension, count, dx, dy, dz, speed) -> {
                    MinecraftServer server = SERVER.get();
                    if (server == null) {
                        return 1;
                    }
                    ResourceLocation id = ResourceLocation.tryParse(particleId);
                    if (id == null) {
                        return 1;
                    }
                    var type = BuiltInRegistries.PARTICLE_TYPE.get(id);
                    if (!(type instanceof SimpleParticleType simple)) {
                        return 1;
                    }
                    ServerLevel level = switch (dimension) {
                        case 1 -> server.getLevel(Level.NETHER);
                        case 2 -> server.getLevel(Level.END);
                        default -> server.getLevel(Level.OVERWORLD);
                    };
                    if (level == null) {
                        return 1;
                    }
                    level.sendParticles(simple, x, y, z, count, dx, dy, dz, speed);
                    return 0;
                });
        LeafMinecraftHooks.installWorldTime(
                (dimension, timeOut) -> {
                    MinecraftServer server = SERVER.get();
                    if (server == null) {
                        return 1;
                    }
                    ServerLevel level = switch (dimension) {
                        case 1 -> server.getLevel(Level.NETHER);
                        case 2 -> server.getLevel(Level.END);
                        default -> server.getLevel(Level.OVERWORLD);
                    };
                    if (level == null) {
                        return 1;
                    }
                    timeOut[0] = level.getDayTime();
                    return 0;
                },
                (dimension, time) -> {
                    MinecraftServer server = SERVER.get();
                    if (server == null) {
                        return 1;
                    }
                    ServerLevel level = switch (dimension) {
                        case 1 -> server.getLevel(Level.NETHER);
                        case 2 -> server.getLevel(Level.END);
                        default -> server.getLevel(Level.OVERWORLD);
                    };
                    if (level == null) {
                        return 1;
                    }
                    level.setDayTime(time);
                    return 0;
                });
        LeafMinecraftHooks.installPlayerVelocity(
                (handle, vxOut, vyOut, vzOut) -> {
                    ServerPlayer player = PLAYERS.get(handle);
                    if (player == null) {
                        return 1;
                    }
                    var v = player.getDeltaMovement();
                    vxOut[0] = v.x;
                    vyOut[0] = v.y;
                    vzOut[0] = v.z;
                    return 0;
                },
                (handle, vx, vy, vz) -> {
                    ServerPlayer player = PLAYERS.get(handle);
                    if (player == null) {
                        return 1;
                    }
                    player.setDeltaMovement(vx, vy, vz);
                    player.hurtMarked = true;
                    return 0;
                });
        LeafMinecraftHooks.installPlayerFlags((handle, flagsOut) -> {
            ServerPlayer player = PLAYERS.get(handle);
            if (player == null) {
                return 1;
            }
            int flags = 0;
            if (player.isShiftKeyDown()) {
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
            if (player.onGround()) {
                flags |= 16;
            }
            if (player.getAbilities().mayfly) {
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
                    server.getCommands().performPrefixedCommand(
                            server.createCommandSourceStack(), cmd);
                } else {
                    ServerPlayer player = PLAYERS.get(handle);
                    if (player == null) {
                        return 1;
                    }
                    server.getCommands().performPrefixedCommand(
                            player.createCommandSourceStack(), cmd);
                }
                return 0;
            } catch (Exception e) {
                return 1;
            }
        });
        LeafMinecraftHooks.installClearInventory(handle -> {
            ServerPlayer player = PLAYERS.get(handle);
            if (player == null) {
                return 1;
            }
            player.getInventory().clearContent();
            return 0;
        });
        LeafMinecraftHooks.installPlayerFlight((handle, allowFlight, flying) -> {
            ServerPlayer player = PLAYERS.get(handle);
            if (player == null) {
                return 1;
            }
            if (allowFlight >= 0) {
                player.getAbilities().mayfly = allowFlight != 0;
                if (allowFlight == 0) {
                    player.getAbilities().flying = false;
                }
            }
            if (flying >= 0) {
                if (flying != 0) {
                    player.getAbilities().mayfly = true;
                    player.getAbilities().flying = true;
                } else {
                    player.getAbilities().flying = false;
                }
            }
            player.onUpdateAbilities();
            return 0;
        });
        LeafMinecraftHooks.installGetBiome((dimension, x, y, z, outId) -> {
            MinecraftServer server = SERVER.get();
            if (server == null) {
                return 1;
            }
            ServerLevel level = switch (dimension) {
                case 1 -> server.getLevel(Level.NETHER);
                case 2 -> server.getLevel(Level.END);
                default -> server.overworld();
            };
            if (level == null) {
                return 1;
            }
            var holder = level.getBiome(new BlockPos(x, y, z));
            ResourceLocation id = level.registryAccess()
                    .registryOrThrow(net.minecraft.core.registries.Registries.BIOME)
                    .getKey(holder.value());
            outId[0] = id != null ? id.toString() : "minecraft:plains";
            return 0;
        });
        LeafMinecraftHooks.installDifficulty(
                difficultyOut -> {
                    MinecraftServer server = SERVER.get();
                    if (server == null) {
                        return 1;
                    }
                    difficultyOut[0] = server.overworld().getDifficulty().getId();
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
                    ServerLevel level = switch (dimension) {
                        case 1 -> server.getLevel(Level.NETHER);
                        case 2 -> server.getLevel(Level.END);
                        default -> server.overworld();
                    };
                    if (level == null) {
                        return 1;
                    }
                    if (level.isThundering()) {
                        weatherOut[0] = 2;
                    } else if (level.isRaining()) {
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
                    ServerLevel level = switch (dimension) {
                        case 1 -> server.getLevel(Level.NETHER);
                        case 2 -> server.getLevel(Level.END);
                        default -> server.overworld();
                    };
                    if (level == null) {
                        return 1;
                    }
                    int duration = durationTicks > 0 ? durationTicks : 6000;
                    switch (weather) {
                        case 1 -> level.setWeatherParameters(0, duration, true, false);
                        case 2 -> level.setWeatherParameters(0, duration, true, true);
                        default -> level.setWeatherParameters(duration, 0, false, false);
                    }
                    return 0;
                });
        LeafMinecraftHooks.installGetLightLevel(
                (dimension, x, y, z, blockOut, skyOut) -> {
                    MinecraftServer server = SERVER.get();
                    if (server == null) {
                        return 1;
                    }
                    ServerLevel level = switch (dimension) {
                        case 1 -> server.getLevel(Level.NETHER);
                        case 2 -> server.getLevel(Level.END);
                        default -> server.overworld();
                    };
                    if (level == null) {
                        return 1;
                    }
                    BlockPos pos = new BlockPos(x, y, z);
                    blockOut[0] = level.getBrightness(
                            net.minecraft.world.level.LightLayer.BLOCK, pos);
                    skyOut[0] = level.getBrightness(
                            net.minecraft.world.level.LightLayer.SKY, pos);
                    return 0;
                });
        LeafMinecraftHooks.installPlayerLatency((handle, latencyOut) -> {
            ServerPlayer player = PLAYERS.get(handle);
            if (player == null) {
                return 1;
            }
            latencyOut[0] = player.connection.latency();
            return 0;
        });
        LeafMinecraftHooks.installWorldSpawn(
                (dimension, xOut, yOut, zOut) -> {
                    MinecraftServer server = SERVER.get();
                    if (server == null) {
                        return 1;
                    }
                    ServerLevel level = switch (dimension) {
                        case 1 -> server.getLevel(Level.NETHER);
                        case 2 -> server.getLevel(Level.END);
                        default -> server.overworld();
                    };
                    if (level == null) {
                        return 1;
                    }
                    BlockPos spawn = level.getSharedSpawnPos();
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
                    ServerLevel level = switch (dimension) {
                        case 1 -> server.getLevel(Level.NETHER);
                        case 2 -> server.getLevel(Level.END);
                        default -> server.overworld();
                    };
                    if (level == null) {
                        return 1;
                    }
                    level.setDefaultSpawnPos(new BlockPos(x, y, z), 0.0f);
                    return 0;
                });
        LeafMinecraftHooks.installPlayerOp((handle, opOut) -> {
            ServerPlayer player = PLAYERS.get(handle);
            MinecraftServer server = SERVER.get();
            if (player == null || server == null) {
                return 1;
            }
            opOut[0] = server.getPlayerList().isOp(player.getGameProfile()) ? 1 : 0;
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
            ServerPlayer player = PLAYERS.get(handle);
            MinecraftServer server = SERVER.get();
            if (player == null || server == null) {
                return 1;
            }
            levelOut[0] = server.getProfilePermissions(player.getGameProfile());
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
                ServerPlayer player = entry.getValue();
                if (player != null
                        && name.equalsIgnoreCase(player.getGameProfile().getName())) {
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
                    var level = switch (dimension) {
                        case 1 -> server.getLevel(net.minecraft.world.level.Level.NETHER);
                        case 2 -> server.getLevel(net.minecraft.world.level.Level.END);
                        default -> server.overworld();
                    };
                    if (level == null) {
                        return 1;
                    }
                    var state = level.getBlockState(new BlockPos(x, y, z));
                    ResourceLocation id = BuiltInRegistries.BLOCK.getKey(state.getBlock());
                    outId[0] = id != null ? id.toString() : "minecraft:air";
                    return 0;
                },
                (dimension, x, y, z, blockId) -> {
                    MinecraftServer server = SERVER.get();
                    if (server == null || blockId == null || blockId.isEmpty()) {
                        return 1;
                    }
                    var level = switch (dimension) {
                        case 1 -> server.getLevel(net.minecraft.world.level.Level.NETHER);
                        case 2 -> server.getLevel(net.minecraft.world.level.Level.END);
                        default -> server.overworld();
                    };
                    if (level == null) {
                        return 1;
                    }
                    ResourceLocation id = ResourceLocation.tryParse(blockId);
                    if (id == null) {
                        return 1;
                    }
                    var block = BuiltInRegistries.BLOCK.get(id);
                    if (block == null) {
                        return 1;
                    }
                    level.setBlockAndUpdate(new BlockPos(x, y, z), block.defaultBlockState());
                    return 0;
                });
    }

    private void onServerStarting(ServerStartingEvent event) {
        SERVER.set(event.getServer());
        LeafBridge.requireOk("neoforge starting", LeafBridge.onServerStarting());
    }

    private void onServerStarted(ServerStartedEvent event) {
        SERVER.set(event.getServer());
        LeafBridge.requireOk("neoforge started", LeafBridge.onServerStarted());
    }

    private void onServerStopping(ServerStoppingEvent event) {
        LeafBridge.requireOk("neoforge stopping", LeafBridge.onServerStopping());
        LeafBridge.requireOk("neoforge shutdown", LeafBridge.shutdown());
        SERVER.set(null);
    }

    private void onServerTick(ServerTickEvent.Post event) {
        if (LeafBridge.isInitialized()) {
            LeafBridge.requireOk("neoforge pump", LeafBridge.pumpMain(64));
        }
    }

    private void onLogin(PlayerEvent.PlayerLoggedInEvent event) {
        if (!(event.getEntity() instanceof ServerPlayer player)) {
            return;
        }
        long handle = LeafPlayerHandles.allocate(player.getUUID());
        PLAYERS.put(handle, player);
        String name = player.getGameProfile().getName();
        LeafBridge.requireOk(
                "neoforge register player",
                LeafBridge.registerPlayer(handle, name));
        LeafMinecraftHooks.putPlayerSender(handle, (h, msg) -> {
            ServerPlayer p = PLAYERS.get(h);
            if (p != null) {
                p.sendSystemMessage(Component.literal(msg));
            }
        });
        LeafBridge.requireOk("neoforge join", LeafBridge.emitPlayerJoin(handle));
    }

    private void onLogout(PlayerEvent.PlayerLoggedOutEvent event) {
        if (!(event.getEntity() instanceof ServerPlayer player)) {
            return;
        }
        Long existing = LeafPlayerHandles.handleOf(player.getUUID());
        long handle = existing != null
                ? existing
                : LeafPlayerHandles.allocate(player.getUUID());
        LeafBridge.requireOk("neoforge leave", LeafBridge.emitPlayerLeave(handle));
        LeafBridge.unregisterPlayer(handle);
        LeafMinecraftHooks.removePlayer(handle);
        PLAYERS.remove(handle);
        LeafPlayerHandles.release(player.getUUID());
    }

    private void onChat(ServerChatEvent event) {
        ServerPlayer player = event.getPlayer();
        Long handle = LeafPlayerHandles.handleOf(player.getUUID());
        if (handle == null) {
            return;
        }
        int[] decision = new int[1];
        int code = LeafBridge.emitPlayerChat(handle, event.getRawText(), decision);
        if (code == 0 && decision[0] == LeafBridge.DECISION_DENY) {
            event.setCanceled(true);
        }
    }

    private void onDeath(LivingDeathEvent event) {
        if (!(event.getEntity() instanceof ServerPlayer player)) {
            return;
        }
        Long handle = LeafPlayerHandles.handleOf(player.getUUID());
        if (handle != null) {
            LeafBridge.emitPlayerDeath(handle);
        }
    }

    private void onEntityJoin(EntityJoinLevelEvent event) {
        Entity entity = event.getEntity();
        if (entity instanceof ServerPlayer || !(event.getLevel() instanceof ServerLevel level)) {
            return;
        }
        emitEntityLifecycle(true, entity, level);
    }

    private void onEntityLeave(EntityLeaveLevelEvent event) {
        Entity entity = event.getEntity();
        if (entity instanceof ServerPlayer || !(event.getLevel() instanceof ServerLevel level)) {
            return;
        }
        emitEntityLifecycle(false, entity, level);
    }

    private static void emitEntityLifecycle(boolean spawn, Entity entity, ServerLevel level) {
        if (!LeafBridge.isInitialized()) {
            return;
        }
        long handle = Integer.toUnsignedLong(entity.getId());
        int typeId = BuiltInRegistries.ENTITY_TYPE.getId(entity.getType());
        int x = entity.getBlockX();
        int y = entity.getBlockY();
        int z = entity.getBlockZ();
        int dimension;
        var key = level.dimension();
        if (key == Level.NETHER) {
            dimension = 1;
        } else if (key == Level.END) {
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

    private void onWorldLoad(LevelEvent.Load event) {
        if (!(event.getLevel() instanceof ServerLevel level)) {
            return;
        }
        if (!LeafBridge.isInitialized()) {
            return;
        }
        int dimension;
        var key = level.dimension();
        if (key == Level.NETHER) {
            dimension = 1;
        } else if (key == Level.END) {
            dimension = 2;
        } else {
            dimension = 0;
        }
        LeafBridge.emitWorldLoad(dimension);
    }

    private void onBlockBreak(BlockEvent.BreakEvent event) {
        if (!(event.getPlayer() instanceof ServerPlayer player)) {
            return;
        }
        Long handle = LeafPlayerHandles.handleOf(player.getUUID());
        long h = handle != null ? handle : 0L;
        var pos = event.getPos();
        int blockId = BuiltInRegistries.BLOCK.getId(event.getState().getBlock());
        int[] decision = new int[1];
        int code = LeafBridge.emitBlockBreak(
                h, pos.getX(), pos.getY(), pos.getZ(), blockId, decision);
        if (code == 0 && decision[0] == LeafBridge.DECISION_DENY) {
            event.setCanceled(true);
        }
    }

    private void onBlockPlace(BlockEvent.EntityPlaceEvent event) {
        if (!(event.getEntity() instanceof ServerPlayer player)) {
            return;
        }
        Long handle = LeafPlayerHandles.handleOf(player.getUUID());
        long h = handle != null ? handle : 0L;
        var pos = event.getPos();
        int blockId = BuiltInRegistries.BLOCK.getId(event.getPlacedBlock().getBlock());
        int[] decision = new int[1];
        int code = LeafBridge.emitBlockPlace(
                h, pos.getX(), pos.getY(), pos.getZ(), blockId, decision);
        if (code == 0 && decision[0] == LeafBridge.DECISION_DENY) {
            event.setCanceled(true);
        }
    }
}
