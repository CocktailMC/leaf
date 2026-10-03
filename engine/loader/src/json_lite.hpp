#pragma once

#include <cctype>
#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "leaf/core/result.hpp"

namespace leaf::json {

struct value;

using object = std::map<std::string, value, std::less<>>;
using array = std::vector<value>;

struct value {
    using storage = std::variant<std::nullptr_t, bool, double, std::string, array, object>;

    storage data{nullptr};

    [[nodiscard]] bool is_null() const noexcept {
        return std::holds_alternative<std::nullptr_t>(data);
    }
    [[nodiscard]] bool is_bool() const noexcept {
        return std::holds_alternative<bool>(data);
    }
    [[nodiscard]] bool is_number() const noexcept {
        return std::holds_alternative<double>(data);
    }
    [[nodiscard]] bool is_string() const noexcept {
        return std::holds_alternative<std::string>(data);
    }
    [[nodiscard]] bool is_array() const noexcept {
        return std::holds_alternative<array>(data);
    }
    [[nodiscard]] bool is_object() const noexcept {
        return std::holds_alternative<object>(data);
    }

    [[nodiscard]] const std::string* as_string() const noexcept {
        return std::get_if<std::string>(&data);
    }
    [[nodiscard]] const object* as_object() const noexcept {
        return std::get_if<object>(&data);
    }
    [[nodiscard]] const array* as_array() const noexcept {
        return std::get_if<array>(&data);
    }
    [[nodiscard]] const double* as_number() const noexcept {
        return std::get_if<double>(&data);
    }
};

class parser {
public:
    explicit parser(std::string_view input)
        : input_(input) {}

    [[nodiscard]] result<value> parse() {
        skip_ws();
        auto v = parse_value();
        if (!v) {
            return v;
        }
        skip_ws();
        if (pos_ != input_.size()) {
            return err_at("unexpected trailing input");
        }
        return v;
    }

private:
    std::string_view input_;
    std::size_t pos_{0};

    [[nodiscard]] result<value> err_at(std::string message) const {
        return err<value>(
            ec::mod_manifest_invalid,
            message + " at offset " + std::to_string(pos_));
    }

    void skip_ws() {
        while (pos_ < input_.size()
            && (input_[pos_] == ' ' || input_[pos_] == '\t' || input_[pos_] == '\n'
                || input_[pos_] == '\r')) {
            ++pos_;
        }
    }

    [[nodiscard]] bool consume(char c) {
        skip_ws();
        if (pos_ < input_.size() && input_[pos_] == c) {
            ++pos_;
            return true;
        }
        return false;
    }

    [[nodiscard]] result<value> parse_value() {
        skip_ws();
        if (pos_ >= input_.size()) {
            return err_at("unexpected end of input");
        }
        const char c = input_[pos_];
        if (c == '{') {
            return parse_object();
        }
        if (c == '[') {
            return parse_array();
        }
        if (c == '"') {
            auto s = parse_string();
            if (!s) {
                return err<value>(s.error());
            }
            return value{.data = std::move(*s)};
        }
        if (c == 't' || c == 'f') {
            return parse_bool();
        }
        if (c == 'n') {
            return parse_null();
        }
        if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) {
            return parse_number();
        }
        return err_at("unexpected character");
    }

    [[nodiscard]] result<std::string> parse_string() {
        if (pos_ >= input_.size() || input_[pos_] != '"') {
            return err<std::string>(
                ec::mod_manifest_invalid,
                "expected string");
        }
        ++pos_;
        std::string out;
        while (pos_ < input_.size()) {
            const char c = input_[pos_++];
            if (c == '"') {
                return out;
            }
            if (c == '\\') {
                if (pos_ >= input_.size()) {
                    return err<std::string>(
                        ec::mod_manifest_invalid,
                        "unterminated escape");
                }
                const char e = input_[pos_++];
                switch (e) {
                    case '"':
                    case '\\':
                    case '/':
                        out.push_back(e);
                        break;
                    case 'b':
                        out.push_back('\b');
                        break;
                    case 'f':
                        out.push_back('\f');
                        break;
                    case 'n':
                        out.push_back('\n');
                        break;
                    case 'r':
                        out.push_back('\r');
                        break;
                    case 't':
                        out.push_back('\t');
                        break;
                    default:
                        return err<std::string>(
                            ec::mod_manifest_invalid,
                            "unsupported escape sequence");
                }
                continue;
            }
            if (static_cast<unsigned char>(c) < 0x20) {
                return err<std::string>(
                    ec::mod_manifest_invalid,
                    "control character in string");
            }
            out.push_back(c);
        }
        return err<std::string>(
            ec::mod_manifest_invalid,
            "unterminated string");
    }

