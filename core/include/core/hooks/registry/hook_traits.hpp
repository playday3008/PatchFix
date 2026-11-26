#pragma once

#include <concepts>
#include <string_view>
#include <utility>

#include <mini/ini.h>

namespace hooks {
    template<typename Tag>
    struct HookTraits {
        static_assert(sizeof(Tag) == 0, "HookTraits<Tag> must be specialized for each hook");
    };

    // hook_list requires this of every hook it names, so a specialization that
    // lacks a member fails at the game's AllHooks, with a note naming the member.
    template<typename Tag>
    concept ValidHookTraits = requires(mINI::INIStructure &ini) {
        { HookTraits<Tag>::name } -> std::convertible_to<std::string_view>;
        typename HookTraits<Tag>::hard_deps;
        typename HookTraits<Tag>::soft_deps;
        { HookTraits<Tag>::required_patterns };
        { HookTraits<Tag>::optional_patterns };
        typename HookTraits<Tag>::Addrs;
        {
            HookTraits<Tag>::install(std::declval<const typename HookTraits<Tag>::Addrs &>())
        } -> std::same_as<bool>;
        typename HookTraits<Tag>::Config;
        { std::declval<typename HookTraits<Tag>::Config &>().load_all(ini) } -> std::same_as<void>;
    };

    template<typename Tag>
    concept HasOnReload =
        requires(const HookTraits<Tag>::Config &cfg) { HookTraits<Tag>::on_reload(cfg); };

    // Undoes what the hook changed in the game when [Hooks] turns it off at
    // runtime; reload() calls it once per transition to disabled. Turning the
    // hook back on needs no counterpart, since on_reload runs for every enabled
    // hook on each reload.
    template<typename Tag>
    concept HasOnDisable = requires { HookTraits<Tag>::on_disable(); };
} // namespace hooks
