#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "core/hooks/registry/parsers.hpp"

using namespace hooks;

TEST_CASE("default_parser<bool> truthy strings", "[parsers][bool]") {
    CHECK(default_parser<bool> {}("true") == true);
    CHECK(default_parser<bool> {}("True") == true);
    CHECK(default_parser<bool> {}("TRUE") == true);
    CHECK(default_parser<bool> {}("yes") == true);
    CHECK(default_parser<bool> {}("on") == true);
}

TEST_CASE("default_parser<bool> falsy strings", "[parsers][bool]") {
    CHECK(default_parser<bool> {}("false") == false);
    CHECK(default_parser<bool> {}("False") == false);
    CHECK(default_parser<bool> {}("FALSE") == false);
    CHECK(default_parser<bool> {}("no") == false);
    CHECK(default_parser<bool> {}("off") == false);
}

TEST_CASE("default_parser<bool> integer strings", "[parsers][bool]") {
    CHECK(default_parser<bool> {}("1") == true);
    CHECK(default_parser<bool> {}("0") == false);
    CHECK(default_parser<bool> {}("42") == true);
    CHECK(default_parser<bool> {}("-1") == true);
}

TEST_CASE("default_parser<bool> empty/invalid returns false", "[parsers][bool]") {
    CHECK(default_parser<bool> {}("") == false);
    CHECK(default_parser<bool> {}("garbage") == false);
}

TEST_CASE("default_parser<float> valid values", "[parsers][float]") {
    CHECK(default_parser<float> {}("1.5") == Catch::Approx(1.5F));
    CHECK(default_parser<float> {}("0") == Catch::Approx(0.0F));
    CHECK(default_parser<float> {}("-3.14") == Catch::Approx(-3.14F));
}

TEST_CASE("default_parser<float> empty/invalid returns 0", "[parsers][float]") {
    CHECK(default_parser<float> {}("") == Catch::Approx(0.0F));
    CHECK(default_parser<float> {}("abc") == Catch::Approx(0.0F));
}

// from_chars accepts "nan" and "inf". Both reach the FOV multiplier and the frame
// pacer unfiltered, where NaN survives std::max and every ordered comparison.
TEST_CASE("default_parser<float> rejects non-finite input", "[parsers][float]") {
    CHECK(default_parser<float> {}("nan") == Catch::Approx(0.0F));
    CHECK(default_parser<float> {}("NaN") == Catch::Approx(0.0F));
    CHECK(default_parser<float> {}("inf") == Catch::Approx(0.0F));
    CHECK(default_parser<float> {}("-inf") == Catch::Approx(0.0F));
    CHECK(default_parser<float> {}("infinity") == Catch::Approx(0.0F));
}

// A partial parse silently drops the rest: "1,5" became 1 and "60fps" became 60,
// so a typo changed the setting instead of falling back.
TEST_CASE("default_parser<float> rejects trailing garbage", "[parsers][float]") {
    CHECK(default_parser<float> {}("1,5") == Catch::Approx(0.0F));
    CHECK(default_parser<float> {}("60fps") == Catch::Approx(0.0F));
    CHECK(default_parser<float> {}("1.5x") == Catch::Approx(0.0F));
}

TEST_CASE("default_parser<float> tolerates surrounding whitespace", "[parsers][float]") {
    CHECK(default_parser<float> {}(" 1.5") == Catch::Approx(1.5F));
    CHECK(default_parser<float> {}("1.5 ") == Catch::Approx(1.5F));
    CHECK(default_parser<float> {}("  -2.5\t") == Catch::Approx(-2.5F));
}

TEST_CASE("ratio_parser colon format", "[parsers][ratio]") {
    float result = ratio_parser {}("16:9");
    CHECK(result == Catch::Approx(16.0F / 9.0F));
}

TEST_CASE("ratio_parser zero denominator", "[parsers][ratio]") {
    CHECK(ratio_parser {}("16:0") == Catch::Approx(0.0F));
}

TEST_CASE("ratio_parser plain float", "[parsers][ratio]") {
    CHECK(ratio_parser {}("1.777") == Catch::Approx(1.777F));
}

TEST_CASE("ratio_parser empty and zero", "[parsers][ratio]") {
    CHECK(ratio_parser {}("") == Catch::Approx(0.0F));
    CHECK(ratio_parser {}("0") == Catch::Approx(0.0F));
}

TEST_CASE("clamped_unit_parser clamps to [0, 1]", "[parsers][clamped]") {
    CHECK(clamped_unit_parser {}("0.5") == Catch::Approx(0.5F));
    CHECK(clamped_unit_parser {}("-1.0") == Catch::Approx(0.0F));
    CHECK(clamped_unit_parser {}("2.0") == Catch::Approx(1.0F));
    CHECK(clamped_unit_parser {}("0.0") == Catch::Approx(0.0F));
    CHECK(clamped_unit_parser {}("1.0") == Catch::Approx(1.0F));
}

TEST_CASE("clamped_unit_parser rejects non-finite input", "[parsers][clamped]") {
    CHECK(clamped_unit_parser {}("nan") == Catch::Approx(0.0F));
    CHECK(clamped_unit_parser {}("inf") == Catch::Approx(0.0F));
    CHECK(clamped_unit_parser {}("-inf") == Catch::Approx(0.0F));
}

TEST_CASE("ratio_parser rejects malformed components", "[parsers][ratio]") {
    CHECK(ratio_parser {}("16:9x") == Catch::Approx(0.0F));
    CHECK(ratio_parser {}("abc:9") == Catch::Approx(0.0F));
    CHECK(ratio_parser {}("nan") == Catch::Approx(0.0F));
    CHECK(ratio_parser {}("2.333junk") == Catch::Approx(0.0F));
}

