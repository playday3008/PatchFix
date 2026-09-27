#pragma once

#include "core/hooks/registry/registry.hpp"

#include "games/batman/ak/hooks/process_event.hpp"

namespace games::batman::ak {
    using AllHooks = hooks::hook_list<ProcessEventHook>;

    using ArkhamKnightRegistry = hooks::Registry<AllHooks>;

    auto registry() -> ArkhamKnightRegistry &;
} // namespace games::batman::ak
