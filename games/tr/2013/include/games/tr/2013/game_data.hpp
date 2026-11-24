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
            std::optional<std::uintptr_t> shake_finalize_rot_gate;
            std::optional<std::uintptr_t> shake_finalize_fov_gate;
            std::optional<std::uintptr_t> shake_bone_rot_gate;
            std::optional<std::uintptr_t> shake_bone_fov_gate;
        };

        // clang-format off
        static constexpr auto scan_entries = std::to_array<ScanEntry<ResolvedAddresses>>({
            {.name="MOVIE_LAYOUT_GATE",       .field=&ResolvedAddresses::movie_layout_gate,       .offset=0x0A, .bytes="8B 87 CC 00 00 00 83 F8 02 0F 8D ? ? ? ? 8B 4F 48 40 89 87 CC 00 00 00"},
            {.name="SHAKE_FINALIZE_ROT_GATE", .field=&ResolvedAddresses::shake_finalize_rot_gate, .offset=0x09, .bytes="8B CF E8 ? ? ? ? 84 C0 74 1A 56 8D 86 90 00 00 00 8B CF 50"},
            {.name="SHAKE_FINALIZE_FOV_GATE", .field=&ResolvedAddresses::shake_finalize_fov_gate, .offset=0x09, .bytes="8B CF E8 ? ? ? ? 84 C0 74 13 8B CF E8 ? ? ? ? D8 86 A0 00 00 00"},
            {.name="SHAKE_BONE_ROT_GATE",     .field=&ResolvedAddresses::shake_bone_rot_gate,     .offset=0x0A, .bytes="80 BF C0 01 00 00 00 8B 73 0C 74 ? 33 C9 8D 86 C8 00 00 00 F6 00 01"},
            {.name="SHAKE_BONE_FOV_GATE",     .field=&ResolvedAddresses::shake_bone_fov_gate,     .offset=0x07, .bytes="80 BF C0 01 00 00 00 74 ? 33 C9 8D 86 C8 00 00 00 F6 00 01 75 ? 41 05 C0 00 00 00 83 F9 10 7C F0 EB ? 8B 4B 0C 8B B7 40 01 00 00 E8"},
        });
        // clang-format on
    };

    static_assert(ValidGameData<tr::TR2013>);
} // namespace games
