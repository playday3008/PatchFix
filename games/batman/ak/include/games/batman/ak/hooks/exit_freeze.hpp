#pragma once

#include <cstddef>
#include <cstdint>

#include <array>
#include <optional>
#include <string_view>
#include <tuple>

#include "core/hooks/registry/config_base.hpp"
#include "core/hooks/registry/dep_list.hpp"
#include "core/hooks/registry/hook_traits.hpp"
#include "core/hooks/registry/ini_field.hpp"

#include "games/batman/ak/game_data.hpp"

namespace games::batman::ak {
    struct ExitFreezeHook {};
} // namespace games::batman::ak

namespace hooks {
    // On exit the main thread joins the component-pool reclaimer threads, which
    // sleep in an INFINITE wait on an event. Under Wine/Proton the stop event can
    // be lost, leaving the game frozen. Bounding the wait makes the reclaimer
    // recheck its stop flag on its own.
    template<>
    struct HookTraits<games::batman::ak::ExitFreezeHook> {
        using Addrs        = games::game_data<games::batman::ArkhamKnight>::ResolvedAddresses;
        using PatternField = std::optional<std::uintptr_t> Addrs::*;

        static constexpr std::string_view name = "ExitFreeze";

        using hard_deps = dep_list<>;
        using soft_deps = dep_list<>;

        static constexpr auto required_patterns = std::array<PatternField, 2> {
            &Addrs::reclaimer_wait,
            &Addrs::reclaimer_wait_alt,
        };
        static constexpr auto optional_patterns = std::array<PatternField, 0> {};

        struct Config : config_base<Config> {
            ini_field<bool> workaround {"ExitFreeze", "Workaround", false};

            static constexpr std::size_t field_count = 1;
            static constexpr auto        field_ptrs  = std::tuple {&Config::workaround};
        };

        static auto install(const Addrs &addrs) -> bool;
    };
} // namespace hooks
