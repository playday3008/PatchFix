#pragma once

#include <cstdint>

#include <array>
#include <optional>
#include <string_view>
#include <tuple>

#include "core/hooks/registry/config_base.hpp"
#include "core/hooks/registry/dep_list.hpp"
#include "core/hooks/registry/hook_traits.hpp"
#include "core/hooks/registry/ini_field.hpp"

#include "games/tr/2013/game_data.hpp"

namespace games::tr::tr2013 {
    struct CinematicAspectHook {};
} // namespace games::tr::tr2013

namespace hooks {
    template<>
    struct HookTraits<games::tr::tr2013::CinematicAspectHook> {
        using Addrs        = games::game_data<games::tr::TR2013>::ResolvedAddresses;
        using PatternField = std::optional<std::uintptr_t> Addrs::*;

        static constexpr std::string_view name = "CinematicAspect";

        using hard_deps = dep_list<>;
        using soft_deps = dep_list<>;

        static constexpr auto required_patterns = std::array<PatternField, 1> {
            &Addrs::cinematic_aspect_call,
        };
        static constexpr auto optional_patterns = std::array<PatternField, 0> {};

        struct Config : config_base<Config> {
            ini_field<bool> clamp {"Camera", "ClampCinematicAspect", true};

            static constexpr std::size_t field_count = 1;
            static constexpr auto        field_ptrs  = std::tuple {&Config::clamp};
        };

        static auto install(const Addrs &addrs) -> bool;
    };
} // namespace hooks
