#include "leaf/minecraft/minecraft_abi.hpp"
#include "leaf/minecraft/stub_minecraft_abi.hpp"
#include "leaf/abi/leaf_abi_v1.h"
#include "test_harness.hpp"

#include <string>

void test_minecraft_abi_stub() {
    auto abi = leaf::create_minecraft_abi(
        leaf::version{1, 21, 1},
        leaf::loader_kind::fabric);
    LEAF_CHECK(abi.has_value());

    auto* stub = dynamic_cast<leaf::stub_minecraft_abi*>(abi->get());
    LEAF_CHECK(stub != nullptr);
    LEAF_CHECK(stub->capabilities().has(leaf::capability::data_components));

    auto player = leaf::player_handle{42};
    stub->register_player(player, "Alice");

    LEAF_CHECK(stub->send_player_message(player, "Hello LEAF").has_value());
    LEAF_CHECK(stub->message_log_size() == 1);
    LEAF_CHECK(stub->message_at(0) == "Alice: Hello LEAF");
    LEAF_CHECK(stub->player_count() == 1);
    leaf::player_handle listed{};
    LEAF_CHECK(stub->get_player_at(0, listed).has_value());
    LEAF_CHECK(listed.raw() == player.raw());
    LEAF_CHECK(!stub->get_player_at(1, listed).has_value());
    LEAF_CHECK(stub->broadcast_message("hi all").has_value());
    LEAF_CHECK(stub->message_log_size() == 2);

    auto name = stub->player_name(player);
    LEAF_CHECK(name.has_value());
    LEAF_CHECK(*name == "Alice");

    std::uint32_t item_id = 99;
    std::uint32_t count = 99;
    LEAF_CHECK(stub->get_inventory_slot(player, 0, item_id, count).has_value());
    LEAF_CHECK(item_id == 0 && count == 0);
    LEAF_CHECK(stub->set_inventory_slot(player, 0, 1, 64).has_value());
    LEAF_CHECK(stub->get_inventory_slot(player, 0, item_id, count).has_value());
    LEAF_CHECK(item_id == 1 && count == 64);

    LeafItemStackV1 stack{};
    stack.struct_size = static_cast<uint32_t>(sizeof(LeafItemStackV1));
    stack.item_id = 2;
    stack.count = 16;
    stack.damage = 3;
    LEAF_CHECK(stub->set_inventory_stack(player, 1, stack).has_value());
    LeafItemStackV1 got{};
    LEAF_CHECK(stub->get_inventory_stack(player, 1, got).has_value());
    LEAF_CHECK(got.item_id == 2 && got.count == 16 && got.damage == 3);

    std::uint32_t block_id = 99;
    LEAF_CHECK(stub->get_block(0, 64, 0, block_id).has_value());
    LEAF_CHECK(block_id == 0);
    LEAF_CHECK(stub->set_block(0, 64, 0, 7).has_value());
    LEAF_CHECK(stub->get_block(0, 64, 0, block_id).has_value());
    LEAF_CHECK(block_id == 7);

    std::int32_t px = 0;
    std::int32_t py = 0;
    std::int32_t pz = 0;
    std::uint32_t pdim = 99;
    LEAF_CHECK(stub->get_player_pos(player, px, py, pz, pdim).has_value());
    LEAF_CHECK(px == 0 && py == 64 && pz == 0 && pdim == 0);
    LEAF_CHECK(stub->set_player_pos(player, 10, 70, -3, 1).has_value());
    LEAF_CHECK(stub->get_player_pos(player, px, py, pz, pdim).has_value());
    LEAF_CHECK(px == 10 && py == 70 && pz == -3 && pdim == 1);

    float health = 0.f;
    float max_health = 0.f;
    LEAF_CHECK(stub->get_player_health(player, health, max_health).has_value());
    LEAF_CHECK(health == 20.f && max_health == 20.f);
    LEAF_CHECK(stub->set_player_health(player, 12.5f).has_value());
    LEAF_CHECK(stub->get_player_health(player, health, max_health).has_value());
    LEAF_CHECK(health == 12.5f && max_health == 20.f);

    std::int32_t food = 0;
    float sat = 0.f;
    LEAF_CHECK(stub->get_player_food(player, food, sat).has_value());
    LEAF_CHECK(food == 20 && sat == 5.f);
    LEAF_CHECK(stub->set_player_food(player, 8, 2.5f).has_value());
    LEAF_CHECK(stub->get_player_food(player, food, sat).has_value());
    LEAF_CHECK(food == 8 && sat == 2.5f);

    std::uint32_t mode = 99;
    LEAF_CHECK(stub->get_player_gamemode(player, mode).has_value());
    LEAF_CHECK(mode == 0);
    LEAF_CHECK(stub->set_player_gamemode(player, 1).has_value());
    LEAF_CHECK(stub->get_player_gamemode(player, mode).has_value());
    LEAF_CHECK(mode == 1);

    std::int32_t xp_level = -1;
    float xp_progress = -1.f;
    LEAF_CHECK(stub->get_player_xp(player, xp_level, xp_progress).has_value());
    LEAF_CHECK(xp_level == 0 && xp_progress == 0.f);
    LEAF_CHECK(stub->set_player_xp_level(player, 30).has_value());
    LEAF_CHECK(stub->get_player_xp(player, xp_level, xp_progress).has_value());
    LEAF_CHECK(xp_level == 30 && xp_progress == 0.f);

    float yaw = -1.f;
    float pitch = -1.f;
    LEAF_CHECK(stub->get_player_look(player, yaw, pitch).has_value());
    LEAF_CHECK(yaw == 0.f && pitch == 0.f);
    LEAF_CHECK(stub->set_player_look(player, 90.f, -45.f).has_value());
    LEAF_CHECK(stub->get_player_look(player, yaw, pitch).has_value());
    LEAF_CHECK(yaw == 90.f && pitch == -45.f);

    LEAF_CHECK(stub->sound_log_size() == 0);
    LEAF_CHECK(stub->play_sound(
                   player,
                   "minecraft:entity.experience_orb.pickup",
                   1.f,
                   1.f,
                   0,
                   64,
                   0,
                   0)
                   .has_value());
    LEAF_CHECK(stub->sound_log_size() == 1);
    LEAF_CHECK(
        stub->sound_at(0).find("minecraft:entity.experience_orb.pickup")
        != std::string::npos);

    LEAF_CHECK(stub->actionbar_log_size() == 0);
    LEAF_CHECK(stub->send_actionbar(player, "Action!").has_value());
    LEAF_CHECK(stub->actionbar_log_size() == 1);
    LEAF_CHECK(stub->actionbar_at(0) == "Action!");
    LEAF_CHECK(stub->title_log_size() == 0);
    LEAF_CHECK(stub->send_title(player, "Hello", "World", 10, 40, 10).has_value());
    LEAF_CHECK(stub->title_log_size() == 1);
    LEAF_CHECK(stub->title_at(0).find("Hello|World") != std::string::npos);

    LEAF_CHECK(stub->kick_log_size() == 0);
    LEAF_CHECK(stub->kick_player(player, "bye").has_value());
    LEAF_CHECK(stub->kick_log_size() == 1);
    LEAF_CHECK(stub->kick_at(0) == "bye");
    LEAF_CHECK(stub->player_count() == 0);

    // Fresh player for give_item (kick unregistered the previous one).
    player = leaf::player_handle{42};
    stub->register_player(player, "Alice");
    LEAF_CHECK(stub->give_item(player, 1, 5, -1).has_value());
    {
        LeafItemStackV1 given{};
        LEAF_CHECK(stub->get_inventory_stack(player, 0, given).has_value());
        LEAF_CHECK(given.item_id == 1 && given.count == 5);
    }
    LEAF_CHECK(stub->give_item(player, 1, 60, -1).has_value());
    {
        LeafItemStackV1 given{};
        LEAF_CHECK(stub->get_inventory_stack(player, 0, given).has_value());
        LEAF_CHECK(given.item_id == 1 && given.count == 64);
        LEAF_CHECK(stub->get_inventory_stack(player, 1, given).has_value());
        LEAF_CHECK(given.item_id == 1 && given.count == 1);
    }

    LEAF_CHECK(stub->apply_effect(player, "minecraft:speed", 200, 1, 0).has_value());
    LEAF_CHECK(stub->effect_log_size() >= 1);
    LEAF_CHECK(stub->clear_effects(player).has_value());
    LEAF_CHECK(stub->effect_at(stub->effect_log_size() - 1).find("clear@")
        != std::string::npos);

    LEAF_CHECK(stub->spawn_particle(
                   "minecraft:happy_villager", 0, 64, 0, 0, 8, 0.2, 0.2, 0.2, 0.01)
                   .has_value());
    LEAF_CHECK(stub->particle_log_size() == 1);
    LEAF_CHECK(
        stub->particle_at(0).find("minecraft:happy_villager") != std::string::npos);

    std::int64_t time = -1;
    LEAF_CHECK(stub->get_world_time(0, time).has_value());
    LEAF_CHECK(time == 0);
    LEAF_CHECK(stub->set_world_time(0, 6000).has_value());
    LEAF_CHECK(stub->get_world_time(0, time).has_value());
    LEAF_CHECK(time == 6000);

    double vx = -1, vy = -1, vz = -1;
    LEAF_CHECK(stub->get_player_velocity(player, vx, vy, vz).has_value());
    LEAF_CHECK(vx == 0 && vy == 0 && vz == 0);
    LEAF_CHECK(stub->set_player_velocity(player, 0.1, 0.5, -0.2).has_value());
    LEAF_CHECK(stub->get_player_velocity(player, vx, vy, vz).has_value());
    LEAF_CHECK(vx == 0.1 && vy == 0.5 && vz == -0.2);

    stub->set_player_flags_for_test(
        player, LEAF_PLAYER_FLAG_SNEAKING | LEAF_PLAYER_FLAG_ON_GROUND);
    std::uint32_t flags = 0;
    LEAF_CHECK(stub->get_player_flags(player, flags).has_value());
    LEAF_CHECK((flags & LEAF_PLAYER_FLAG_SNEAKING) != 0);
    LEAF_CHECK((flags & LEAF_PLAYER_FLAG_ON_GROUND) != 0);

    LEAF_CHECK(stub->run_command(player, "say hello").has_value());
    LEAF_CHECK(stub->command_log_size() == 1);
    LEAF_CHECK(stub->command_at(0).find("say hello") != std::string::npos);
    LEAF_CHECK(stub->run_command(leaf::player_handle{0}, "time set day").has_value());
    LEAF_CHECK(stub->command_log_size() == 2);

    LEAF_CHECK(stub->give_item(player, 1, 3, -1).has_value());
    LEAF_CHECK(stub->clear_inventory(player).has_value());
    LeafItemStackV1 cleared{};
    LEAF_CHECK(stub->get_inventory_stack(player, 0, cleared).has_value());
    LEAF_CHECK(cleared.item_id == 0);

    LEAF_CHECK(stub->broadcast_actionbar("all bar").has_value());
    LEAF_CHECK(stub->actionbar_log_size() >= 1);
    LEAF_CHECK(stub->broadcast_title("Hi", "there", 10, 40, 10).has_value());

    LEAF_CHECK(stub->set_player_flight(player, 1, 1).has_value());
    flags = 0;
    LEAF_CHECK(stub->get_player_flags(player, flags).has_value());
    LEAF_CHECK((flags & LEAF_PLAYER_FLAG_FLYING) != 0);
    LEAF_CHECK((flags & LEAF_PLAYER_FLAG_ALLOW_FLIGHT) != 0);
    LEAF_CHECK(stub->set_player_flight(player, -1, 0).has_value());
    LEAF_CHECK(stub->get_player_flags(player, flags).has_value());
    LEAF_CHECK((flags & LEAF_PLAYER_FLAG_FLYING) == 0);
    LEAF_CHECK((flags & LEAF_PLAYER_FLAG_ALLOW_FLIGHT) != 0);

    std::string biome;
    LEAF_CHECK(stub->get_biome(0, 0, 64, 0, biome).has_value());
    LEAF_CHECK(biome == "minecraft:plains");

    std::uint32_t difficulty = 99;
    LEAF_CHECK(stub->get_difficulty(difficulty).has_value());
    LEAF_CHECK(difficulty == 2);
    LEAF_CHECK(stub->set_difficulty(3).has_value());
    LEAF_CHECK(stub->get_difficulty(difficulty).has_value());
    LEAF_CHECK(difficulty == 3);
    LEAF_CHECK(!stub->set_difficulty(4).has_value());

    std::uint32_t weather = 99;
    LEAF_CHECK(stub->get_weather(0, weather).has_value());
    LEAF_CHECK(weather == LEAF_WEATHER_CLEAR);
    LEAF_CHECK(stub->set_weather(0, LEAF_WEATHER_RAIN, 6000).has_value());
    LEAF_CHECK(stub->get_weather(0, weather).has_value());
    LEAF_CHECK(weather == LEAF_WEATHER_RAIN);

    std::uint32_t block_light = 99;
    std::uint32_t sky_light = 99;
    LEAF_CHECK(stub->get_light_level(0, 0, 64, 0, block_light, sky_light).has_value());
    LEAF_CHECK(block_light == 0);
    LEAF_CHECK(sky_light == 15);

    stub->set_player_latency_for_test(player, 42);
    std::int32_t latency = -1;
    LEAF_CHECK(stub->get_player_latency(player, latency).has_value());
    LEAF_CHECK(latency == 42);

    std::int32_t sx = -1, sy = -1, sz = -1;
    LEAF_CHECK(stub->get_world_spawn(0, sx, sy, sz).has_value());
    LEAF_CHECK(sx == 0 && sy == 64 && sz == 0);
    LEAF_CHECK(stub->set_world_spawn(0, 10, 70, -5).has_value());
    LEAF_CHECK(stub->get_world_spawn(0, sx, sy, sz).has_value());
    LEAF_CHECK(sx == 10 && sy == 70 && sz == -5);

    stub->set_player_op_for_test(player, true);
    std::int32_t is_op = 0;
    LEAF_CHECK(stub->is_player_op(player, is_op).has_value());
    LEAF_CHECK(is_op == 1);

    stub->set_player_uuid_for_test(player, "550e8400-e29b-41d4-a716-446655440000");
    std::string uuid;
    LEAF_CHECK(stub->get_player_uuid(player, uuid).has_value());
    LEAF_CHECK(uuid == "550e8400-e29b-41d4-a716-446655440000");

    stub->set_player_permission_for_test(player, 3);
    std::int32_t perm = -1;
    LEAF_CHECK(stub->get_player_permission_level(player, perm).has_value());
    LEAF_CHECK(perm == 3);

    leaf::player_handle found{};
    LEAF_CHECK(stub->find_player_by_uuid(
        "550e8400-e29b-41d4-a716-446655440000", found).has_value());
    LEAF_CHECK(found.raw() == player.raw());
    LEAF_CHECK(!stub->find_player_by_uuid(
        "00000000-0000-4000-8000-ffffffffffff", found).has_value());

    leaf::player_handle by_name{};
    LEAF_CHECK(stub->find_player_by_name("Alice", by_name).has_value());
    LEAF_CHECK(by_name.raw() == player.raw());
    LEAF_CHECK(!stub->find_player_by_name("Nobody", by_name).has_value());

    LEAF_CHECK(stub->set_block_registry_id(0, 1, 64, 1, "minecraft:stone").has_value());
    std::string block_reg;
    LEAF_CHECK(stub->get_block_registry_id(0, 1, 64, 1, block_reg).has_value());
    LEAF_CHECK(block_reg == "minecraft:stone");
    LEAF_CHECK(stub->set_block_registry_id(0, 1, 64, 1, "minecraft:air").has_value());
    LEAF_CHECK(stub->get_block_registry_id(0, 1, 64, 1, block_reg).has_value());
    LEAF_CHECK(block_reg == "minecraft:air");

    LEAF_CHECK(stub->give_item_registry_id(
        player, "minecraft:diamond", 2, -1).has_value());

    LEAF_CHECK(stub->set_inventory_item_registry_id(
        player, 0, "minecraft:apple", 3, -1).has_value());
    std::string inv_id;
    std::uint32_t inv_count = 0;
    std::int32_t inv_damage = 0;
    LEAF_CHECK(stub->get_inventory_item_registry_id(
        player, 0, inv_id, inv_count, inv_damage).has_value());
    LEAF_CHECK(inv_id == "minecraft:apple");
    LEAF_CHECK(inv_count == 3);

    LEAF_CHECK(stub->teleport_player(player, 10, 70, -5, 0, 90.0f, 0.0f).has_value());
    std::int32_t tx = 0, ty = 0, tz = 0;
    std::uint32_t tdim = 99;
    LEAF_CHECK(stub->get_player_pos(player, tx, ty, tz, tdim).has_value());
    LEAF_CHECK(tx == 10 && ty == 70 && tz == -5 && tdim == 0);

    LEAF_CHECK(stub->set_selected_slot(player, 3).has_value());
    std::uint32_t sel = 99;
    LEAF_CHECK(stub->get_selected_slot(player, sel).has_value());
    LEAF_CHECK(sel == 3);

    LEAF_CHECK(stub->set_held_item_registry_id(
        player, "minecraft:stick", 5, -1).has_value());
    std::string held_id;
    std::uint32_t held_count = 0;
    std::int32_t held_damage = 0;
    LEAF_CHECK(stub->get_held_item_registry_id(
        player, held_id, held_count, held_damage).has_value());
    LEAF_CHECK(held_id == "minecraft:stick");
    LEAF_CHECK(held_count == 5);

    LEAF_CHECK(stub->set_equipment_item_registry_id(
        player, LEAF_EQUIP_OFFHAND, "minecraft:shield", 1, -1).has_value());
    LEAF_CHECK(stub->set_equipment_item_registry_id(
        player, LEAF_EQUIP_HEAD, "minecraft:iron_helmet", 1, 0).has_value());
    std::string equip_id;
    std::uint32_t equip_count = 0;
    std::int32_t equip_damage = 0;
    LEAF_CHECK(stub->get_equipment_item_registry_id(
        player, LEAF_EQUIP_OFFHAND, equip_id, equip_count, equip_damage)
                   .has_value());
    LEAF_CHECK(equip_id == "minecraft:shield");
    LEAF_CHECK(stub->get_equipment_item_registry_id(
        player, LEAF_EQUIP_HEAD, equip_id, equip_count, equip_damage)
                   .has_value());
    LEAF_CHECK(equip_id == "minecraft:iron_helmet");
    LEAF_CHECK(stub->get_equipment_item_registry_id(
        player, LEAF_EQUIP_MAINHAND, equip_id, equip_count, equip_damage)
                   .has_value());
    LEAF_CHECK(equip_id == "minecraft:stick");

    stub->set_world_seed_for_test(0, 1234567890123LL);
    std::int64_t seed = 0;
    LEAF_CHECK(stub->get_world_seed(0, seed).has_value());
    LEAF_CHECK(seed == 1234567890123LL);

    LEAF_CHECK(stub->set_player_health(player, 10.0f).has_value());
    LEAF_CHECK(stub->heal_player(player).has_value());
    float hp = 0.0f;
    float max_hp = 0.0f;
    LEAF_CHECK(stub->get_player_health(player, hp, max_hp).has_value());
    LEAF_CHECK(hp == max_hp);

    LEAF_CHECK(stub->set_player_health(player, 0.0f).has_value());
    std::int32_t alive = 1;
    LEAF_CHECK(stub->is_player_alive(player, alive).has_value());
    LEAF_CHECK(alive == 0);
    LEAF_CHECK(stub->heal_player(player).has_value());
    LEAF_CHECK(stub->is_player_alive(player, alive).has_value());
    LEAF_CHECK(alive == 1);

    LEAF_CHECK(stub->set_player_absorption(player, 4.0f).has_value());
    float abs = 0.0f;
    LEAF_CHECK(stub->get_player_absorption(player, abs).has_value());
    LEAF_CHECK(abs == 4.0f);

    LEAF_CHECK(stub->set_player_invulnerable(player, 1).has_value());
    std::int32_t invuln = 0;
    LEAF_CHECK(stub->get_player_invulnerable(player, invuln).has_value());
    LEAF_CHECK(invuln == 1);

    LEAF_CHECK(stub->set_player_air(player, 150).has_value());
    std::int32_t air = 0;
    LEAF_CHECK(stub->get_player_air(player, air).has_value());
    LEAF_CHECK(air == 150);

    LEAF_CHECK(stub->set_player_fire_ticks(player, 40).has_value());
    std::int32_t fire = 0;
    LEAF_CHECK(stub->get_player_fire_ticks(player, fire).has_value());
    LEAF_CHECK(fire == 40);

    LEAF_CHECK(stub->set_player_frozen_ticks(player, 25).has_value());
    std::int32_t frozen = 0;
    LEAF_CHECK(stub->get_player_frozen_ticks(player, frozen).has_value());
    LEAF_CHECK(frozen == 25);

    LEAF_CHECK(stub->extinguish_player(player).has_value());
    LEAF_CHECK(stub->get_player_fire_ticks(player, fire).has_value());
    LEAF_CHECK(fire == 0);
    LEAF_CHECK(stub->unfreeze_player(player).has_value());
    LEAF_CHECK(stub->get_player_frozen_ticks(player, frozen).has_value());
    LEAF_CHECK(frozen == 0);

    LEAF_CHECK(stub->set_player_no_gravity(player, 1).has_value());
    std::int32_t no_grav = 0;
    LEAF_CHECK(stub->get_player_no_gravity(player, no_grav).has_value());
    LEAF_CHECK(no_grav == 1);
    LEAF_CHECK(stub->set_player_no_gravity(player, 0).has_value());
    LEAF_CHECK(stub->get_player_no_gravity(player, no_grav).has_value());
    LEAF_CHECK(no_grav == 0);

    LEAF_CHECK(stub->set_player_silent(player, 1).has_value());
    std::int32_t silent = 0;
    LEAF_CHECK(stub->get_player_silent(player, silent).has_value());
    LEAF_CHECK(silent == 1);
    LEAF_CHECK(stub->set_player_silent(player, 0).has_value());
    LEAF_CHECK(stub->get_player_silent(player, silent).has_value());
    LEAF_CHECK(silent == 0);

    LEAF_CHECK(stub->set_player_glowing(player, 1).has_value());
    std::int32_t glowing = 0;
    LEAF_CHECK(stub->get_player_glowing(player, glowing).has_value());
    LEAF_CHECK(glowing == 1);
    LEAF_CHECK(stub->set_player_glowing(player, 0).has_value());
    LEAF_CHECK(stub->get_player_glowing(player, glowing).has_value());
    LEAF_CHECK(glowing == 0);

    LEAF_CHECK(stub->set_player_invisible(player, 1).has_value());
    std::int32_t invisible = 0;
    LEAF_CHECK(stub->get_player_invisible(player, invisible).has_value());
    LEAF_CHECK(invisible == 1);
    LEAF_CHECK(stub->set_player_invisible(player, 0).has_value());
    LEAF_CHECK(stub->get_player_invisible(player, invisible).has_value());
    LEAF_CHECK(invisible == 0);

    LEAF_CHECK(stub->set_player_portal_cooldown(player, 40).has_value());
    std::int32_t portal_cd = 0;
    LEAF_CHECK(stub->get_player_portal_cooldown(player, portal_cd).has_value());
    LEAF_CHECK(portal_cd == 40);
    LEAF_CHECK(stub->set_player_portal_cooldown(player, 0).has_value());
    LEAF_CHECK(stub->get_player_portal_cooldown(player, portal_cd).has_value());
    LEAF_CHECK(portal_cd == 0);

    stub->set_player_max_air_for_test(player, 300);
    std::int32_t max_air = 0;
    LEAF_CHECK(stub->get_player_max_air(player, max_air).has_value());
    LEAF_CHECK(max_air == 300);
    stub->set_player_max_air_for_test(player, 600);
    LEAF_CHECK(stub->get_player_max_air(player, max_air).has_value());
    LEAF_CHECK(max_air == 600);

    LEAF_CHECK(stub->set_player_air(player, 50).has_value());
    LEAF_CHECK(stub->refill_player_air(player).has_value());
    LEAF_CHECK(stub->get_player_air(player, air).has_value());
    LEAF_CHECK(air == max_air);

    stub->unregister_player(player);
    LEAF_CHECK(stub->player_count() == 0);

    auto server = stub->get_server();
    LEAF_CHECK(server.has_value());
    LEAF_CHECK(server->valid());

    // 1.20.1 should prefer legacy capabilities.
    auto legacy = leaf::create_minecraft_abi(
        leaf::version{1, 20, 1},
        leaf::loader_kind::forge);
    LEAF_CHECK(legacy.has_value());
    LEAF_CHECK(legacy.value()->capabilities().has(leaf::capability::legacy_item_nbt));
    LEAF_CHECK(!legacy.value()->capabilities().has(leaf::capability::data_components));
}

void test_minecraft_abi_bundle() {
    test_minecraft_abi_stub();
}
