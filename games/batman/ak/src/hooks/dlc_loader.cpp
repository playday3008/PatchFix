#include "games/batman/ak/hooks/dlc_loader.hpp"

#include <array>
#include <exception>
#include <filesystem>
#include <string>

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

        // Game thread only. The lookup runs once; a miss disables the loader
        // for the session. s_installing stops InstallDLC, which re-enters
        // ProcessEvent, from triggering a nested install.
        bool    s_disabled      = false;
        bool    s_installing    = false;
        int     s_trigger_name  = -1;
        UClass *s_manager_class = nullptr;

        auto utf8(const std::wstring &s) -> std::string {
            return win32::wchar_to_utf8(s.c_str(), static_cast<int>(s.size()));
        }

        auto exe_dir() -> std::filesystem::path {
            std::array<wchar_t, MAX_PATH> buf {};
            const DWORD                   len = GetModuleFileNameW(nullptr, buf.data(), MAX_PATH);
            return std::filesystem::path(std::wstring(buf.data(), len)).parent_path();
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

            for (const auto &folder : custom_content::list_folders(root)) {
                const auto name = folder.filename().wstring();
                const auto bundle =
                    custom_content::build_bundle(folder,
                                                 custom_content::game_prefix(configured, name));
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

            if (s_trigger_name < 0) {
                s_trigger_name  = FName("RefreshDLCEnumComplete").GetDisplayIndex();
                s_manager_class = UDownloadableContentManager::StaticClass();
                if (s_trigger_name < 0 || s_manager_class == nullptr) {
                    log::get()->error("DLCLoader: trigger function or manager class not found");
                    s_disabled = true;
                    return;
                }
            }

            if (fn->Name.GetDisplayIndex() != s_trigger_name || self == nullptr ||
                !self->IsA(s_manager_class)) {
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
        games::batman::ak::subscribe(&games::batman::ak::on_process_event);
        log::get()->info("Arkham Knight DLCLoaderHook: subscribed to ProcessEvent");
        return true;
    }
} // namespace hooks