    [[nodiscard]] result<value> parse_object() {
        if (!consume('{')) {
            return err_at("expected '{'");
        }
        object obj;
        skip_ws();
        if (consume('}')) {
            return value{.data = std::move(obj)};
        }
        while (true) {
            skip_ws();
            auto key = parse_string();
            if (!key) {
                return err<value>(key.error());
            }
            if (!consume(':')) {
                return err_at("expected ':' after object key");
            }
            auto val = parse_value();
            if (!val) {
                return val;
            }
            obj.emplace(std::move(*key), std::move(*val));
            skip_ws();
            if (consume('}')) {
                return value{.data = std::move(obj)};
            }
            if (!consume(',')) {
                return err_at("expected ',' or '}' in object");
            }
        }
    }

    [[nodiscard]] result<value> parse_array() {
        if (!consume('[')) {
            return err_at("expected '['");
        }
        array arr;
        skip_ws();
        if (consume(']')) {
            return value{.data = std::move(arr)};
        }
        while (true) {
            auto val = parse_value();
            if (!val) {
                return val;
            }
            arr.push_back(std::move(*val));
            skip_ws();
            if (consume(']')) {
                return value{.data = std::move(arr)};
            }
            if (!consume(',')) {
                return err_at("expected ',' or ']' in array");
            }
        }
    }

    [[nodiscard]] result<value> parse_bool() {
        if (input_.substr(pos_, 4) == "true") {
            pos_ += 4;
            return value{.data = true};
        }
        if (input_.substr(pos_, 5) == "false") {
            pos_ += 5;
            return value{.data = false};
        }
        return err_at("invalid boolean");
    }

    [[nodiscard]] result<value> parse_null() {
        if (input_.substr(pos_, 4) == "null") {
            pos_ += 4;
            return value{.data = nullptr};
        }
        return err_at("invalid null");
    }

    [[nodiscard]] result<value> parse_number() {
        const std::size_t start = pos_;
        if (pos_ < input_.size() && input_[pos_] == '-') {
            ++pos_;
        }
        if (pos_ >= input_.size()
            || !std::isdigit(static_cast<unsigned char>(input_[pos_]))) {
            return err_at("invalid number");
        }
        if (input_[pos_] == '0') {
            ++pos_;
        } else {
            while (pos_ < input_.size()
                && std::isdigit(static_cast<unsigned char>(input_[pos_]))) {
                ++pos_;
            }
        }
        if (pos_ < input_.size() && input_[pos_] == '.') {
            ++pos_;
            if (pos_ >= input_.size()
                || !std::isdigit(static_cast<unsigned char>(input_[pos_]))) {
                return err_at("invalid fractional number");
            }
            while (pos_ < input_.size()
                && std::isdigit(static_cast<unsigned char>(input_[pos_]))) {
                ++pos_;
            }
        }
        if (pos_ < input_.size() && (input_[pos_] == 'e' || input_[pos_] == 'E')) {
            ++pos_;
            if (pos_ < input_.size() && (input_[pos_] == '+' || input_[pos_] == '-')) {
                ++pos_;
            }
            if (pos_ >= input_.size()
                || !std::isdigit(static_cast<unsigned char>(input_[pos_]))) {
                return err_at("invalid exponent");
            }
            while (pos_ < input_.size()
                && std::isdigit(static_cast<unsigned char>(input_[pos_]))) {
                ++pos_;
            }
        }

        const auto slice = input_.substr(start, pos_ - start);
        try {
            return value{.data = std::stod(std::string{slice})};
        } catch (...) {
            return err_at("number conversion failed");
        }
    }
};

[[nodiscard]] inline result<value> parse(std::string_view text) {
    return parser{text}.parse();
}

} // namespace leaf::json
