#include "games/tr/2013/hooks/save_compatibility.hpp"

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
        using Tag = games::tr::tr2013::SaveCompatibilityHook;

        // Both builds write each save as one zlib stream holding the same game
        // state. The Steam build (Steam Cloud) stores the 40-byte slot info
        // (date, progress) a second time after the game state and fills the load
        // menu from that trailer. Non-Steam builds (local files; checked on Epic)
        // write the game state only and read the slot info from its first 40
        // bytes, which usually match the trailer.
        constexpr std::uint32_t k_slot_info_size = 40;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wglobal-constructors"
        mem::MidHook g_cloud_hook;
        mem::MidHook g_local_hook;
#pragma clang diagnostic pop

        // Steam build: a non-Steam save inflates 40 bytes short, so the trailing
        // slot info is read from memory the inflate never wrote. It is filled from
        // the start of the game state instead.

        // Offsets into the cloud request, the thread parameter in edi.
        constexpr std::uintptr_t k_state_size_offset     = 0x4C;
        constexpr std::uintptr_t k_slot_info_size_offset = 0x50;
        // Inflated size, a local of the cloud thread at [ebp-0x0C].
        constexpr std::intptr_t k_inflated_size_offset = -0x0C;

        auto read_u32(std::uintptr_t addr) -> std::uint32_t {
            std::uint32_t value = 0;
            std::memcpy(&value, reinterpret_cast<const void *>(addr), sizeof(value));
            return value;
        }

        // Runs right after the inflate call in the cloud thread's read path,
        // with its result in eax and the inflated buffer, sized for the game
        // state plus the slot info, in esi.
        struct FillSlotInfo {
            [[maybe_unused]] static constexpr std::string_view name = "SaveCompatibility/Cloud";

            [[maybe_unused]] static void operator()(mem::Registers &regs) {
                if (regs.eax != 0 || !games::tr::tr2013::registry().enabled<Tag>()) {
                    return;
                }
                const auto state_size     = read_u32(regs.edi + k_state_size_offset);
                const auto slot_info_size = read_u32(regs.edi + k_slot_info_size_offset);
                const auto inflated_size  = read_u32(regs.ebp + k_inflated_size_offset);
                if (slot_info_size == 0 || inflated_size != state_size) {
                    return;
                }
                std::memcpy(reinterpret_cast<void *>(regs.esi + state_size),
                            reinterpret_cast<const void *>(regs.esi),
                            slot_info_size);
                log::get()->info(
                    "TR2013 SaveCompatibilityHook: filled slot info of a save without it");
            }
        };

        // Non-Steam builds: saves are inflated into a buffer of exactly the game
        // state's size, so a Steam save fills it and zlib returns Z_BUF_ERROR
        // before it reaches the stream's checksum. To accept such a save without
        // skipping that check, it is inflated again into a buffer with room for
        // the slot info, and taken only if that succeeds, checksum included, at
        // exactly the Steam size. The trailing slot info is dropped.
        constexpr int  k_z_ok        = 0;
        constexpr auto k_z_buf_error = static_cast<std::uintptr_t>(-5);

        // The hook sits right after a 5-byte call to the game's zlib uncompress.
        constexpr std::uintptr_t k_call_size = 5;

        // zlib uncompress(dest, &dest_len, src, src_len), statically linked.
        using Uncompress        = int(__cdecl *)(void          *dest,
                                                 std::uint32_t *dest_len,
                                                 const void    *src,
                                                 std::uint32_t  src_len);
        Uncompress g_uncompress = nullptr;

        auto stack_arg(std::uintptr_t esp, std::size_t index) -> std::uintptr_t {
            return *reinterpret_cast<const std::uintptr_t *>(esp +
                                                             (index * sizeof(std::uintptr_t)));
        }

        // Runs right after the inflate call in the local-save worker, with its
        // result in eax and its four arguments still on the stack. On failure
        // uncompress leaves *dest_len at the buffer size.
        struct AcceptTrailingSlotInfo {
            [[maybe_unused]] static constexpr std::string_view name = "SaveCompatibility/Local";

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
                    log::get()->warn("TR2013 SaveCompatibilityHook: out of memory, save rejected");
                    return;
                }
                auto       scratch_len = scratch_size;
                const auto result      = g_uncompress(scratch.data(), &scratch_len, src, src_len);
                if (result != k_z_ok || scratch_len != scratch_size) {
                    log::get()->warn(
                        "TR2013 SaveCompatibilityHook: save rejected (zlib {}, {} bytes)",
                        result,
                        scratch_len);
                    return;
                }
                std::memcpy(dest, scratch.data(), dest_size);
                *dest_len = dest_size;
                regs.eax  = 0;
                log::get()->info(
                    "TR2013 SaveCompatibilityHook: loaded a save with trailing slot info");
            }
        };
    } // namespace

    auto HookTraits<Tag>::install(const Addrs &addrs) -> bool {
        if (!addrs.cloud_inflate_result && !addrs.save_inflate_result) {
            log::get()->error("TR2013 SaveCompatibilityHook: no save read path found");
            return false;
        }

        if (addrs.cloud_inflate_result) {
            auto addr   = addrs.cloud_inflate_result.value();
            auto result = mem::make_hook<FillSlotInfo>(addr);
            if (!result) {
                log::get()->error("TR2013 SaveCompatibilityHook: {}", result.error());
                return false;
            }
            g_cloud_hook = std::move(*result);
            log::get()->info("TR2013 SaveCompatibilityHook: Steam Cloud read at 0x{:X}", addr);
        }

        // A failure past this point unhooks the cloud half, so a hook reported
        // as failed does not keep running.
        if (addrs.save_inflate_result) {
            auto addr       = addrs.save_inflate_result.value();
            auto uncompress = mem::insn::branch_target(addr - k_call_size);
            if (!uncompress) {
                log::get()->error("TR2013 SaveCompatibilityHook: expected a call before 0x{:X}",
                                  addr);
                g_cloud_hook = {};
                return false;
            }
            g_uncompress = reinterpret_cast<Uncompress>(*uncompress);

            auto result = mem::make_hook<AcceptTrailingSlotInfo>(addr);
            if (!result) {
                log::get()->error("TR2013 SaveCompatibilityHook: {}", result.error());
                g_cloud_hook = {};
                return false;
            }
            g_local_hook = std::move(*result);
            log::get()->info("TR2013 SaveCompatibilityHook: local save read at 0x{:X}", addr);
        }
        return true;
    }
} // namespace hooks
