#pragma once

#include <cstdint>

#include <array>
#include <optional>
#include <string_view>

#include "games/game_data.hpp"

namespace games::batman {
    struct ArkhamKnight {};
} // namespace games::batman

namespace games {
    template<>
    struct game_data<batman::ArkhamKnight> {
        static constexpr std::string_view name     = "Arkham Knight";
        static constexpr std::string_view exe_name = "BatmanAK.exe";

        struct ResolvedAddresses {
            std::optional<std::uintptr_t> gobjects;
            std::optional<std::uintptr_t> gnames;
            std::optional<std::uintptr_t> app_realloc;
            std::optional<std::uintptr_t> process_event;
            std::optional<std::uintptr_t> reclaimer_wait;
            std::optional<std::uintptr_t> reclaimer_wait_alt;
        };

        // GOBJECTS and GNAMES are the generated SDK's own patterns. Each match is
        // the start of the pattern; sdk::init reads the RIP-relative operand.
        // RECLAIMER_WAIT* land on the `or edx, -1` (INFINITE) before the idle
        // wait of the two component-pool reclaimer threads.
        // clang-format off
        static constexpr auto scan_entries = std::to_array<ScanEntry<ResolvedAddresses>>({
            {.name="GOBJECTS",           .field=&ResolvedAddresses::gobjects,           .offset=0x00, .bytes="48 89 6C 24 18 56 48 83 EC 20 48 89 5C 24 30 48 8B F1 48 8D 2D ? ? ? ? 48 89 7C 24 38 66 90 FF 46 08 48 63 46 08 3B 05 ? ? ? ?"},
            {.name="GNAMES",             .field=&ResolvedAddresses::gnames,             .offset=0x00, .bytes="40 53 48 83 EC 20 48 63 01 48 8B 0D ? ? ? ? 48 8B DA 48 8B 0C C1"},
            {.name="APP_REALLOC",        .field=&ResolvedAddresses::app_realloc,        .offset=0x00, .bytes="4C 8B D1 48 8B 0D ? ? ? ? 45 8B C8 48 8B 01"},
            {.name="PROCESS_EVENT",      .field=&ResolvedAddresses::process_event,      .offset=0x00, .bytes="40 55 57 41 54 41 55 41 56 48 83 EC ? 48 8D 6C 24 ? 48 C7 45"},
            {.name="RECLAIMER_WAIT",     .field=&ResolvedAddresses::reclaimer_wait,     .offset=0x29, .bytes="48 89 83 E4 02 00 00 4C 89 25 ? ? ? ? 48 8B CE FF 15 ? ? ? ? EB 20 48 8B CB FF 15 ? ? ? ? 48 8B 4F 20 48 8B 01 83 CA FF FF 50 28"},
            {.name="RECLAIMER_WAIT_ALT", .field=&ResolvedAddresses::reclaimer_wait_alt, .offset=0x29, .bytes="48 89 83 64 03 00 00 4C 89 25 ? ? ? ? 48 8B CE FF 15 ? ? ? ? EB 20 48 8B CB FF 15 ? ? ? ? 48 8B 4F 20 48 8B 01 83 CA FF FF 50 28"},
        });
        // clang-format on
    };

    static_assert(ValidGameData<batman::ArkhamKnight>);
} // namespace games
