#pragma once

#include <cstdint>

#include <array>
#include <optional>
#include <string_view>

#include "games/game_data.hpp"

namespace games::tr {
    struct TR2013 {};
} // namespace games::tr

namespace games {
    template<>
    struct game_data<tr::TR2013> {
        static constexpr std::string_view name     = "Tomb Raider (2013)";
        static constexpr std::string_view exe_name = "TombRaider.exe";

        struct ResolvedAddresses {
            std::optional<std::uintptr_t> movie_layout_gate;
        };

        // clang-format off
        static constexpr auto scan_entries = std::to_array<ScanEntry<ResolvedAddresses>>({
            {.name="MOVIE_LAYOUT_GATE", .field=&ResolvedAddresses::movie_layout_gate, .offset=0x0A, .bytes="8B 87 CC 00 00 00 83 F8 02 0F 8D ? ? ? ? 8B 4F 48 40 89 87 CC 00 00 00"},
        });
        // clang-format on
    };

    static_assert(ValidGameData<tr::TR2013>);
} // namespace games
