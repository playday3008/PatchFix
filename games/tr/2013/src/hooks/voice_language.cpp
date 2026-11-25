#include "games/tr/2013/hooks/voice_language.hpp"

#include <cstddef>

#include <string_view>
#include <utility>

#include "core/logger.hpp" // IWYU pragma: keep

#include "core/mem/hook.hpp"
#include "core/mem/insn.hpp"

#include "games/tr/2013/registry.hpp"

namespace hooks {
    namespace {
        using Tag = games::tr::tr2013::VoiceLanguageHook;

        // The Epic build reads its voice language from the VOLanguage setting at
        // startup and again when Display Options are applied, but the options
        // menu only changes it in memory and in the profile. A changed voice
        // language therefore reverts on the next launch, or as soon as Display
        // Options are applied. Writing it through the game's settings object, the
        // same one those reads use, keeps its cache and the registry in step.

        // Returns the settings object (nx::NxSettingsImpl).
        using GetSettings = void *(*)();
        // Settings vtable +0x40: __thiscall set_int(name, category, value),
        // which writes HKCU\Software\Crystal Dynamics\Tomb Raider\<category>
        // and updates the cached value. Category 0 is the root key. __fastcall
        // with an unused edx matches it: this in ecx, callee pops the rest.
        using SetInt =
            bool(__fastcall *)(void *self, void *edx, const char *name, int category, int value);
        constexpr std::size_t k_set_int_slot = 0x40 / sizeof(void *);

        GetSettings g_get_settings = nullptr;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wglobal-constructors"
        mem::MidHook g_hook;
#pragma clang diagnostic pop

        // Runs on the branch the menu takes when the voice language changed,
        // with the new language id in ebx.
        struct PersistVoiceLanguage {
            [[maybe_unused]] static constexpr std::string_view name = "VoiceLanguage";

            [[maybe_unused]] static void operator()(mem::Registers &regs) {
                if (!games::tr::tr2013::registry().enabled<Tag>()) {
                    return;
                }
                const auto language = static_cast<int>(regs.ebx);
                if (language < 0) {
                    return;
                }
                auto *settings = g_get_settings();
                if (settings == nullptr) {
                    return;
                }
                auto *vtable = *static_cast<SetInt **>(settings);
                // Indexes the game's vtable.
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                if (!vtable[k_set_int_slot](settings, nullptr, "VOLanguage", 0, language)) {
                    log::get()->warn("TR2013 VoiceLanguageHook: failed to save voice language {}",
                                     language);
                    return;
                }
                log::get()->info("TR2013 VoiceLanguageHook: saved voice language {}", language);
            }
        };
    } // namespace

    auto HookTraits<Tag>::install(const Addrs &addrs) -> bool {
        auto getter = mem::insn::branch_target(addrs.settings_getter_call.value());
        if (!getter) {
            log::get()->error("TR2013 VoiceLanguageHook: expected a call at 0x{:X}",
                              addrs.settings_getter_call.value());
            return false;
        }
        g_get_settings = reinterpret_cast<GetSettings>(*getter);

        auto addr   = addrs.vo_menu_apply.value();
        auto result = mem::make_hook<PersistVoiceLanguage>(addr);
        if (!result) {
            log::get()->error("TR2013 VoiceLanguageHook: {}", result.error());
            return false;
        }
        g_hook = std::move(*result);

        log::get()->info("TR2013 VoiceLanguageHook: installed at 0x{:X}", addr);
        return true;
    }
} // namespace hooks
