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

#include "games/batman/ak/game_data.hpp"
#include "games/batman/ak/hooks/process_event.hpp"
#include "games/batman/ak/path_field.hpp"

namespace games::batman::ak {
    struct DLCLoaderHook {};
} // namespace games::batman::ak

namespace hooks {
    template<>
    struct HookTraits<games::batman::ak::DLCLoaderHook> {
        using Addrs        = games::game_data<games::batman::ArkhamKnight>::ResolvedAddresses;
        using PatternField = std::optional<std::uintptr_t> Addrs::*;

        static constexpr std::string_view name = "DLCLoader";

        using hard_deps = dep_list<games::batman::ak::ProcessEventHook>;
        using soft_deps = dep_list<>;

        static constexpr auto required_patterns = std::array<PatternField, 0> {};
        static constexpr auto optional_patterns = std::array<PatternField, 0> {};

        struct Config : config_base<Config> {
            games::batman::ak::path_field custom_root {
                "DLCLoader", "CustomRoot", L"..\\..\\DLC\\Custom"};

            static constexpr std::size_t field_count = 1;
            static constexpr auto        field_ptrs  = std::tuple {&Config::custom_root};
        };

        static auto install(const Addrs &addrs) -> bool;
    };
} // namespace hooks
