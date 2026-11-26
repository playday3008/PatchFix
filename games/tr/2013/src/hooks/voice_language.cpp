#include "games/tr/2013/hooks/voice_language.hpp"

#include <cstddef>

#include <filesystem>
#include <string_view>
#include <utility>

#include <Windows.h>

#include "core/logger.hpp" // IWYU pragma: keep

#include "core/mem/hook.hpp"
#include "core/mem/insn.hpp"
#include "core/win32/pe.hpp"

#include "games/tr/2013/registry.hpp"

namespace hooks {
    namespace {
        using Tag = games::tr::tr2013::VoiceLanguageHook;

        // Non-Steam builds (checked on Epic) read the voice language from the
        // VOLanguage setting at startup and again when Display Options are
        // applied, but the options menu only changes it in memory and in the
        // profile. A changed voice
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
        mem::MidHook g_row_hook;
        mem::MidHook g_list_hook;

        std::filesystem::path g_game_dir;
#pragma clang diagnostic pop

        // Options-menu control ids (sub_ABFD00 switch) and list heads in the menu object.
        constexpr std::uintptr_t k_voice_language_control = 0x1E;
        constexpr std::uintptr_t k_voice_list_offset      = 0x14FC;

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

        // The menu maps the current voice language to a dropdown row by searching
        // the text-language list, which has its own fixed order (Spanish before
        // Italian, Japanese last), while the voice list holds the installed
        // languages in id order. A saved language therefore shows as a different
        // one, e.g. Russian as Polish. Runs after edx is loaded with the text list
        // head; points it at the voice list for the voice control.
        struct VoiceRowLookup {
            [[maybe_unused]] static constexpr std::string_view name = "VoiceLanguage/Row";

            [[maybe_unused]] static void operator()(mem::Registers &regs) {
                if (!games::tr::tr2013::registry().enabled<Tag>() ||
                    regs.esi != k_voice_language_control) {
                    return;
                }
                regs.edx =
                    *reinterpret_cast<const std::uintptr_t *>(regs.edi + k_voice_list_offset);
            }
        };

        // The menu offers every language whose bigfile exists, but five ship
        // only a stub without voice-over and play English. Runs after that
        // existence check returns, with the language id in esi and the result
        // in al; clears al for a stub. A file whose size can't be read stays.
        struct HideStubLanguages {
            [[maybe_unused]] static constexpr std::string_view name = "VoiceLanguage/List";

            [[maybe_unused]] static void operator()(mem::Registers &regs) {
                if (!games::tr::tr2013::registry().enabled<Tag>() || (regs.eax & 0xFFU) == 0) {
                    return;
                }
                const auto file = games::tr::tr2013::voice_bigfile(static_cast<int>(regs.esi));
                if (file.empty()) {
                    return;
                }
                WIN32_FILE_ATTRIBUTE_DATA info {};
                if (GetFileAttributesExW((g_game_dir / file).c_str(),
                                         GetFileExInfoStandard,
                                         &info) == 0) {
                    return;
                }
                const auto size = (std::uint64_t {info.nFileSizeHigh} << 32U) | info.nFileSizeLow;
                if (games::tr::tr2013::is_voice_pack(size)) {
                    return;
                }
                regs.eax &= ~std::uintptr_t {0xFF};
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

        if (addrs.vo_menu_row_lookup) {
            auto row_addr   = addrs.vo_menu_row_lookup.value();
            auto row_result = mem::make_hook<VoiceRowLookup>(row_addr);
            if (row_result) {
                g_row_hook = std::move(*row_result);
                log::get()->info("TR2013 VoiceLanguageHook: menu row fix at 0x{:X}", row_addr);
            } else {
                log::get()->warn("TR2013 VoiceLanguageHook: menu row fix: {}", row_result.error());
            }
        }

        if (addrs.vo_menu_list_check) {
            g_game_dir       = win32::get_module_path(nullptr).parent_path();
            auto list_addr   = addrs.vo_menu_list_check.value();
            auto list_result = mem::make_hook<HideStubLanguages>(list_addr);
            if (list_result) {
                g_list_hook = std::move(*list_result);
                log::get()->info("TR2013 VoiceLanguageHook: stub language filter at 0x{:X}",
                                 list_addr);
            } else {
                log::get()->warn("TR2013 VoiceLanguageHook: stub language filter: {}",
                                 list_result.error());
            }
        }
        return true;
    }
} // namespace hooks
