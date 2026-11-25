#include "games/tr/2013/hooks/camera_shake.hpp"

#include <cstdint>

#include <array>

#include "core/logger.hpp" // IWYU pragma: keep

#include "core/mem/write.hpp"

#include "games/tr/2013/registry.hpp"

namespace hooks {
    namespace {
        using Tag = games::tr::tr2013::CameraShakeHook;

        // The camera's shake manager feeds two effects into the camera state: a
        // rotation/position offset and an FOV pulse added to the projection FOV.
        // Each is applied from two places, the regular camera finalize and the
        // cutscene bone camera, behind a `jz` that skips it when no shake is
        // active. Turning all four into `jmp` takes the no-shake path always.
        // A single-byte write, so it is safe to flip while the camera runs.
        constexpr std::uint8_t k_op_jz  = 0x74;
        constexpr std::uint8_t k_op_jmp = 0xEB;

        std::array<std::uintptr_t, 4> g_gates {};
        bool                          g_installed = false;

        void apply_patch(bool disable) {
            if (!g_installed) {
                return;
            }

            for (auto gate : g_gates) {
                if (!mem::write<std::uint8_t>(gate, disable ? k_op_jmp : k_op_jz)) {
                    log::get()->error("TR2013 CameraShakeHook: failed to write branch at 0x{:X}",
                                      gate);
                    return;
                }
            }

            log::get()->info("TR2013 CameraShakeHook: shake {}", disable ? "off" : "on");
        }
    } // namespace

    void HookTraits<Tag>::on_reload(const Config &cfg) {
        apply_patch(cfg.disable.get());
    }

    auto HookTraits<Tag>::install(const Addrs &addrs) -> bool {
        g_gates = {
            addrs.shake_finalize_rot_gate.value(),
            addrs.shake_finalize_fov_gate.value(),
            addrs.shake_bone_rot_gate.value(),
            addrs.shake_bone_fov_gate.value(),
        };

        for (auto gate : g_gates) {
            auto opcode = mem::read<std::uint8_t>(gate);
            if (opcode != k_op_jz) {
                log::get()->error("TR2013 CameraShakeHook: expected jz at 0x{:X}, found 0x{:02X}",
                                  gate,
                                  opcode);
                return false;
            }
        }

        g_installed = true;
        apply_patch(games::tr::tr2013::registry().config<Tag>().disable.get());

        log::get()->info("TR2013 CameraShakeHook: installed");
        return true;
    }
} // namespace hooks
