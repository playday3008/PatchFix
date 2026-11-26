#pragma once

#include <cstdint>

#include <array>
#include <optional>
#include <string_view>

#include "core/hooks/registry/config_base.hpp"
#include "core/hooks/registry/dep_list.hpp"
#include "core/hooks/registry/hook_traits.hpp"

#include "games/tr/2013/game_data.hpp"

namespace games::tr::tr2013 {
    struct SaveCompatibilityHook {};
} // namespace games::tr::tr2013

namespace hooks {
    template<>
    struct HookTraits<games::tr::tr2013::SaveCompatibilityHook> {
        using Addrs        = games::game_data<games::tr::TR2013>::ResolvedAddresses;
        using PatternField = std::optional<std::uintptr_t> Addrs::*;

        static constexpr std::string_view name = "SaveCompatibility";

        using hard_deps = dep_list<>;
        using soft_deps = dep_list<>;

        // Each build matches only its own half: the Steam build the Steam Cloud
        // read, non-Steam builds the local-save read.
        static constexpr auto required_patterns = std::array<PatternField, 0> {};
        static constexpr auto optional_patterns = std::array<PatternField, 2> {
            &Addrs::cloud_inflate_result,
            &Addrs::save_inflate_result,
        };

        using Config = empty_config;

        static auto install(const Addrs &addrs) -> bool;
    };
} // namespace hooks
