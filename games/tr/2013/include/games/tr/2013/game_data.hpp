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
            std::optional<std::uintptr_t> cinematic_aspect_call;
            std::optional<std::uintptr_t> fov_exploration_store;
            std::optional<std::uintptr_t> fov_aim_store;
            std::optional<std::uintptr_t> fov_force_store;
            std::optional<std::uintptr_t> fov_anchored_store;
            std::optional<std::uintptr_t> frame_timestep;
            std::optional<std::uintptr_t> vo_menu_apply;
            std::optional<std::uintptr_t> settings_getter_call;
            std::optional<std::uintptr_t> vo_menu_row_lookup;
        };

        // clang-format off
        static constexpr auto scan_entries = std::to_array<ScanEntry<ResolvedAddresses>>({
            {.name="MOVIE_LAYOUT_GATE",       .field=&ResolvedAddresses::movie_layout_gate,       .offset=0x0A, .bytes="8B 87 CC 00 00 00 83 F8 02 0F 8D ? ? ? ? 8B 4F 48 40 89 87 CC 00 00 00"},
            {.name="SHAKE_FINALIZE_ROT_GATE", .field=&ResolvedAddresses::shake_finalize_rot_gate, .offset=0x09, .bytes="8B CF E8 ? ? ? ? 84 C0 74 1A 56 8D 86 90 00 00 00 8B CF 50"},
            {.name="SHAKE_FINALIZE_FOV_GATE", .field=&ResolvedAddresses::shake_finalize_fov_gate, .offset=0x09, .bytes="8B CF E8 ? ? ? ? 84 C0 74 13 8B CF E8 ? ? ? ? D8 86 A0 00 00 00"},
            {.name="SHAKE_BONE_ROT_GATE",     .field=&ResolvedAddresses::shake_bone_rot_gate,     .offset=0x0A, .bytes="80 BF C0 01 00 00 00 8B 73 0C 74 ? 33 C9 8D 86 C8 00 00 00 F6 00 01"},
            {.name="SHAKE_BONE_FOV_GATE",     .field=&ResolvedAddresses::shake_bone_fov_gate,     .offset=0x07, .bytes="80 BF C0 01 00 00 00 74 ? 33 C9 8D 86 C8 00 00 00 F6 00 01 75 ? 41 05 C0 00 00 00 83 F9 10 7C F0 EB ? 8B 4B 0C 8B B7 40 01 00 00 E8"},
            {.name="CINEMATIC_ASPECT_CALL",   .field=&ResolvedAddresses::cinematic_aspect_call,   .offset=0x0B, .bytes="8B 06 8B CE F2 0F 5A C0 8B 40 08 F3 0F 11 45 EC FF D0 F3 0F 10 45 EC F3 0F 11 45 EC D8 7D EC"},
            {.name="FOV_EXPLORATION_STORE",   .field=&ResolvedAddresses::fov_exploration_store,   .offset=0x0C, .bytes="F3 0F 59 8F 58 05 00 00 F3 0F 58 C8 F3 0F 11 88 A0 00 00 00"},
            {.name="FOV_AIM_STORE",           .field=&ResolvedAddresses::fov_aim_store,           .offset=0x0B, .bytes="8B 86 40 01 00 00 F3 0F 10 45 F4 F3 0F 11 80 A0 00 00 00 8B 86 40 01 00 00"},
            {.name="FOV_FORCE_STORE",         .field=&ResolvedAddresses::fov_force_store,         .offset=0x0B, .bytes="8B 86 40 01 00 00 F3 0F 10 45 C8 F3 0F 11 80 A0 00 00 00 0F 28 55 B0"},
            {.name="FOV_ANCHORED_STORE",      .field=&ResolvedAddresses::fov_anchored_store,      .offset=0x05, .bytes="F3 0F 10 45 FC F3 0F 11 80 A0 00 00 00 33 C0 38 86 78 01 00 00"},
            {.name="FRAME_TIMESTEP",          .field=&ResolvedAddresses::frame_timestep,          .offset=0x00, .bytes="55 8B EC 83 EC 10 F7 05 ? ? ? ? 00 00 00 10 56 57 0F 85 ? ? ? ? 53 E8"},
            // Epic (and possibly GOG) builds only: the Steam build takes its voice language from the Steam client and has no VOLanguage setting.
            {.name="VO_MENU_APPLY",           .field=&ResolvedAddresses::vo_menu_apply,           .offset=0x0F, .bytes="8B 9E F0 14 00 00 E8 ? ? ? ? 3B D8 74 ? 8B 86 EC 14 00 00 50 53 50 E8 ? ? ? ? 0F B7 86 F0 14 00 00"},
            {.name="SETTINGS_GETTER_CALL",    .field=&ResolvedAddresses::settings_getter_call,    .offset=0x07, .bytes="C7 45 FC FF FF FF FF E8 ? ? ? ? 8D 4D FC 51 8D 4D F8 8B 10 51 6A 00 68 ? ? ? ? 8B C8 FF 52 28 66 8B 45 F8"},
            {.name="VO_MENU_ROW_LOOKUP",      .field=&ResolvedAddresses::vo_menu_row_lookup,      .offset=0x06, .bytes="8B 97 F4 14 00 00 89 45 FC 8B 0A 3B CA 74 ? 8B 41 0C 3B 45 08 75 ? 83 FE 1D 74 ? 3B 45 FC 75 ? 83 FE 1E 74"},
        });
        // clang-format on
    };

    static_assert(ValidGameData<tr::TR2013>);
} // namespace games
