#include "games/tr/2013/hooks/load_steam_saves.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>

#include <new>
#include <string_view>
#include <utility>
#include <vector>

#include "core/logger.hpp" // IWYU pragma: keep

#include "core/mem/hook.hpp"
#include "core/mem/insn.hpp"

#include "games/tr/2013/registry.hpp"

namespace hooks {
    namespace {
        using Tag = games::tr::tr2013::LoadSteamSavesHook;

        // A Steam save is the game state followed by 40 bytes of slot info (date,
        // progress) stored separately, compressed as one zlib stream. The Epic build inflates saves
        // into a buffer of exactly the game state's size, so a Steam save fills it and zlib returns
        // Z_BUF_ERROR before it reaches the stream's checksum. To accept such a save without
        // skipping that check, it is inflated again into a buffer with room for the slot info, and
        // taken only if that succeeds, checksum included, at exactly the Steam size. The Epic build
        // takes the slot info from the start of the game state, which usually
        // matches the trailing one, so the trailing slot info is dropped.
        constexpr int           k_z_ok           = 0;
        constexpr auto          k_z_buf_error    = static_cast<std::uintptr_t>(-5);
        constexpr std::uint32_t k_slot_info_size = 40;

        // The hook sits right after a 5-byte call to the game's zlib uncompress.
        constexpr std::uintptr_t k_call_size = 5;

        // zlib uncompress(dest, &dest_len, src, src_len), statically linked.
        using Uncompress        = int(__cdecl *)(void          *dest,
                                                 std::uint32_t *dest_len,
                                                 const void    *src,
                                                 std::uint32_t  src_len);
        Uncompress g_uncompress = nullptr;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wglobal-constructors"
        mem::MidHook g_hook;
#pragma clang diagnostic pop

        auto stack_arg(std::uintptr_t esp, std::size_t index) -> std::uintptr_t {
            return *reinterpret_cast<const std::uintptr_t *>(esp +
                                                             (index * sizeof(std::uintptr_t)));
        }

        // Runs right after the inflate call in the local-save worker, with its
        // result in eax and its four arguments still on the stack. On failure
        // uncompress leaves *dest_len at the buffer size.
        struct AcceptTrailingSlotInfo {
            [[maybe_unused]] static constexpr std::string_view name = "LoadSteamSaves";

            [[maybe_unused]] static void operator()(mem::Registers &regs) {
                if (regs.eax != k_z_buf_error || !games::tr::tr2013::registry().enabled<Tag>()) {
                    return;
                }
                auto       *dest      = reinterpret_cast<void *>(stack_arg(regs.esp, 0));
                auto       *dest_len  = reinterpret_cast<std::uint32_t *>(stack_arg(regs.esp, 1));
                const auto *src       = reinterpret_cast<const void *>(stack_arg(regs.esp, 2));
                const auto  src_len   = static_cast<std::uint32_t>(stack_arg(regs.esp, 3));
                const auto  dest_size = *dest_len;

                const auto scratch_size = dest_size + k_slot_info_size;
                // An exception cannot unwind through the hook stub, so a failed
                // allocation is caught here and leaves the stock rejection.
                std::vector<std::byte> scratch;
                try {
                    scratch.resize(scratch_size);
                } catch (const std::bad_alloc &) {
                    log::get()->warn("TR2013 LoadSteamSavesHook: out of memory, save rejected");
                    return;
                }
                auto       scratch_len = scratch_size;
                const auto result      = g_uncompress(scratch.data(), &scratch_len, src, src_len);
                if (result != k_z_ok || scratch_len != scratch_size) {
                    log::get()->warn("TR2013 LoadSteamSavesHook: save rejected (zlib {}, {} bytes)",
                                     result,
                                     scratch_len);
                    return;
                }
                std::memcpy(dest, scratch.data(), dest_size);
                *dest_len = dest_size;
                regs.eax  = 0;
                log::get()->info(
                    "TR2013 LoadSteamSavesHook: loaded a save with trailing slot info");
            }
        };
    } // namespace

    auto HookTraits<Tag>::install(const Addrs &addrs) -> bool {
        auto addr       = addrs.save_inflate_result.value();
        auto uncompress = mem::insn::branch_target(addr - k_call_size);
        if (!uncompress) {
            log::get()->error("TR2013 LoadSteamSavesHook: expected a call before 0x{:X}", addr);
            return false;
        }
        g_uncompress = reinterpret_cast<Uncompress>(*uncompress);

        auto result = mem::make_hook<AcceptTrailingSlotInfo>(addr);
        if (!result) {
            log::get()->error("TR2013 LoadSteamSavesHook: {}", result.error());
            return false;
        }
        g_hook = std::move(*result);

        log::get()->info("TR2013 LoadSteamSavesHook: installed at 0x{:X}", addr);
        return true;
    }
} // namespace hooks
