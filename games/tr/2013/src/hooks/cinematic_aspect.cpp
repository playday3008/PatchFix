#include "games/tr/2013/hooks/cinematic_aspect.hpp"

#include <atomic>
#include <string_view>
#include <utility>

#include "core/logger.hpp" // IWYU pragma: keep

#include "core/mem/hook.hpp"

#include "games/tr/2013/registry.hpp"

namespace hooks {
    namespace {
        using Tag = games::tr::tr2013::CinematicAspectHook;

        // The cinematic camera turns its horizontal FOV into a vertical one with
        // the shell's aspect ratio. In a window the shell reports 16:9 and the
        // renderer widens the frustum to the real window, which is Hor+. In
        // exclusive fullscreen it reports the monitor's own aspect, so on an
        // ultrawide the vertical FOV shrinks and cutscenes lose their top and
        // bottom. Capping that one read at 16:9 gives the windowed result.
        constexpr float k_max_aspect = 16.0F / 9.0F;

        // PCGameShell::GetAspectRatio, taken from the vtable at the call site.
        using GetAspectRatio = float(__fastcall *)(void *self, void *edx);
        std::atomic<GetAspectRatio> g_original {nullptr};

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wglobal-constructors"
        mem::MidHook g_hook;
#pragma clang diagnostic pop

        // __fastcall with an unused edx matches the __thiscall call site: this in
        // ecx, no stack arguments, float returned in st(0).
        auto __fastcall clamped_aspect(void *self, void *edx) -> float {
            auto aspect = g_original.load(std::memory_order_relaxed)(self, edx);
            return aspect > k_max_aspect ? k_max_aspect : aspect;
        }

        // Runs just before `call eax`, with eax holding the vtable entry.
        struct SwapAspectCall {
            [[maybe_unused]] static constexpr std::string_view name = "CinematicAspect";

            [[maybe_unused]] static void operator()(mem::Registers &regs) {
                auto &registry = games::tr::tr2013::registry();
                if (!registry.enabled<Tag>() || !registry.config<Tag>().clamp.get()) {
                    return;
                }
                g_original.store(reinterpret_cast<GetAspectRatio>(regs.eax),
                                 std::memory_order_relaxed);
                regs.eax = reinterpret_cast<std::uintptr_t>(&clamped_aspect);
            }
        };
    } // namespace

    auto HookTraits<Tag>::install(const Addrs &addrs) -> bool {
        auto addr   = addrs.cinematic_aspect_call.value();
        auto result = mem::make_hook<SwapAspectCall>(addr);
        if (!result) {
            log::get()->error("TR2013 CinematicAspectHook: {}", result.error());
            return false;
        }
        g_hook = std::move(*result);

        log::get()->info("TR2013 CinematicAspectHook: installed at 0x{:X}", addr);
        return true;
    }
} // namespace hooks
