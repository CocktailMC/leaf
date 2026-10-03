#include "leaf/loader/shared_library.hpp"

#include <string>

#if defined(_WIN32)
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>
#else
#  include <dlfcn.h>
#endif

namespace leaf {

shared_library::~shared_library() {
    close();
}

shared_library::shared_library(shared_library&& other) noexcept
    : handle_(other.handle_)
    , path_(std::move(other.path_)) {
    other.handle_ = nullptr;
}

shared_library& shared_library::operator=(shared_library&& other) noexcept {
    if (this != &other) {
        close();
        handle_ = other.handle_;
        path_ = std::move(other.path_);
        other.handle_ = nullptr;
    }
    return *this;
}

void shared_library::close() noexcept {
    if (!handle_) {
        return;
    }
#if defined(_WIN32)
    FreeLibrary(static_cast<HMODULE>(handle_));
#else
    dlclose(handle_);
#endif
    handle_ = nullptr;
}

result<shared_library> shared_library::open(const std::filesystem::path& path) {
#if defined(_WIN32)
    HMODULE handle = LoadLibraryW(path.wstring().c_str());
    if (!handle) {
        return err<shared_library>(
            ec::mod_load_failed,
            "LoadLibrary failed for " + path.string());
    }
    return shared_library{handle, path};
#else
    // RTLD_LOCAL keeps mod symbols from colliding across mods.
    void* handle = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (!handle) {
        const char* msg = dlerror();
        return err<shared_library>(
            ec::mod_load_failed,
            std::string("dlopen failed for ") + path.string() + ": "
                + (msg ? msg : "unknown error"));
    }
    return shared_library{handle, path};
#endif
}

result<void*> shared_library::symbol(std::string_view name) const {
    if (!handle_) {
        return err<void*>(ec::mod_load_failed, "library is not open");
    }

    const std::string symbol_name{name};
#if defined(_WIN32)
    FARPROC sym = GetProcAddress(static_cast<HMODULE>(handle_), symbol_name.c_str());
    if (!sym) {
        return err<void*>(
            ec::mod_entry_missing,
            "symbol not found: " + symbol_name);
    }
    return reinterpret_cast<void*>(sym);
#else
    dlerror(); // clear
    void* sym = dlsym(handle_, symbol_name.c_str());
    const char* err_msg = dlerror();
    if (err_msg != nullptr || sym == nullptr) {
        return err<void*>(
            ec::mod_entry_missing,
            "symbol not found: " + symbol_name
                + (err_msg ? std::string(" (") + err_msg + ")" : ""));
    }
    return sym;
#endif
}

} // namespace leaf
