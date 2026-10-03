#pragma once

#include <cstdint>

#include <array>
#include <optional>
#include <string_view>

#include "core/hooks/registry/config_base.hpp"
#include "core/hooks/registry/dep_list.hpp"
#include "core/hooks/registry/hook_traits.hpp"

#include "games/batman/ak/game_data.hpp"
#include "games/batman/ak/subscribers.hpp"

namespace games::batman::ak {
    struct ProcessEventHook {};

    // Adds a handler to the ProcessEvent hook. Call from a hook's install that
    // lists ProcessEventHook in its hard_deps, as the last step, after
    // everything else in install that can fail: there is no unsubscribe.
    // A runtime [Hooks] toggle does not remove the handler either, so it
    // must check registry().enabled<Tag>() for its own hook tag itself.
    auto subscribe(Handler handler) -> void;
} // namespace games::batman::ak

namespace hooks {
    template<>
    struct HookTraits<games::batman::ak::ProcessEventHook> {
        using Addrs        = games::game_data<games::batman::ArkhamKnight>::ResolvedAddresses;
        using PatternField = std::optional<std::uintptr_t> Addrs::*;

        static constexpr std::string_view name = "ProcessEvent";

        using hard_deps = dep_list<>;
        using soft_deps = dep_list<>;

        static constexpr auto required_patterns = std::array<PatternField, 4> {
            &Addrs::gobjects,
            &Addrs::gnames,
            &Addrs::app_realloc,
            &Addrs::process_event,
        };
        static constexpr auto optional_patterns = std::array<PatternField, 0> {};

        using Config = empty_config;

        static auto install(const Addrs &addrs) -> bool;
    };
} // namespace hooks
