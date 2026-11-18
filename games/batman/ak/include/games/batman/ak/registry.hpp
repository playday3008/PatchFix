#pragma once

#include "core/hooks/registry/registry.hpp"

#include "games/batman/ak/hooks/dlc_loader.hpp"
#include "games/batman/ak/hooks/exit_freeze.hpp"
#include "games/batman/ak/hooks/process_event.hpp"

namespace games::batman::ak {
    using AllHooks = hooks::hook_list<ProcessEventHook, DLCLoaderHook, ExitFreezeHook>;

    using ArkhamKnightRegistry = hooks::Registry<AllHooks>;

    auto registry() -> ArkhamKnightRegistry &;
} // namespace games::batman::ak