TEST_CASE("ratio_parser tolerates spaces around the colon", "[parsers][ratio]") {
    CHECK(ratio_parser {}("21 : 9") == Catch::Approx(21.0F / 9.0F));
}

namespace {
    // NOLINTNEXTLINE(readability-enum-initial-value)
    enum class Color : std::uint8_t {
        Red   = 0,
        Green = 1,
        Blue  = 2,
        _count,
    };
} // namespace

TEST_CASE("parse_enum string match", "[parsers][enum]") {
    constexpr auto table = std::to_array<std::pair<std::string_view, Color>>({
        {"red", Color::Red},
        {"green", Color::Green},
        {"blue", Color::Blue},
    });
    CHECK(detail::parse_enum<Color>(std::string("red"), table, Color::Red) == Color::Red);
    CHECK(detail::parse_enum<Color>(std::string("GREEN"), table, Color::Red) == Color::Green);
    CHECK(detail::parse_enum<Color>(std::string("Blue"), table, Color::Red) == Color::Blue);
}

TEST_CASE("parse_enum integer path with _count bounds", "[parsers][enum]") {
    constexpr auto table = std::to_array<std::pair<std::string_view, Color>>({
        {"red", Color::Red},
        {"green", Color::Green},
        {"blue", Color::Blue},
    });
    CHECK(detail::parse_enum<Color>(std::string("0"), table, Color::Red) == Color::Red);
    CHECK(detail::parse_enum<Color>(std::string("2"), table, Color::Red) == Color::Blue);
    CHECK(detail::parse_enum<Color>(std::string("99"), table, Color::Red) == Color::Red);
    CHECK(detail::parse_enum<Color>(std::string("-1"), table, Color::Red) == Color::Red);
}

// "1abc" parsed as enumerator 1 because only the error code was consulted.
TEST_CASE("parse_enum rejects trailing garbage on the integer path", "[parsers][enum]") {
    constexpr auto table = std::to_array<std::pair<std::string_view, Color>>({
        {"red", Color::Red},
        {"green", Color::Green},
        {"blue", Color::Blue},
    });
    CHECK(detail::parse_enum<Color>(std::string("1abc"), table, Color::Blue) == Color::Blue);
    CHECK(detail::parse_enum<Color>(std::string("2 3"), table, Color::Blue) == Color::Blue);
}

TEST_CASE("parse_enum invalid string returns fallback", "[parsers][enum]") {
    constexpr auto table = std::to_array<std::pair<std::string_view, Color>>({
        {"red", Color::Red},
    });
    CHECK(detail::parse_enum<Color>(std::string("purple"), table, Color::Blue) == Color::Blue);
}

namespace {
    enum class Sparse : std::uint8_t {
        A = 2,
        B = 5,
    };
} // namespace

TEST_CASE("parse_enum without _count allows any integer", "[parsers][enum]") {
    constexpr auto table = std::to_array<std::pair<std::string_view, Sparse>>({
        {"a", Sparse::A},
        {"b", Sparse::B},
    });
    CHECK(detail::parse_enum<Sparse>(std::string("99"), table, Sparse::A) ==
          static_cast<Sparse>(99));
    CHECK(detail::parse_enum<Sparse>(std::string("b"), table, Sparse::A) == Sparse::B);
}

TEST_CASE("int_range_parser keeps values in range", "[parsers][range]") {
    using p = int_range_parser<0, 16, 4>;
    CHECK(p {}("8") == 8);
    CHECK(p {}(" 0 ") == 0);
    CHECK(p {}("16") == 16);
    CHECK(p {}("17") == 4);
    CHECK(p {}("-1") == 4);
    CHECK(p {}("3x") == 4);
    CHECK(p {}("") == 4);
}

TEST_CASE("float_range_parser keeps finite values in range", "[parsers][range]") {
    using p = float_range_parser<0.05F, 1.0F, 0.5F>;
    CHECK(p {}("0.25") == Catch::Approx(0.25F));
    CHECK(p {}("1") == Catch::Approx(1.0F));
    CHECK(p {}("0.05") == Catch::Approx(0.05F));
    CHECK(p {}("0.01") == Catch::Approx(0.5F));
    CHECK(p {}("nan") == Catch::Approx(0.5F));
    CHECK(p {}("x") == Catch::Approx(0.5F));
}

TEST_CASE("parse_virtual_key accepts F-keys, letters, digits and hex", "[parsers][vk]") {
    CHECK(detail::parse_virtual_key("F1") == 0x70);
    CHECK(detail::parse_virtual_key("f24") == 0x87);
    CHECK(detail::parse_virtual_key("F25") == std::nullopt);
    CHECK(detail::parse_virtual_key("F0") == std::nullopt);
    CHECK(detail::parse_virtual_key("a") == 'A');
    CHECK(detail::parse_virtual_key("7") == '7');
    CHECK(detail::parse_virtual_key(" F6 ") == 0x75);
    CHECK(detail::parse_virtual_key("0x01") == 0x01);
    CHECK(detail::parse_virtual_key("0XfE") == 0xFE);
    CHECK(detail::parse_virtual_key("0x00") == std::nullopt);
    CHECK(detail::parse_virtual_key("0xFF") == std::nullopt);
    CHECK(detail::parse_virtual_key("") == std::nullopt);
    CHECK(detail::parse_virtual_key("Space") == std::nullopt);
}

TEST_CASE("virtual_key_parser gives 0 for an unknown key", "[parsers][vk]") {
    CHECK(virtual_key_parser {}("nope") == 0);
    CHECK(virtual_key_parser {}("F6") == 0x75);
}
