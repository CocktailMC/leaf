#include "leaf/core/version.hpp"

namespace leaf {
namespace {

// Keep a TU so leaf_core is a real static library even while most APIs
// are header-only. Future diagnostics / process-global state can live here.
[[maybe_unused]] constexpr version k_linked_engine_version = engine_version;

} // namespace
} // namespace leaf
