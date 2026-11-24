#include "games/tr/2013/hooks/movie_relayout.hpp"

#include <cstdint>

#include "core/logger.hpp" // IWYU pragma: keep

#include "core/mem/write.hpp"

#include "games/tr/2013/registry.hpp"

namespace hooks {
    namespace {
        using Tag = games::tr::tr2013::MovieRelayoutHook;

        // Each Scaleform movie's per-frame layout update (stage viewport, aspect
        // offset, _root.UpdateScreenInfo) counts its runs and returns early from
        // the third on, so a movie created before the screen layout settles keeps
        // a stale placement. Build 743 ran it every frame.
        //
        // The gate is `cmp eax, 2; jge skip`. Turning the jge (0F 8D) into jo
        // (0F 80) is a single-byte write that is safe to flip while the render
        // thread runs; `cmp eax, 2` only overflows once the counter has wrapped,
        // after 2^31 frames.
        constexpr std::uint8_t k_op_jge = 0x8D;
        constexpr std::uint8_t k_op_jo  = 0x80;

        std::uintptr_t g_branch = 0;

        void apply_patch(bool every_frame) {
            if (g_branch == 0) {
                return;
            }

            if (!mem::write<std::uint8_t>(g_branch, every_frame ? k_op_jo : k_op_jge)) {
                log::get()->error("TR2013 MovieRelayoutHook: failed to write branch opcode");
                return;
            }

            log::get()->info("TR2013 MovieRelayoutHook: per-frame relayout {}",
                             every_frame ? "on" : "off");
        }
    } // namespace

    void HookTraits<Tag>::on_reload(const Config &cfg) {
        apply_patch(cfg.every_frame.get());
    }

    auto HookTraits<Tag>::install(const Addrs &addrs) -> bool {
        auto branch_addr = addrs.movie_layout_gate.value();

        // The address is the second opcode byte of the jge; refuse anything else.
        auto opcode = mem::read<std::uint8_t>(branch_addr);
        if (mem::read<std::uint8_t>(branch_addr - 1) != 0x0F || opcode != k_op_jge) {
            log::get()->error("TR2013 MovieRelayoutHook: expected jge at 0x{:X}, found 0x{:02X}",
                              branch_addr - 1,
                              opcode);
            return false;
        }

        g_branch = branch_addr;
        apply_patch(games::tr::tr2013::registry().config<Tag>().every_frame.get());

        log::get()->info("TR2013 MovieRelayoutHook: installed at 0x{:X}", branch_addr - 1);
        return true;
    }
} // namespace hooks
