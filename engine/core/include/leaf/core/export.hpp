#pragma once

// Shared-library export macros for the LEAFMC engine.
// The C ABI surface uses LEAF_ABI_EXPORT; internal C++ APIs use LEAF_API.

#if defined(_WIN32) || defined(__CYGWIN__)
#  if defined(LEAF_ENGINE_BUILD)
#    define LEAF_API __declspec(dllexport)
#  else
#    define LEAF_API __declspec(dllimport)
#  endif
#  define LEAF_ABI_EXPORT __declspec(dllexport)
#else
#  define LEAF_API __attribute__((visibility("default")))
#  define LEAF_ABI_EXPORT __attribute__((visibility("default")))
#endif

#define LEAF_ABI_CALL
