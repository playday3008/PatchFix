#pragma once

#include <cctype>
#include <cmath>
#include <cstddef>

#include <algorithm>
#include <array>
#include <charconv>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

namespace hooks {
    namespace detail {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage"
        template<typename T>
        auto sv_from_chars(std::string_view sv, T &value) -> std::from_chars_result {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            return std::from_chars(sv.data(), sv.data() + sv.size(), value);
        }

        // True when from_chars consumed the whole view, not just a valid prefix.
        inline auto consumed_all(std::string_view sv, const char *end) -> bool {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            return end == sv.data() + sv.size();
        }
#pragma clang diagnostic pop

        inline auto trim(std::string_view sv) -> std::string_view {
            const auto is_space = [](char c) -> bool {
                return std::isspace(static_cast<unsigned char>(c)) != 0;
            };
            while (!sv.empty() && is_space(sv.front())) {
                sv.remove_prefix(1);
            }
            while (!sv.empty() && is_space(sv.back())) {
                sv.remove_suffix(1);
            }
            return sv;
        }

        // from_chars reports success on a partial parse and happily yields nan/inf,
        // both of which propagate into the camera and the frame pacer. Require the
        // whole (trimmed) string to be consumed and the result to be finite.
        inline auto parse_finite(std::string_view sv) -> std::optional<float> {
            sv = trim(sv);
            if (sv.empty()) {
                return std::nullopt;
            }
            float val      = 0.0F;
            auto [ptr, ec] = sv_from_chars(sv, val);
            if (ec != std::errc {} || !consumed_all(sv, ptr) || !std::isfinite(val)) {
                return std::nullopt;
            }
            return val;
        }

        inline auto ascii_iequal(std::string_view a, std::string_view b) -> bool {
            return std::ranges::equal(a, b, [](char x, char y) -> bool {
                return std::tolower(static_cast<unsigned char>(x)) ==
                       std::tolower(static_cast<unsigned char>(y));
            });
        }

        inline auto ascii_upper(char c) -> char {
            return static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        }

        // "0x01".."0xFE" (prefix already checked) as a virtual-key code.
        inline auto parse_hex_key(std::string_view digits) -> std::optional<int> {
            int v = 0;
            for (const char c : digits) {
                const char u = ascii_upper(c);
                if (u >= '0' && u <= '9') {
                    v = (v * 16) + (u - '0');
                } else if (u >= 'A' && u <= 'F') {
                    v = (v * 16) + (u - 'A' + 10);
                } else {
                    return std::nullopt;
                }
            }
            return v >= 0x01 && v <= 0xFE ? std::optional<int> {v} : std::nullopt;
        }

        // "1".."24" after the F of an F-key, as VK_F1..VK_F24.
        inline auto parse_function_key(std::string_view digits) -> std::optional<int> {
            int v          = 0;
            auto [ptr, ec] = sv_from_chars(digits, v);
            if (ec != std::errc {} || !consumed_all(digits, ptr) || v < 1 || v > 24) {
                return std::nullopt;
            }
            return 0x70 + v - 1;
        }

        // F1-F24, A-Z, 0-9 or a hex code 0x01-0xFE, case-insensitive.
        inline auto parse_virtual_key(std::string_view text) -> std::optional<int> {
            const auto s = trim(text);
            if (s.size() == 1) {
                const char c = ascii_upper(s.front());
                if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) {
                    return c;
                }
                return std::nullopt;
            }
            if (s.size() >= 3 && s.size() <= 4 && s.front() == '0' && ascii_upper(s.at(1)) == 'X') {
                return parse_hex_key(s.substr(2));
            }
            if (s.size() >= 2 && s.size() <= 3 && ascii_upper(s.front()) == 'F') {
                return parse_function_key(s.substr(1));
            }
            return std::nullopt;
        }

        template<typename E, std::size_t N>
        auto parse_enum(const std::string                                   &s,
                        const std::array<std::pair<std::string_view, E>, N> &table,
                        E                                                    fallback) -> E {
            for (const auto &[name, val] : table) {
                if (ascii_iequal(s, name)) {
                    return val;
                }
            }
            const std::string_view num = trim(s);
            int                    raw = 0;
            auto [ptr, ec]             = sv_from_chars(num, raw);
            if (ec == std::errc {} && consumed_all(num, ptr)) {
                if constexpr (requires { E::_count; }) {
                    if (raw < 0 || raw >= static_cast<int>(std::to_underlying(E::_count))) {
                        return fallback;
                    }
                }
                return static_cast<E>(raw);
            }
            return fallback;
        }
    } // namespace detail

    template<typename T>
    struct default_parser;

    template<>
    struct default_parser<float> {
        [[maybe_unused]] static auto operator()(const std::string &s) -> float {
            return detail::parse_finite(s).value_or(0.0F);
        }
    };

    template<>
    struct default_parser<bool> {
        [[maybe_unused]] static auto operator()(const std::string &s) -> bool {
            constexpr auto truthy = std::to_array<std::string_view>({
                "true",
                "yes",
                "on",
            });
            constexpr auto falsy  = std::to_array<std::string_view>({
                "false",
                "no",
                "off",
            });

            if (std::ranges::any_of(truthy, [&](std::string_view t) -> bool {
                    return detail::ascii_iequal(s, t);
                })) {
                return true;
            }
            if (std::ranges::any_of(falsy, [&](std::string_view f) -> bool {
                    return detail::ascii_iequal(s, f);
                })) {
                return false;
            }
            int val = 0;
            detail::sv_from_chars(s, val);
            return val != 0;
        }
    };

    struct ratio_parser {
        [[maybe_unused]] static auto operator()(const std::string &s) -> float {
            const std::string_view str(s);
            if (str.empty() || str == "0") {
                return 0.0F;
            }
            auto colon = str.find(':');
            if (colon != std::string_view::npos) {
                const auto w = detail::parse_finite(str.substr(0, colon));
                const auto h = detail::parse_finite(str.substr(colon + 1));
                if (!w || !h || *h <= 0.0F) {
                    return 0.0F;
                }
                return *w / *h;
            }
            return detail::parse_finite(str).value_or(0.0F);
        }
    };

    struct clamped_unit_parser {
        [[maybe_unused]] static auto operator()(const std::string &s) -> float {
            return std::clamp(default_parser<float> {}(s), 0.0F, 1.0F);
        }
    };

    // An integer in [Min, Max]; anything else, including trailing text, gives Default.
    template<int Min, int Max, int Default>
        requires(Min <= Default && Default <= Max)
    struct int_range_parser {
        [[maybe_unused]] static auto operator()(const std::string &s) -> int {
            const auto text = detail::trim(s);
            int        v    = 0;
            auto [ptr, ec]  = detail::sv_from_chars(text, v);
            if (ec != std::errc {} || !detail::consumed_all(text, ptr) || v < Min || v > Max) {
                return Default;
            }
            return v;
        }
    };

    // A finite float in [Min, Max]; anything else gives Default.
    template<float Min, float Max, float Default>
        requires(Min <= Default && Default <= Max)
    struct float_range_parser {
        [[maybe_unused]] static auto operator()(const std::string &s) -> float {
            const auto v = detail::parse_finite(s);
            return v && *v >= Min && *v <= Max ? *v : Default;
        }
    };

    // A Windows virtual-key code (see detail::parse_virtual_key); 0 when not recognised.
    struct virtual_key_parser {
        [[maybe_unused]] static auto operator()(const std::string &s) -> int {
            return detail::parse_virtual_key(s).value_or(0);
        }
    };
} // namespace hooks
