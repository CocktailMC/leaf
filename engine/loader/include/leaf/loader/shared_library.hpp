#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <utility>

#include "leaf/core/result.hpp"

namespace leaf {

/// RAII wrapper around dlopen / LoadLibrary.
/// Move-only. Closing happens in the destructor (or explicit close()).
class shared_library {
public:
    shared_library() = default;
    ~shared_library();

    shared_library(const shared_library&) = delete;
    shared_library& operator=(const shared_library&) = delete;

    shared_library(shared_library&& other) noexcept;
    shared_library& operator=(shared_library&& other) noexcept;

    [[nodiscard]] static result<shared_library> open(
        const std::filesystem::path& path);

    [[nodiscard]] result<void*> symbol(std::string_view name) const;

    [[nodiscard]] bool is_open() const noexcept { return handle_ != nullptr; }

    [[nodiscard]] const std::filesystem::path& path() const noexcept {
        return path_;
    }

    void close() noexcept;

private:
    explicit shared_library(void* handle, std::filesystem::path path) noexcept
        : handle_(handle)
        , path_(std::move(path)) {}

    void* handle_{nullptr};
    std::filesystem::path path_;
};

} // namespace leaf
