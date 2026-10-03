#pragma once

/// Leaf C++23 SDK — thin wrappers over the stable C ABI.
///
/// Mods should include this header (or the individual `leaf/sdk/*.hpp` pieces)
/// and never depend on `engine/` internals or Minecraft / loader packages.

#include "leaf/sdk/api.hpp"
#include "leaf/sdk/events.hpp"
#include "leaf/sdk/mod.hpp"
#include "leaf/sdk/status.hpp"
