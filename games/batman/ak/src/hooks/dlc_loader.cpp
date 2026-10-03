#include "games/batman/ak/hooks/dlc_loader.hpp"

#include <cwchar>

#include <array>
#include <exception>
#include <filesystem>
#include <string>
#include <system_error>
#include <vector>

#include <Windows.h>

#include "core/logger.hpp" // IWYU pragma: keep

#include "core/win32/string.hpp"

#include "games/batman/ak/custom_content.hpp"
#include "games/batman/ak/registry.hpp"
#include "games/batman/ak/sdk.hpp"

#include "GameDefines.hpp"
#include "SDK_HEADERS/Core_classes.hpp"
#include "SDK_HEADERS/Core_structs.hpp"
#include "SDK_HEADERS/Engine_classes.hpp"
#include "SDK_HEADERS/Engine_structs.hpp"

namespace games::batman::ak {
    namespace {
        using Tag = DLCLoaderHook;

        // Game thread only. s_installing stops InstallDLC, which re-enters
        // ProcessEvent, from triggering a nested install.
        bool    s_disabled      = false;
        bool    s_installing    = false;
        UClass *s_manager_class = nullptr;

        sdk::DeferredName s_trigger_name {"DLCLoader", "RefreshDLCEnumComplete"};

        auto utf8(const std::wstring &s) -> std::string {
            return win32::wchar_to_utf8(s.c_str(), static_cast<int>(s.size()));
        }

        auto exe_dir() -> std::filesystem::path {
            std::array<wchar_t, MAX_PATH> buf {};
            const DWORD                   len = GetModuleFileNameW(nullptr, buf.data(), MAX_PATH);
            return std::filesystem::path(std::wstring(buf.data(), len)).parent_path();
        }

        // Folders under DLC (the game's own, not Custom) that Steam did not
        // install: either not a bare appid, or an appid Steam does not report
        // owned and installed. Logged before custom_root's own folders install,
        // so mgr's InstalledDLC still holds only the game's own entries.
        void log_discarded_dlc(UDownloadableContentManager *mgr,
                               const std::filesystem::path &custom_root) {
            const auto dlc_dir = (exe_dir() / L"..\\..\\DLC").lexically_normal();

            std::error_code list_ec;
            const auto      folders = custom_content::list_folders(dlc_dir, list_ec);
            if (list_ec) {
                log::get()->warn("DLCLoader: could not list {}: {}",
                                 utf8(dlc_dir.wstring()),
                                 list_ec.message());
                return;
            }

            std::vector<std::wstring> names;
            for (const auto &folder : folders) {
                if (_wcsicmp(folder.lexically_normal().wstring().c_str(),
                             custom_root.wstring().c_str()) == 0) {
                    continue;
                }
                names.push_back(folder.filename().wstring());
            }

            std::vector<std::wstring> installed;
            installed.reserve(mgr->InstalledDLC.size());
            for (const auto &entry : mgr->InstalledDLC) {
                installed.push_back(entry.ToWideString());
            }

            for (const auto &discarded : custom_content::discarded_dlc(names, installed)) {
                const char *why = discarded.reason == custom_content::DiscardReason::NotAnAppId
                                      ? "not an appid, the game ignores it"
                                      : "Steam does not report it owned and installed";
                log::get()->info("DLCLoader: DLC\\{} not installed: {}", utf8(discarded.name), why);
            }
        }

