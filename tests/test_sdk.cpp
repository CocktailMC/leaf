#include "leaf/sdk/sdk.hpp"
#include "test_harness.hpp"

#include "leaf/abi/leaf_event_v1.h"

#include <cstring>
#include <string>
#include <vector>

namespace {

struct mock_api_state {
    std::vector<std::string> logs;
    int caps_mask{0};
    LeafHandle server{99};
    LeafStatus send_status{LEAF_STATUS_OK};
    std::string last_message;
    LeafHandle last_player{0};

    LeafEventCallback sub_cb{nullptr};
    void* sub_ud{nullptr};
    std::uint64_t next_sub{1};
    std::uint64_t active_sub{0};
};

mock_api_state* g_mock{nullptr};

void mock_log(int /*level*/, const char* message) {
    if (g_mock && message) {
        g_mock->logs.emplace_back(message);
    }
}

int mock_has_capability(uint32_t capability) {
    if (!g_mock) {
        return 0;
    }
    return (g_mock->caps_mask & (1 << static_cast<int>(capability))) ? 1 : 0;
}

LeafHandle mock_get_server() {
    return g_mock ? g_mock->server : 0;
}

LeafStatus mock_send(LeafHandle player, const char* message) {
    if (!g_mock || !message) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    g_mock->last_player = player;
    g_mock->last_message = message;
    return g_mock->send_status;
}

LeafStatus mock_subscribe(
    uint32_t /*event_id*/,
    LeafEventCallback callback,
    void* user_data,
    uint64_t* out_id) {
    if (!g_mock || !callback || !out_id) {
        return LEAF_STATUS_INVALID_ARGUMENT;
    }
    g_mock->sub_cb = callback;
    g_mock->sub_ud = user_data;
    g_mock->active_sub = g_mock->next_sub++;
    *out_id = g_mock->active_sub;
    return LEAF_STATUS_OK;
}

LeafStatus mock_unsubscribe(uint64_t id) {
    if (!g_mock || id == 0 || id != g_mock->active_sub) {
        return LEAF_STATUS_NOT_FOUND;
    }
    g_mock->sub_cb = nullptr;
    g_mock->sub_ud = nullptr;
    g_mock->active_sub = 0;
    return LEAF_STATUS_OK;
}

LeafApiV1 make_mock_table() {
    LeafApiV1 api{};
    api.abi_major = LEAF_ABI_VERSION_MAJOR;
    api.abi_minor = LEAF_ABI_VERSION_MINOR;
    api.struct_size = static_cast<uint32_t>(sizeof(LeafApiV1));
    api.log = &mock_log;
    api.has_capability = &mock_has_capability;
    api.get_server = &mock_get_server;
    api.send_player_message = &mock_send;
    api.subscribe_event = &mock_subscribe;
    api.unsubscribe_event = &mock_unsubscribe;
    return api;
}

class probe_mod final : public leaf::sdk::mod {
public:
    int loads{0};
    int enables{0};
    int disables{0};
    int unloads{0};
    int joins{0};
    leaf::sdk::subscription join_sub;

    [[nodiscard]] leaf::sdk::mod_info info() const noexcept override {
        return {.id = "probe", .name = "Probe", .version = "0.0.1"};
    }

    void on_load(leaf::sdk::api& a) override {
        ++loads;
        a.info("probe load");
    }

    void on_enable(leaf::sdk::api& a) override {
        ++enables;
        LEAF_CHECK(leaf::sdk::ok(
            a.subscribe_method<&probe_mod::on_join>(
                LEAF_EVENT_PLAYER_JOIN, this, join_sub)));
    }

    void on_disable(leaf::sdk::api& /*a*/) override {
        ++disables;
        join_sub.reset();
    }

    void on_unload(leaf::sdk::api& /*a*/) override { ++unloads; }

    void on_join(leaf::sdk::event_view view) {
        const auto ev = leaf::sdk::as_player_join(view);
        if (ev.player != 0) {
            ++joins;
        }
    }
};

} // namespace

