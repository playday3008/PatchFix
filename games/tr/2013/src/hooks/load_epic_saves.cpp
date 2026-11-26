#include "games/tr/2013/hooks/load_epic_saves.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>

#include <string_view>
#include <utility>

#include "core/logger.hpp" // IWYU pragma: keep

#include "core/mem/hook.hpp"

#include "games/tr/2013/registry.hpp"

namespace hooks {
    namespace {
        using Tag = games::tr::tr2013::LoadEpicSavesHook;

        // The Steam build stores each save as the game state followed by a
        // 40-byte copy of its slot info (date, progress), compressed as one
        // stream, and fills the load menu's slot entry from that copy. Epic
        // saves end after the game state, so the copy is read from memory the
        // inflate never wrote. The game state starts with the same slot info
        // fields, so they are copied from there.

        // Offsets into the cloud request, the thread parameter in edi.
        constexpr std::uintptr_t k_state_size_offset     = 0x4C;
        constexpr std::uintptr_t k_slot_info_size_offset = 0x50;
        // Inflated size, a local of the cloud thread at [ebp-0x0C].
        constexpr std::uintptr_t k_inflated_size_offset = 0x0C;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wglobal-constructors"
        mem::MidHook g_hook;
#pragma clang diagnostic pop

        auto read_u32(std::uintptr_t addr) -> std::uint32_t {
            std::uint32_t value = 0;
            std::memcpy(&value, reinterpret_cast<const void *>(addr), sizeof(value));
            return value;
        }

        // Runs right after the inflate call in the cloud thread's read path,
        // with its result in eax and the inflated buffer, sized for the game
        // state plus the slot info, in esi.
        struct FillSlotInfo {
            [[maybe_unused]] static constexpr std::string_view name = "LoadEpicSaves";

            [[maybe_unused]] static void operator()(mem::Registers &regs) {
                if (regs.eax != 0 || !games::tr::tr2013::registry().enabled<Tag>()) {
                    return;
                }
                const auto state_size     = read_u32(regs.edi + k_state_size_offset);
                const auto slot_info_size = read_u32(regs.edi + k_slot_info_size_offset);
                const auto inflated_size  = read_u32(regs.ebp - k_inflated_size_offset);
                if (slot_info_size == 0 || inflated_size != state_size) {
                    return;
                }
                auto *buffer = reinterpret_cast<std::byte *>(regs.esi);
                // Indexes the game's buffer.
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                std::memcpy(buffer + state_size, buffer, slot_info_size);
                log::get()->info("TR2013 LoadEpicSavesHook: filled slot info of a save without it");
            }
        };
    } // namespace

    auto HookTraits<Tag>::install(const Addrs &addrs) -> bool {
        auto addr   = addrs.cloud_inflate_result.value();
        auto result = mem::make_hook<FillSlotInfo>(addr);
        if (!result) {
            log::get()->error("TR2013 LoadEpicSavesHook: {}", result.error());
            return false;
        }
        g_hook = std::move(*result);

        log::get()->info("TR2013 LoadEpicSavesHook: installed at 0x{:X}", addr);
        return true;
    }
} // namespace hooks
