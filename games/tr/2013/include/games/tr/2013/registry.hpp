#pragma once

#include "core/hooks/registry/registry.hpp"

#include "games/tr/2013/hooks/camera_shake.hpp"
#include "games/tr/2013/hooks/movie_relayout.hpp"

namespace games::tr::tr2013 {
    using AllHooks = hooks::hook_list<MovieRelayoutHook, CameraShakeHook>;

    using TR2013Registry = hooks::Registry<AllHooks>;

    auto registry() -> TR2013Registry &;
} // namespace games::tr::tr2013
