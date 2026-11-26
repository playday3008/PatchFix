#pragma once

#include <cstdint>

#include <algorithm>
#include <array>
#include <optional>
#include <string_view>
#include <tuple>

#include "core/hooks/registry/config_base.hpp"
#include "core/hooks/registry/dep_list.hpp"
#include "core/hooks/registry/hook_traits.hpp"
#include "core/hooks/registry/ini_field.hpp"

#include "games/tr/2013/game_data.hpp"

namespace games::tr::tr2013 {
    struct FrameLimiterHook {};

    // Caps below this turn the limiter off.
    inline constexpr float k_min_cap = 1.0F;

    // The game's timestep clamps each frame delta at 66666 us, so a frame longer
    // than 1/15 s runs the game in slow motion. Caps from k_min_cap up to this
    // value are raised to it.
    inline constexpr float k_floor_cap = 15.0F;

    // Clock ticks per frame for `cap` frames per second; 0 means off.
    [[nodiscard]] inline auto frame_period(float cap, std::int64_t ticks_per_second)
        -> std::int64_t {
        if (!(cap >= k_min_cap)) {
            return 0;
        }
        return static_cast<std::int64_t>(static_cast<double>(ticks_per_second) /
                                         std::max(cap, k_floor_cap));
    }

    // Schedules frame starts one period apart, in clock ticks. A frame that
    // arrives late starts at once without shifting the schedule, so the average
    // rate holds; one that is a full period or more behind (a load, an alt-tab)
    // restarts the schedule instead of letting the next frames run back to back.
    class FramePacer {
        std::int64_t next_ {0};
        std::int64_t period_ {0};

      public:
        // Ticks to wait from `now` before this frame may start; 0 means go now.
        // A period <= 0 disables pacing and forgets the schedule.
        [[nodiscard]] auto due(std::int64_t now, std::int64_t period) -> std::int64_t {
            if (period <= 0) {
                period_ = 0;
                return 0;
            }
            if (period != period_ || now - next_ >= period) {
                period_ = period;
                next_   = now + period;
                return 0;
            }
            const std::int64_t wait = std::max<std::int64_t>(next_ - now, 0);
            next_ += period;
            return wait;
        }
    };
} // namespace games::tr::tr2013

namespace hooks {
    template<>
    struct HookTraits<games::tr::tr2013::FrameLimiterHook> {
        using Addrs        = games::game_data<games::tr::TR2013>::ResolvedAddresses;
        using PatternField = std::optional<std::uintptr_t> Addrs::*;

        static constexpr std::string_view name = "FrameLimiter";

        using hard_deps = dep_list<>;
        using soft_deps = dep_list<>;

        static constexpr auto required_patterns = std::array<PatternField, 1> {
            &Addrs::frame_timestep,
        };
        static constexpr auto optional_patterns = std::array<PatternField, 0> {};

        struct Config : config_base<Config> {
            ini_field<float> cap {"FPS", "FrameCap", 0.0F};

            static constexpr std::size_t field_count = 1;
            static constexpr auto        field_ptrs  = std::tuple {&Config::cap};
        };

        static auto install(const Addrs &addrs) -> bool;
        static void on_reload(const Config &cfg);
    };
} // namespace hooks
