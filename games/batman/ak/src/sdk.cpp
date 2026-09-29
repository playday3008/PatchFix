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

        ReallocFn s_realloc = nullptr;
    } // namespace

    auto init(const Addrs &addrs) -> bool {
        // required_patterns guarantees these are set before install() calls init().
        const auto gobjects = mem::x64::read_rel(
            addrs.gobjects.value() + k_gobjects_disp); // NOLINT(bugprone-unchecked-optional-access)
        const auto gnames = mem::x64::read_rel(
            addrs.gnames.value() + k_gnames_disp); // NOLINT(bugprone-unchecked-optional-access)
        const auto app_realloc =
            addrs.app_realloc.value(); // NOLINT(bugprone-unchecked-optional-access)

        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        GObjects = reinterpret_cast<GObjectsArray *>(gobjects);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        GNames = reinterpret_cast<TArray<FNameEntry *> *>(gnames);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        s_realloc = reinterpret_cast<ReallocFn>(app_realloc);

        log::get()->info("Arkham Knight SDK: GObjects=0x{:X} GNames=0x{:X} appRealloc=0x{:X}",
                         gobjects,
                         gnames,
                         app_realloc);
        return gobjects != 0 && gnames != 0 && s_realloc != nullptr;
    }

    auto make_fstring(std::wstring_view s) -> FString {
        return make_fstring(s_realloc, s);
    }

    auto make_string_array(std::span<const std::wstring> items) -> TArray<FString> {
        return make_string_array(s_realloc, items);
    }
} // namespace games::batman::ak::sdk
