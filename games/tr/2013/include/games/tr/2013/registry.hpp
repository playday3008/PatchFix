#pragma once

#include "core/hooks/registry/registry.hpp"

#include "games/tr/2013/hooks/camera_shake.hpp"
#include "games/tr/2013/hooks/cinematic_aspect.hpp"
#include "games/tr/2013/hooks/fov_scale.hpp"
#include "games/tr/2013/hooks/frame_limiter.hpp"
#include "games/tr/2013/hooks/load_epic_saves.hpp"
#include "games/tr/2013/hooks/movie_relayout.hpp"
#include "games/tr/2013/hooks/voice_language.hpp"

namespace games::tr::tr2013 {
    using AllHooks = hooks::hook_list<MovieRelayoutHook,
                                      CameraShakeHook,
                                      CinematicAspectHook,
                                      VoiceLanguageHook,
                                      FovScaleHook,
                                      FrameLimiterHook,
                                      LoadEpicSavesHook>;

    using TR2013Registry = hooks::Registry<AllHooks>;

    auto registry() -> TR2013Registry &;
} // namespace games::tr::tr2013
