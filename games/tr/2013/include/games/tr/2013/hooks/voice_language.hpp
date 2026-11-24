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
    struct VoiceLanguageHook {};
} // namespace games::tr::tr2013

namespace hooks {
    template<>
    struct HookTraits<games::tr::tr2013::VoiceLanguageHook> {
        using Addrs        = games::game_data<games::tr::TR2013>::ResolvedAddresses;
        using PatternField = std::optional<std::uintptr_t> Addrs::*;

        static constexpr std::string_view name = "VoiceLanguage";

        using hard_deps = dep_list<>;
        using soft_deps = dep_list<>;

        static constexpr auto required_patterns = std::array<PatternField, 2> {
            &Addrs::vo_menu_apply,
            &Addrs::settings_getter_call,
        };
        static constexpr auto optional_patterns = std::array<PatternField, 0> {};

        using Config = empty_config;

        static auto install(const Addrs &addrs) -> bool;
    };
} // namespace hooks