void test_sdk_api_and_events() {
    mock_api_state state;
    state.caps_mask = (1 << LEAF_CAP_DATA_COMPONENTS);
    g_mock = &state;

    auto table = make_mock_table();
    leaf::sdk::api a{&table};

    LEAF_CHECK(a.valid());
    LEAF_CHECK(a.abi_major() == LEAF_ABI_VERSION_MAJOR);
    a.info("hello sdk");
    LEAF_CHECK(state.logs.size() == 1);
    LEAF_CHECK(state.logs[0] == "hello sdk");
    LEAF_CHECK(a.has_capability(LEAF_CAP_DATA_COMPONENTS));
    LEAF_CHECK(!a.has_capability(LEAF_CAP_LEGACY_ITEM_NBT));
    LEAF_CHECK(a.get_server() == 99);
    LEAF_CHECK(leaf::sdk::ok(a.send_player_message(7, "hi")));
    LEAF_CHECK(state.last_player == 7);
    LEAF_CHECK(state.last_message == "hi");

    // Contiguous packet: header + LeafPlayerJoinPayloadV1
    alignas(8) unsigned char packet[sizeof(LeafEventHeaderV1)
        + sizeof(LeafPlayerJoinPayloadV1)]{};
    auto* hdr = reinterpret_cast<LeafEventHeaderV1*>(packet);
    hdr->event_id = LEAF_EVENT_PLAYER_JOIN;
    hdr->schema_version = 1;
    hdr->payload_size = static_cast<uint32_t>(sizeof(LeafPlayerJoinPayloadV1));
    auto* payload = reinterpret_cast<LeafPlayerJoinPayloadV1*>(hdr + 1);
    payload->player = 42;

    leaf::sdk::event_view view{packet};
    LEAF_CHECK(view.valid());
    LEAF_CHECK(view.id() == LEAF_EVENT_PLAYER_JOIN);
    LEAF_CHECK(leaf::sdk::as_player_join(view).player == 42);

    g_mock = nullptr;
}

void test_sdk_mod_lifecycle() {
    mock_api_state state;
    g_mock = &state;
    auto table = make_mock_table();

    probe_mod probe;
    leaf::sdk::api a{&table};

    probe.on_load(a);
    LEAF_CHECK(probe.loads == 1);
    LEAF_CHECK(!state.logs.empty());

    probe.on_enable(a);
    LEAF_CHECK(probe.enables == 1);
    LEAF_CHECK(probe.join_sub.active());
    LEAF_CHECK(state.sub_cb != nullptr);

    alignas(8) unsigned char packet[sizeof(LeafEventHeaderV1)
        + sizeof(LeafPlayerJoinPayloadV1)]{};
    auto* hdr = reinterpret_cast<LeafEventHeaderV1*>(packet);
    hdr->event_id = LEAF_EVENT_PLAYER_JOIN;
    hdr->schema_version = 1;
    hdr->payload_size = static_cast<uint32_t>(sizeof(LeafPlayerJoinPayloadV1));
    reinterpret_cast<LeafPlayerJoinPayloadV1*>(hdr + 1)->player = 5;

    state.sub_cb(packet, state.sub_ud);
    LEAF_CHECK(probe.joins == 1);

    probe.on_disable(a);
    LEAF_CHECK(probe.disables == 1);
    LEAF_CHECK(!probe.join_sub.active());
    LEAF_CHECK(state.active_sub == 0);

    probe.on_unload(a);
    LEAF_CHECK(probe.unloads == 1);

    LEAF_CHECK(leaf::sdk::ok(leaf::sdk::status::ok));
    LEAF_CHECK(leaf::sdk::status_name(leaf::sdk::status::not_found)
        == "NOT_FOUND");

    g_mock = nullptr;
}

void test_sdk_bundle() {
    test_sdk_api_and_events();
    test_sdk_mod_lifecycle();
}
