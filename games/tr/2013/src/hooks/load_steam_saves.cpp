#include "games/tr/2013/hooks/load_steam_saves.hpp"

#include <cstdint>

#include <string_view>
#include <utility>

#include "core/logger.hpp" // IWYU pragma: keep

#include "core/mem/hook.hpp"

#include "games/tr/2013/registry.hpp"

namespace hooks {
    namespace {
        using Tag = games::tr::tr2013::LoadSteamSavesHook;

        // A Steam save is the 1310720-byte game state followed by a 40-byte copy
        // of its slot info, compressed as one zlib stream. The Epic build inflates
        // saves into a buffer of exactly the game state's size, so a Steam save
        // fills it and zlib returns Z_BUF_ERROR with input left over, which the
        // game treats as a failed load. The game state is complete at that point,
        // and the Epic build takes the slot info from its first bytes anyway.
        // A truncated or corrupt stream returns Z_DATA_ERROR instead and still
        // fails.
        constexpr auto k_z_buf_error = static_cast<std::uintptr_t>(-5);

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wglobal-constructors"
        mem::MidHook g_hook;
#pragma clang diagnostic pop

        // Runs right after the inflate call in the local-save worker, with its
        // result in eax.
        struct AcceptTrailingSlotInfo {
            [[maybe_unused]] static constexpr std::string_view name = "LoadSteamSaves";

            [[maybe_unused]] static void operator()(mem::Registers &regs) {
                if (regs.eax != k_z_buf_error || !games::tr::tr2013::registry().enabled<Tag>()) {
                    return;
                }
                regs.eax = 0;
                log::get()->info(
                    "TR2013 LoadSteamSavesHook: loaded a save with trailing slot info");
            }
        };
    } // namespace

    auto HookTraits<Tag>::install(const Addrs &addrs) -> bool {
        auto addr   = addrs.save_inflate_result.value();
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
