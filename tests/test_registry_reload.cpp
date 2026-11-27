#include <cstdint>

#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include <mini/ini.h>

#include "core/hooks/registry/config_base.hpp"
#include "core/hooks/registry/dep_list.hpp"
#include "core/hooks/registry/hook_traits.hpp"
#include "core/hooks/registry/registry.hpp"

namespace {
    struct MockAddrs {
        std::optional<std::uintptr_t> addr;
    };

    using MockField = std::optional<std::uintptr_t> MockAddrs::*;

    struct BaseHook {};
    struct DependentHook {};
    struct PlainHook {};

    struct Calls {
        int reload  = 0;
        int disable = 0;
    };

    Calls g_base;
    Calls g_dependent;

    // Names of the hooks whose on_disable ran, in call order.
    std::vector<std::string> g_disable_order;

    void reset_calls() {
        g_base      = {};
        g_dependent = {};
        g_disable_order.clear();
    }

    template<typename HardDeps>
    struct MockTraits {
        using Addrs     = MockAddrs;
        using Config    = hooks::empty_config;
        using hard_deps = HardDeps;
        using soft_deps = hooks::dep_list<>;

        static constexpr auto required_patterns = std::array<MockField, 0> {};
        static constexpr auto optional_patterns = std::array<MockField, 0> {};

        static auto install(const Addrs & /*addrs*/) -> bool { return true; }
    };
} // namespace

namespace hooks {
    template<>
    struct HookTraits<BaseHook> : MockTraits<dep_list<>> {
        static constexpr std::string_view name = "Base";

        static void on_reload(const Config & /*cfg*/) { ++g_base.reload; }
        static void on_disable() {
            ++g_base.disable;
            g_disable_order.emplace_back(name);
        }
    };

    template<>
    struct HookTraits<DependentHook> : MockTraits<dep_list<BaseHook>> {
        static constexpr std::string_view name = "Dependent";

        static void on_reload(const Config & /*cfg*/) { ++g_dependent.reload; }
        static void on_disable() {
            ++g_dependent.disable;
            g_disable_order.emplace_back(name);
        }
    };

    // Has neither callback; reload must still compile and run for it.
    template<>
    struct HookTraits<PlainHook> : MockTraits<dep_list<>> {
        static constexpr std::string_view name = "Plain";
    };
} // namespace hooks

namespace {
    using MockRegistry = hooks::Registry<hooks::hook_list<BaseHook, DependentHook, PlainHook>>;

    // The registry holds atomics, so it is set up in place rather than returned.
    void mark_installed(MockRegistry &registry) {
        registry.set_installed<BaseHook>(true);
        registry.set_installed<DependentHook>(true);
        registry.set_installed<PlainHook>(true);
    }

    auto ini_with(std::string_view hook, std::string_view value) -> mINI::INIStructure {
        mINI::INIStructure ini;
        ini["Hooks"][std::string(hook)] = std::string(value);
        return ini;
    }
} // namespace

TEST_CASE("reload calls on_disable once when an installed hook is turned off",
          "[registry][reload]") {
    reset_calls();
    MockRegistry registry;
    mark_installed(registry);
    auto off = ini_with("Base", "false");

    registry.reload(off);
    CHECK(g_base.disable == 1);
    CHECK(g_base.reload == 0);

    registry.reload(off);
    CHECK(g_base.disable == 1);
}

TEST_CASE("reload calls on_reload, not on_disable, when a hook is turned back on",
          "[registry][reload]") {
    reset_calls();
    MockRegistry registry;
    mark_installed(registry);
    auto off = ini_with("Base", "false");
    auto on  = ini_with("Base", "true");

    registry.reload(off);
    registry.reload(on);
    CHECK(g_base.disable == 1);
    CHECK(g_base.reload == 1);
}

TEST_CASE("reload skips on_disable for a hook that never installed", "[registry][reload]") {
    reset_calls();
    MockRegistry registry;
    auto         off = ini_with("Base", "false");

    registry.reload(off);
    CHECK(g_base.disable == 0);
}

TEST_CASE("reload calls on_disable for a hook disabled through its hard dependency",
          "[registry][reload]") {
    reset_calls();
    MockRegistry registry;
    mark_installed(registry);
    auto off = ini_with("Base", "false");

    registry.reload(off);
    CHECK(g_dependent.disable == 1);
    CHECK(g_dependent.reload == 0);
}

TEST_CASE("reload leaves other hooks alone when one is turned off", "[registry][reload]") {
    reset_calls();
    MockRegistry registry;
    mark_installed(registry);
    auto off = ini_with("Plain", "false");

    registry.reload(off);
    CHECK(g_base.disable == 0);
    CHECK(g_base.reload == 1);
    CHECK_FALSE(registry.enabled<PlainHook>());
}

TEST_CASE("reload undoes a hook before the hooks it depends on", "[registry][reload]") {
    reset_calls();
    MockRegistry registry;
    mark_installed(registry);
    auto off = ini_with("Base", "false");

    registry.reload(off);
    CHECK(g_disable_order == std::vector<std::string> {"Dependent", "Base"});
}
