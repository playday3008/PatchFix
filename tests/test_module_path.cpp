#include <clocale>

#include <filesystem>
#include <string>

#include <Windows.h>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "core/win32/pe.hpp"

namespace {
    // Copies this test executable into `dir` and maps it as a module, so
    // GetModuleFileNameW reports a path inside `dir`. The copy's imports are
    // not resolved and its entry point does not run.
    struct ModuleCopy {
        std::filesystem::path path;
        HMODULE               module = nullptr;

        explicit ModuleCopy(const std::filesystem::path &dir) : path(dir / L"copy.exe") {
            std::filesystem::create_directories(dir);
            std::wstring self(MAX_PATH, L'\0');
            self.resize(GetModuleFileNameW(nullptr, self.data(), MAX_PATH));
            std::filesystem::copy_file(self,
                                       path,
                                       std::filesystem::copy_options::overwrite_existing);
            module = LoadLibraryExW(path.c_str(), nullptr, DONT_RESOLVE_DLL_REFERENCES);
        }

        ModuleCopy(const ModuleCopy &)                     = delete;
        ModuleCopy(ModuleCopy &&)                          = delete;
        auto operator=(const ModuleCopy &) -> ModuleCopy & = delete;
        auto operator=(ModuleCopy &&) -> ModuleCopy &      = delete;

        ~ModuleCopy() {
            if (module != nullptr) {
                FreeLibrary(module);
            }
            std::error_code ignored;
            std::filesystem::remove_all(path.parent_path(), ignored);
        }
    };

    // Restores the "C" locale the process starts with.
    struct LocaleGuard {
        LocaleGuard()                                        = default;
        LocaleGuard(const LocaleGuard &)                     = delete;
        LocaleGuard(LocaleGuard &&)                          = delete;
        auto operator=(const LocaleGuard &) -> LocaleGuard & = delete;
        auto operator=(LocaleGuard &&) -> LocaleGuard &      = delete;
        ~LocaleGuard() { std::setlocale(LC_ALL, "C"); }
    };
} // namespace

TEST_CASE("get_module_path keeps a non-ASCII directory under any C runtime code page",
          "[win32][path]") {
    const char *locale = GENERATE("C", ".1252", ".1251", ".932", ".UTF8");
    // Cyrillic and Japanese: no single-byte code page holds both.
    const auto dir = std::filesystem::temp_directory_path() / L"PatchFix Тест テスト";

    const ModuleCopy copy(dir);
    REQUIRE(copy.module != nullptr);

    const LocaleGuard guard;
    if (std::setlocale(LC_ALL, locale) == nullptr) {
        SKIP("locale " << locale << " is not available");
    }

    const auto path = win32::get_module_path(copy.module);
    CHECK(path.native() == copy.path.native());
    CHECK(std::filesystem::exists(path));
}
