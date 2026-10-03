#include "leaf/core/error.hpp"
#include "test_harness.hpp"

#include <string>

void test_error_hex_codec() {
    constexpr auto code = leaf::make_error_code(
        leaf::error_severity::error,
        leaf::error_module::event,
        0x0006);
    LEAF_CHECK(code == 0x02030006u);
    LEAF_CHECK(code == leaf::ec::event_recursive_limit);
    LEAF_CHECK(code == LEAF_EVENT_RECURSION_LIMIT);

    LEAF_CHECK(leaf::error_severity_of(code) == 0x02);
    LEAF_CHECK(leaf::error_module_of(code) == 0x03);
    LEAF_CHECK(leaf::error_number_of(code) == 0x0006);

    LEAF_CHECK(leaf::error_symbol(code) == "LEAF_EVENT_RECURSION_LIMIT");
    LEAF_CHECK(leaf::module_name(code) == "event");
    LEAF_CHECK(leaf::format_error_code(code) == "0x02030006");

    leaf::error err{code};
    const auto formatted = err.format();
    LEAF_CHECK(formatted.find("0x02030006") != std::string::npos);
    LEAF_CHECK(formatted.find("LEAF_EVENT_RECURSION_LIMIT") != std::string::npos);

    LEAF_CHECK(leaf::is_success(leaf::ok_code));
    LEAF_CHECK(leaf::is_failure(code));

    LEAF_CHECK(leaf::ec::invalid_handle == 0x02050001u);
    LEAF_CHECK(leaf::ec::abi_version_mismatch == 0x0202000Cu);
    LEAF_CHECK(leaf::ec::stale_handle == 0x02050002u);
}
