#include "games/batman/ak/sdk.hpp"

#include <cstdint>

#include <string>
#include <string_view>

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

    void free_buffer(void *ptr) {
        if (ptr != nullptr) {
            s_realloc(ptr, 0, 8);
        }
    }

    auto DeferredName::index() -> int {
        if (index_ >= 0) {
            return index_;
        }
        const auto *names = FName::Names();
        const auto  count = names != nullptr ? names->size() : 0;
        if (!gate_.should_attempt(count)) {
            return -1;
        }

        index_ = FName(std::string(name_).c_str()).GetDisplayIndex();
        if (index_ < 0) {
            gate_.record_failure(count);
            if (!logged_miss_) {
                logged_miss_ = true;
                log::get()->debug("{}: {} not registered yet, will retry when names change",
                                  component_,
                                  name_);
            }
            return -1;
        }
        log::get()->info("{}: {} resolved to name index {}", component_, name_, index_);
        return index_;
    }
} // namespace games::batman::ak::sdk
