#include "games/tr/2013/hooks/frame_limiter.hpp"

#include <cstdint>

#include <string_view>
#include <utility>

#include <Windows.h>

#include "core/logger.hpp" // IWYU pragma: keep

#include "core/mem/hook.hpp"
#include "core/win32/timer_resolution.hpp"

#include "games/tr/2013/registry.hpp"

namespace hooks {
    namespace {
        using Tag = games::tr::tr2013::FrameLimiterHook;

        // The hooked function is the engine's per-frame timestep: it samples
        // QueryPerformanceCounter, averages the last six frame deltas into the
        // game's dt and has no frame cap of its own. Waiting at its entry,
        // before it samples the clock, makes the wait part of the next measured
        // delta, so game speed stays correct at any cap. At high frame rates
        // the Endurance boss fight QTE never shows its second prompt; a 60 FPS
        // cap avoids it.
        constexpr float k_min_cap = 1.0F;

        std::int64_t g_ticks_per_second = 0;

        // Only the game's main loop calls the timestep, so the pacer needs no
        // synchronization.
        games::tr::tr2013::FramePacer g_pacer;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wglobal-constructors"
        mem::MidHook g_hook;
#pragma clang diagnostic pop

        auto now_ticks() -> std::int64_t {
            LARGE_INTEGER counter {};
            QueryPerformanceCounter(&counter);
            return counter.QuadPart;
        }

        // Sleeps while at least 2 ms remain, then spins: Sleep(1) can overshoot
        // by up to a millisecond even at 1 ms timer resolution.
        void wait_until(std::int64_t target) {
            const std::int64_t ticks_per_ms = g_ticks_per_second / 1000;
            for (std::int64_t left = target - now_ticks(); left > 0; left = target - now_ticks()) {
                if (left >= 2 * ticks_per_ms) {
                    Sleep(static_cast<DWORD>((left / ticks_per_ms) - 1));
                } else {
                    YieldProcessor();
                }
            }
        }

        void apply_timer_resolution(float cap) {
            if (cap < k_min_cap) {
                win32::restore_timer_resolution();
            } else if (!win32::raise_timer_resolution()) {
                log::get()->warn("TR2013 FrameLimiterHook: failed to raise timer resolution");
            }
        }

        struct LimitFrame {
            [[maybe_unused]] static constexpr std::string_view name = "FrameLimiter";

            [[maybe_unused]] static void operator()(mem::Registers & /*regs*/) {
                auto       &registry = games::tr::tr2013::registry();
                const float cap = registry.enabled<Tag>() ? registry.config<Tag>().cap.get() : 0.0F;
                const auto  period =
                    cap < k_min_cap
                        ? std::int64_t {0}
                        : static_cast<std::int64_t>(static_cast<double>(g_ticks_per_second) / cap);

                const std::int64_t now  = now_ticks();
                const std::int64_t wait = g_pacer.due(now, period);
                if (wait > 0) {
                    wait_until(now + wait);
                }
            }
        };
    } // namespace

    auto HookTraits<Tag>::install(const Addrs &addrs) -> bool {
        LARGE_INTEGER frequency {};
        QueryPerformanceFrequency(&frequency);
        g_ticks_per_second = frequency.QuadPart;

        auto addr   = addrs.frame_timestep.value();
        auto result = mem::make_hook<LimitFrame>(addr);
        if (!result) {
            log::get()->error("TR2013 FrameLimiterHook: {}", result.error());
            return false;
        }
        g_hook = std::move(*result);

        apply_timer_resolution(games::tr::tr2013::registry().config<Tag>().cap.get());

        log::get()->info("TR2013 FrameLimiterHook: installed at 0x{:X}", addr);
        return true;
    }

    void HookTraits<Tag>::on_reload(const Config &cfg) {
        apply_timer_resolution(cfg.cap.get());
    }
} // namespace hooks
