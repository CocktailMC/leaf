#pragma once

#include <expected>
#include <utility>

#include "leaf/core/error.hpp"

namespace leaf {

/// Preferred error-handling type inside the C++23 engine.
/// Never throw across the Leaf C ABI boundary; map to LeafStatus instead.
template <typename T>
using result = std::expected<T, error>;

using status = result<void>;

template <typename T>
[[nodiscard]] constexpr result<T> ok(T&& value) {
    return result<T>{std::forward<T>(value)};
}

[[nodiscard]] inline status ok() {
    return status{};
}

template <typename T = void>
[[nodiscard]] result<T> err(error_code code, std::string message = {}) {
    return std::unexpected(make_error(code, std::move(message)));
}

template <typename T = void>
[[nodiscard]] result<T> err(error e) {
    return std::unexpected(std::move(e));
}

} // namespace leaf
