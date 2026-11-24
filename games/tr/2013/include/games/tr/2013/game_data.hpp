#pragma once

#include <array>
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

        struct ResolvedAddresses {};

        static constexpr std::array<ScanEntry<ResolvedAddresses>, 0> scan_entries {};
    };

    static_assert(ValidGameData<tr::TR2013>);
} // namespace games
