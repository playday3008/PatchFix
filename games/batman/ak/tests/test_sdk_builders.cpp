#include <cstdint>
#include <cstdlib>
#include <cwchar>

#include <array>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "games/batman/ak/sdk.hpp"

using games::batman::ak::sdk::make_fstring;
using games::batman::ak::sdk::make_string_array;

namespace {
    struct Allocation {
        std::uint32_t size;
        std::uint32_t align;
    };

    std::vector<Allocation> s_allocations;
    std::vector<void *>     s_blocks;

    auto fake_realloc(void *ptr, std::uint32_t size, std::uint32_t align) -> void * {
        REQUIRE(ptr == nullptr);
        s_allocations.push_back({size, align});
        void *block = std::malloc(size);
        s_blocks.push_back(block);
        return block;
    }

    struct Reset {
        Reset() { s_allocations.clear(); }
        ~Reset() {
            for (void *block : s_blocks) {
                std::free(block);
            }
            s_blocks.clear();
        }
        Reset(const Reset &)                     = delete;
        auto operator=(const Reset &) -> Reset & = delete;
    };
} // namespace

TEST_CASE("make_fstring copies and terminates into one game allocation", "[batman-ak][sdk]") {
    const Reset reset;
    FString     s = make_fstring(&fake_realloc, L"abc");

    REQUIRE(s_allocations.size() == 1);
    CHECK(s_allocations[0].size == 4 * sizeof(wchar_t));
    CHECK(s_allocations[0].align == 8);
    CHECK(s.length() == 4);
    CHECK(s.size() == 4);
    CHECK(std::wcscmp(s.c_str(), L"abc") == 0);
    CHECK(s.c_str()[3] == L'\0');
}

TEST_CASE("make_fstring of an empty string is a lone terminator", "[batman-ak][sdk]") {
    const Reset reset;
    FString     s = make_fstring(&fake_realloc, L"");

    REQUIRE(s_allocations.size() == 1);
    CHECK(s_allocations[0].size == sizeof(wchar_t));
    CHECK(s.length() == 1);
    CHECK(s.size() == 1);
    CHECK(s.c_str()[0] == L'\0');
}

TEST_CASE("make_string_array allocates the array and every element", "[batman-ak][sdk]") {
    const Reset                       reset;
    const std::array<std::wstring, 3> items {L"a", L"bc", L""};
    TArray<FString>                   arr = make_string_array(&fake_realloc, items);

    REQUIRE(arr.size() == 3);
    CHECK(arr.capacity() == 3);
    CHECK(s_allocations.size() == 4);
    CHECK(s_allocations[0].size == 3 * sizeof(FString));
    CHECK(std::wcscmp(arr.at(0).c_str(), L"a") == 0);
    CHECK(arr.at(0).length() == 2);
    CHECK(arr.at(0).size() == arr.at(0).length());
    CHECK(std::wcscmp(arr.at(1).c_str(), L"bc") == 0);
    CHECK(arr.at(1).length() == 3);
    CHECK(arr.at(1).size() == arr.at(1).length());
    CHECK(arr.at(2).length() == 1);
    CHECK(arr.at(2).size() == arr.at(2).length());
}

TEST_CASE("make_string_array of nothing allocates nothing", "[batman-ak][sdk]") {
    const Reset     reset;
    TArray<FString> arr = make_string_array(&fake_realloc, {});

    CHECK(s_allocations.empty());
    CHECK(arr.size() == 0);
    CHECK(arr.capacity() == 0);
    CHECK(arr.data() == nullptr);
}
