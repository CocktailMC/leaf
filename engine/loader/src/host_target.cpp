#include "leaf/loader/host_target.hpp"

namespace leaf {

std::string current_host_target() {
#if defined(_WIN32) && (defined(_M_X64) || defined(__x86_64__))
    return "windows-x86_64";
#elif defined(_WIN32) && (defined(_M_ARM64) || defined(__aarch64__))
    return "windows-arm64";
#elif defined(__APPLE__) && defined(__aarch64__)
    return "macos-arm64";
#elif defined(__APPLE__) && defined(__x86_64__)
    return "macos-x86_64";
#elif defined(__linux__) && defined(__x86_64__)
    return "linux-x86_64";
#elif defined(__linux__) && defined(__aarch64__)
    return "linux-aarch64";
#else
    return "unknown";
#endif
}

std::string native_library_filename(std::string_view mod_id) {
#if defined(_WIN32)
    return std::string(mod_id) + ".dll";
#elif defined(__APPLE__)
    return "lib" + std::string(mod_id) + ".dylib";
#else
    return "lib" + std::string(mod_id) + ".so";
#endif
}

} // namespace leaf
