#include "games/tr/2013/hooks/multiplayer_mouse.hpp"

#include <cstdint>

#include <string_view>
#include <utility>

#include "core/logger.hpp" // IWYU pragma: keep

#include "core/mem/hook.hpp"
#include "core/mem/write.hpp"

#include "games/tr/2013/registry.hpp"

namespace hooks {
    namespace {
        using Tag = games::tr::tr2013::MultiplayerMouseHook;

        // The UI highlight (the _root.UIHighlight movie) switches the mouse to
        // cursor mode and locks it there while it shows; it is the only caller
        // that takes the lock. Only its fade-out releases the lock and restores
        // the mode it saved. Removing a highlight entry while it shows, which the
        // multiplayer menus do, releases the movie without fading out, so the
        // lock stays: every later switch back to mouselook is only queued, the
        // cursor stays visible and the camera does not turn. Changing display
        // settings rebuilds the UI, which runs the fade-out, so that was the
        // known workaround.
        //
        // The hook runs where the highlight releases its movie. A lock still held
        // there is the highlight's own, so it does what the fade-out does.

        using SetModeFn = void(__cdecl *)(std::int32_t mode, bool lock);
        using UnlockFn  = bool(__cdecl *)();

        // The setter starts `push ebp; mov ebp, esp; cmp byte ptr [lock], 0`;
        // the lock flag address is the cmp operand.
        constexpr std::uintptr_t k_lock_operand_offset = 5;
        // UIHighlight member: mouse mode saved when it took the lock.
        constexpr std::uintptr_t k_saved_mode_offset = 0x30;

        SetModeFn      g_set_mode  = nullptr;
        UnlockFn       g_unlock    = nullptr;
        std::uintptr_t g_lock_flag = 0;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wglobal-constructors"
        mem::MidHook g_hook;
#pragma clang diagnostic pop

        // Highlight release (UIHighlight vtable slot 8), at entry with the
        // highlight in ecx.
        struct ReleaseLock {
            [[maybe_unused]] static constexpr std::string_view name = "MultiplayerMouse";

            [[maybe_unused]] static void operator()(mem::Registers &regs) {
                if (!games::tr::tr2013::registry().enabled<Tag>() ||
                    mem::read<std::uint8_t>(g_lock_flag) == 0) {
                    return;
                }
                // Same as the fade-out: apply the mode queued while locked, or
                // restore the saved one when nothing was queued.
                if (!g_unlock()) {
                    g_set_mode(mem::read<std::int32_t>(regs.ecx + k_saved_mode_offset), false);
                }
                log::get()->debug(
                    "TR2013 MultiplayerMouseHook: released the highlight's mouse lock");
            }
        };
    } // namespace

    auto HookTraits<Tag>::install(const Addrs &addrs) -> bool {
        const auto setter = addrs.mouse_mode_set.value();
        g_set_mode        = reinterpret_cast<SetModeFn>(setter);
        g_unlock          = reinterpret_cast<UnlockFn>(addrs.mouse_mode_unlock.value());
        g_lock_flag       = mem::read<std::uintptr_t>(setter + k_lock_operand_offset);

        auto addr   = addrs.highlight_release.value();
        auto result = mem::make_hook<ReleaseLock>(addr);
        if (!result) {
            log::get()->error("TR2013 MultiplayerMouseHook: {}", result.error());
            return false;
        }
        g_hook = std::move(*result);

        log::get()->info("TR2013 MultiplayerMouseHook: installed at 0x{:X}", addr);
        return true;
    }
} // namespace hooks
