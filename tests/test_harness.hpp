#pragma once

#include <cstdlib>
#include <iostream>
#include <string_view>

namespace leaf::test {

struct stats {
    int passed{0};
    int failed{0};
};

inline stats& global_stats() {
    static stats s;
    return s;
}

inline void check(
    bool condition,
    std::string_view expr,
    std::string_view file,
    int line) {
    if (condition) {
        ++global_stats().passed;
        return;
    }
    ++global_stats().failed;
    std::cerr << "FAIL " << file << ':' << line << "  " << expr << '\n';
}

} // namespace leaf::test

#define LEAF_CHECK(expr) \
    ::leaf::test::check(static_cast<bool>(expr), #expr, __FILE__, __LINE__)

#define LEAF_REQUIRE(expr)                                                     \
    do {                                                                       \
        const bool _leaf_ok = static_cast<bool>(expr);                         \
        ::leaf::test::check(_leaf_ok, #expr, __FILE__, __LINE__);              \
        if (!_leaf_ok) {                                                       \
            std::cerr << "aborting test after REQUIRE failure\n";              \
            std::exit(1);                                                      \
        }                                                                      \
    } while (0)
