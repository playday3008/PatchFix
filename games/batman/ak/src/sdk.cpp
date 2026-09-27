#include "games/batman/ak/sdk.hpp"

#include <cstdint>

#include "core/logger.hpp" // IWYU pragma: keep

#include "core/mem/x64.hpp"

#include "GameDefines.hpp"

namespace games::batman::ak::sdk {
    namespace {
        // Offsets of the RIP-relative disp32 inside the GOBJECTS and GNAMES
        // matches ("lea rbp, GObjects" at +18, "mov rcx, GNames" at +9).
        constexpr std::uintptr_t k_gobjects_disp = 21;
        constexpr std::uintptr_t k_gnames_disp   = 12;
    } // namespace

    auto init(const Addrs &addrs) -> bool {
        // required_patterns guarantees these are set before install() calls init().
        const auto gobjects = mem::x64::read_rel(
            addrs.gobjects.value() + k_gobjects_disp); // NOLINT(bugprone-unchecked-optional-access)
        const auto gnames = mem::x64::read_rel(
            addrs.gnames.value() + k_gnames_disp); // NOLINT(bugprone-unchecked-optional-access)

        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        GObjects = reinterpret_cast<GObjectsArray *>(gobjects);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        GNames = reinterpret_cast<TArray<FNameEntry *> *>(gnames);

        log::get()->info("Arkham Knight SDK: GObjects=0x{:X} GNames=0x{:X}", gobjects, gnames);
        return gobjects != 0 && gnames != 0;
    }
} // namespace games::batman::ak::sdk
