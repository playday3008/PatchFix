#include "games/tr/2013/hooks/fov_scale.hpp"

#include <algorithm>
#include <numbers>
#include <string_view>
#include <utility>

#include "core/logger.hpp" // IWYU pragma: keep

#include "core/mem/hook.hpp"

#include "games/tr/2013/registry.hpp"

namespace hooks {
    namespace {
        using Tag = games::tr::tr2013::FovScaleHook;

        // Each gameplay camera controller computes its vertical FOV in radians
        // every frame and stores it to CameraState+0xA0, which the frustum and
        // projection read. Scaling the value at those stores widens the view
        // without touching camera profile data. Cutscenes run through
        // CameraBoneCameraController, which stores the FOV to the same field
        // through an instruction this hook does not patch, so their framing is
        // unchanged. The camera shake FOV pulse is added later, on top of the
        // scaled value.
        constexpr float k_deg_to_rad = std::numbers::pi_v<float> / 180.0F;
        constexpr float k_min_fov    = 1.0F * k_deg_to_rad;
        constexpr float k_max_fov    = 160.0F * k_deg_to_rad;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wglobal-constructors"
        mem::MidHook g_exploration_hook;
        mem::MidHook g_aim_hook;
        mem::MidHook g_force_hook;
        mem::MidHook g_anchored_hook;
#pragma clang diagnostic pop

        void scale(float &fov) {
            auto &registry = games::tr::tr2013::registry();
            if (!registry.enabled<Tag>()) {
                return;
            }
            const float multiplier = registry.config<Tag>().multiplier.get();
            if (multiplier <= 0.0F || multiplier == 1.0F) {
                return;
            }
            fov = std::clamp(fov * multiplier, k_min_fov, k_max_fov);
        }

        // Free third-person camera: exploring, climbing, traversal. FOV in xmm1.
        struct ScaleExploration {
            [[maybe_unused]] static constexpr std::string_view name = "FovScale";

            [[maybe_unused]] static void operator()(mem::Registers &regs) {
                scale(regs.xmm1.f32[0]);
            }
        };

        // Aiming a weapon. FOV in xmm0.
        struct ScaleAim {
            [[maybe_unused]] static constexpr std::string_view name = "FovScale";

            [[maybe_unused]] static void operator()(mem::Registers &regs) {
                scale(regs.xmm0.f32[0]);
            }
        };

        // Scripted camera that steers the view during gameplay. FOV in xmm0.
        struct ScaleForce {
            [[maybe_unused]] static constexpr std::string_view name = "FovScale";

            [[maybe_unused]] static void operator()(mem::Registers &regs) {
                scale(regs.xmm0.f32[0]);
            }
        };

        // Camera anchored to a point or path in a set piece. FOV in xmm0.
        struct ScaleAnchored {
            [[maybe_unused]] static constexpr std::string_view name = "FovScale";

            [[maybe_unused]] static void operator()(mem::Registers &regs) {
                scale(regs.xmm0.f32[0]);
            }
        };

        template<typename Functor>
        auto install_one(mem::MidHook &slot, std::uintptr_t addr) -> bool {
            auto result = mem::make_hook<Functor>(addr);
            if (!result) {
                log::get()->error("TR2013 FovScaleHook: 0x{:X}: {}", addr, result.error());
                return false;
            }
            slot = std::move(*result);
            return true;
        }
    } // namespace

    auto HookTraits<Tag>::install(const Addrs &addrs) -> bool {
        if (!install_one<ScaleExploration>(g_exploration_hook,
                                           addrs.fov_exploration_store.value()) ||
            !install_one<ScaleAim>(g_aim_hook, addrs.fov_aim_store.value()) ||
            !install_one<ScaleForce>(g_force_hook, addrs.fov_force_store.value()) ||
            !install_one<ScaleAnchored>(g_anchored_hook, addrs.fov_anchored_store.value())) {
            return false;
        }

        log::get()->info("TR2013 FovScaleHook: installed at 0x{:X}, 0x{:X}, 0x{:X}, 0x{:X}",
                         addrs.fov_exploration_store.value(),
                         addrs.fov_aim_store.value(),
                         addrs.fov_force_store.value(),
                         addrs.fov_anchored_store.value());
        return true;
    }
} // namespace hooks
