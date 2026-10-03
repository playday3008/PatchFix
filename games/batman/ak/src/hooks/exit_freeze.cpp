#include "games/batman/ak/hooks/exit_freeze.hpp"

#include <cstdint>
#include <cstring>

#include <array>

#include "core/logger.hpp" // IWYU pragma: keep

#include "core/mem/write.hpp"

#include "games/batman/ak/registry.hpp"

namespace hooks {
    namespace {
        // or edx, -1 (INFINITE)
        constexpr std::array<std::uint8_t, 3> k_infinite {0x83, 0xCA, 0xFF};
        // push 100; pop rdx: the same 3 bytes, a 100 ms timeout
        constexpr std::array<std::uint8_t, 3> k_timeout {0x6A, 0x64, 0x5A};

        auto patch(std::uintptr_t addr) -> bool {
            if (std::memcmp(reinterpret_cast<const void *>(addr),
                            k_infinite.data(),
                            k_infinite.size()) != 0) {
                log::get()->error("ExitFreeze: unexpected bytes at 0x{:X}", addr);
                return false;
            }
            return mem::write(addr, k_timeout.data(), k_timeout.size());
        }
    } // namespace

    auto HookTraits<games::batman::ak::ExitFreezeHook>::install(const Addrs &addrs) -> bool {
        // Patched once at launch: rewriting the bytes while a reclaimer thread
        // runs them could tear the instruction.
        if (!games::batman::ak::registry()
                 .config<games::batman::ak::ExitFreezeHook>()
                 .workaround.get()) {
            log::get()->info("ExitFreeze: Workaround off, skipping");
            return true;
        }
        if (!patch(addrs.reclaimer_wait.value()) || !patch(addrs.reclaimer_wait_alt.value())) {
            return false;
        }
        log::get()->info("Arkham Knight ExitFreezeHook: installed");
        return true;
    }
} // namespace hooks