        void install_custom_content(UDownloadableContentManager *mgr) {
            const auto configured = registry().config<Tag>().custom_root.get();
            const auto root       = custom_content::resolve_root(exe_dir(), configured);
            const int  before     = mgr->InstalledDLC.size();
            log::get()->info("DLCLoader: custom root {}, {} DLC installed",
                             utf8(root.wstring()),
                             before);
            for (int i = 0; i < before; ++i) {
                log::get()->debug("DLCLoader: installed {}",
                                  utf8(mgr->InstalledDLC.at(i).ToWideString()));
            }

            std::error_code list_ec;
            const auto      folders = custom_content::list_folders(root, list_ec);
            if (list_ec) {
                log::get()->warn("DLCLoader: could not list {}: {}",
                                 utf8(root.wstring()),
                                 list_ec.message());
            }

            // Diagnostic only: a failure here must not stop custom content installing.
            try {
                log_discarded_dlc(mgr, root);
            } catch (const std::exception &e) {
                log::get()->warn("DLCLoader: could not check the game's DLC folders: {}", e.what());
            }

            for (const auto &folder : folders) {
                const auto name = folder.filename().wstring();
                const auto bundle =
                    custom_content::build_bundle(folder,
                                                 custom_content::game_prefix(configured, name));
                if (bundle.error) {
                    log::get()->warn("DLCLoader: {} scan hit an error: {}",
                                     utf8(name),
                                     bundle.error.message());
                }
                if (bundle.packages.empty() && bundle.files.empty()) {
                    log::get()->info("DLCLoader: {} skipped, no files", utf8(name));
                    continue;
                }

                FOnlineContent content {};
                content.FriendlyName    = sdk::make_fstring(name);
                content.Filename        = sdk::make_fstring(name);
                content.ContentPackages = sdk::make_string_array(bundle.packages);
                content.ContentFiles    = sdk::make_string_array(bundle.files);

                if (mgr->InstallDLC(content)) {
                    log::get()->info("DLCLoader: {} installed, {} packages, {} files",
                                     utf8(name),
                                     bundle.packages.size(),
                                     bundle.files.size());
                } else {
                    log::get()->warn("DLCLoader: {} was rejected by InstallDLC", utf8(name));
                }
            }

            log::get()->info("DLCLoader: {} DLC installed after custom content (was {})",
                             mgr->InstalledDLC.size(),
                             before);
        }

        void on_process_event(UObject *self, UFunction *fn, [[maybe_unused]] void *params) {
            if (fn == nullptr || s_disabled || s_installing || !registry().enabled<Tag>()) {
                return;
            }

            try {
                const auto trigger_name = s_trigger_name.index();
                if (trigger_name < 0 || fn->Name.GetDisplayIndex() != trigger_name ||
                    self == nullptr) {
                    return;
                }

                // Resolved lazily: only walks GObjects once the trigger name has
                // actually matched, instead of on every plugin's first ProcessEvent.
                if (s_manager_class == nullptr) {
                    s_manager_class = UDownloadableContentManager::StaticClass();
                    if (s_manager_class == nullptr) {
                        log::get()->error("DLCLoader: manager class not found");
                        s_disabled = true;
                        return;
                    }
                }

                if (!self->IsA(s_manager_class)) {
                    return;
                }
            } catch (const std::exception &e) {
                log::get()->error("DLCLoader: lookup failed: {}", e.what());
                s_disabled = true;
                return;
            }

            // The game clears all DLC before each refresh, so install every time.
            s_installing = true;
            try {
                // IsA checked above.
                // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
                install_custom_content(static_cast<UDownloadableContentManager *>(self));
            } catch (const std::exception &e) {
                log::get()->error("DLCLoader: install failed: {}", e.what());
            }
            s_installing = false;
        }
    } // namespace
} // namespace games::batman::ak

namespace hooks {
    auto HookTraits<games::batman::ak::DLCLoaderHook>::install([[maybe_unused]] const Addrs &addrs)
        -> bool {
        if (GetModuleHandleW(L"steam_api64.dll") == nullptr) {
            log::get()->info(
                "Arkham Knight DLCLoaderHook: this build loads every folder under DLC itself, "
                "loader not needed");
            return true;
        }
        games::batman::ak::subscribe(&games::batman::ak::on_process_event);
        log::get()->info("Arkham Knight DLCLoaderHook: subscribed to ProcessEvent");
        return true;
    }
} // namespace hooks
