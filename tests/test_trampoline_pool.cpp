#include <cstddef>
#include <cstdint>

#include <algorithm>
#include <array>
#include <vector>

#include <Windows.h>

#include <catch2/catch_test_macros.hpp>

#include <safetyhook/allocator.hpp>
#include <safetyhook/mid_hook.hpp>

#include "core/mem/trampoline_pool.hpp"

namespace {
    constexpr std::uintptr_t k_rel32_reach = 0x7FFF'FFFF;
    constexpr std::uintptr_t k_granularity = 0x1'0000;

    struct ImageRange {
        std::uintptr_t lo;
        std::uintptr_t hi;
    };

    auto test_image() -> ImageRange {
        const auto  base = reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
        const auto *dos  = reinterpret_cast<const IMAGE_DOS_HEADER *>(base);
        const auto *nt   = reinterpret_cast<const IMAGE_NT_HEADERS *>(
            base + static_cast<std::uintptr_t>(dos->e_lfanew));
        return {.lo = base, .hi = base + nt->OptionalHeader.SizeOfImage};
    }

    auto distance(std::uintptr_t a, std::uintptr_t b) -> std::uintptr_t {
        return a > b ? a - b : b - a;
    }

    // Reserves every free region around [lo, hi] so nothing new can be placed in
    // rel32 reach of it, mirroring an engine that has claimed that range.
    class NearMemoryHog {
        std::vector<void *> regions_;

      public:
        NearMemoryHog(std::uintptr_t lo, std::uintptr_t hi) {
            SYSTEM_INFO si {};
            GetSystemInfo(&si);
            const auto min_addr = reinterpret_cast<std::uintptr_t>(si.lpMinimumApplicationAddress);
            const auto max_addr = reinterpret_cast<std::uintptr_t>(si.lpMaximumApplicationAddress);

            const std::uintptr_t start = lo > min_addr + k_rel32_reach ? lo - k_rel32_reach
                                                                       : min_addr;
            const std::uintptr_t end   = std::min(hi + k_rel32_reach, max_addr);

            MEMORY_BASIC_INFORMATION mbi {};
            for (std::uintptr_t p = start; p < end;
                 p = reinterpret_cast<std::uintptr_t>(mbi.BaseAddress) + mbi.RegionSize) {
                if (VirtualQuery(reinterpret_cast<void *>(p), &mbi, sizeof(mbi)) == 0) {
                    break;
                }
                if (mbi.State != MEM_FREE) {
                    continue;
                }
                const auto region_lo = reinterpret_cast<std::uintptr_t>(mbi.BaseAddress);
                const auto region_hi = region_lo + mbi.RegionSize;
                const auto aligned   = (region_lo + k_granularity - 1) & ~(k_granularity - 1);
                if (aligned >= region_hi) {
                    continue;
                }
                if (void *r = VirtualAlloc(reinterpret_cast<void *>(aligned),
                                           region_hi - aligned,
                                           MEM_RESERVE,
                                           PAGE_NOACCESS)) {
                    regions_.push_back(r);
                }
            }
        }

        ~NearMemoryHog() {
            for (void *r : regions_) {
                VirtualFree(r, 0, MEM_RELEASE);
            }
        }

        NearMemoryHog(const NearMemoryHog &)                     = delete;
        auto operator=(const NearMemoryHog &) -> NearMemoryHog & = delete;
        NearMemoryHog(NearMemoryHog &&)                          = delete;
        auto operator=(NearMemoryHog &&) -> NearMemoryHog &      = delete;
    };

    // mov eax, [rip+0]; ret; then padding so the hook has room to overwrite.
    alignas(16) std::array<std::uint8_t, 23> g_rip_relative_target {
        0x8B, 0x05, 0x00, 0x00, 0x00, 0x00, 0xC3, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
        0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
    };
} // namespace

TEST_CASE("pool is within rel32 reach of both ends of the range", "[trampoline_pool]") {
    const auto image = test_image();

    auto pool = mem::trampoline_pool::reserve(image.lo, image.hi);

    REQUIRE(pool);
    CHECK(distance(*pool, image.lo) <= k_rel32_reach);
    CHECK(distance(*pool, image.hi) <= k_rel32_reach);
}

TEST_CASE("mid hook on a RIP-relative instruction installs after near memory is exhausted",
          "[trampoline_pool]") {
    const auto image = test_image();
    REQUIRE(mem::trampoline_pool::reserve(image.lo, image.hi));

    auto *target = g_rip_relative_target.data();

    const NearMemoryHog hog(image.lo, image.hi);

    // Without the pool there must be no room left, or this test proves nothing.
    auto fresh = safetyhook::Allocator::create()->allocate_near({target}, 1);
    REQUIRE_FALSE(fresh);
    REQUIRE(fresh.error() == safetyhook::Allocator::Error::NO_MEMORY_IN_RANGE);

    auto hook = safetyhook::MidHook::create(target, [](safetyhook::Context &) {});

    REQUIRE(hook);
    CHECK(target[0] == 0xE9);
}
