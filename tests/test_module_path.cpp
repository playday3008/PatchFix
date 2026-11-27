#include <clocale>

#include <filesystem>
#include <string>
#include <system_error>
#include <utility>

#include <Windows.h>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "core/win32/pe.hpp"

namespace {
    // Removes the directory when destroyed, including after a failed setup.
    struct DirGuard {
        std::filesystem::path dir;

        explicit DirGuard(std::filesystem::path d) : dir(std::move(d)) {
            std::filesystem::create_directories(dir);
        }
        DirGuard(const DirGuard &)                     = delete;
        DirGuard(DirGuard &&)                          = delete;
        auto operator=(const DirGuard &) -> DirGuard & = delete;
        auto operator=(DirGuard &&) -> DirGuard &      = delete;
        ~DirGuard() {
            std::error_code ignored;
            std::filesystem::remove_all(dir, ignored);
        }
    };

    // Copies this test executable into `dir` and maps it as a module, so
    // GetModuleFileNameW reports a path inside `dir`. The copy's imports are
    // not resolved and its entry point does not run.
    struct ModuleCopy {
        DirGuard              guard;
        std::filesystem::path path;
        HMODULE               module = nullptr;

        explicit ModuleCopy(const std::filesystem::path &dir)
            : guard(dir),
              path(dir / L"copy.exe") {
            const auto self = win32::get_module_path(nullptr);
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

// The MSVC STL converts narrow paths through UTF-8 under a UTF-8 C runtime
// locale and through the system ANSI code page under any other, so these two
// locales cover both conversions. The directory mixes Cyrillic and Japanese,
// which no single-byte or DBCS ANSI code page holds; a system set to the UTF-8
// ANSI code page would not catch a narrow round trip.
TEST_CASE("get_module_path keeps a non-ASCII directory under both narrow path conversions",
          "[win32][path]") {
    const char *locale = GENERATE("C", ".UTF8");
    const auto  dir    = std::filesystem::temp_directory_path() /
                         (L"PatchFix Тест テスト " + std::to_wstring(GetCurrentProcessId()));

    const ModuleCopy copy(dir);
    REQUIRE(copy.module != nullptr);

    const LocaleGuard guard;
    REQUIRE(std::setlocale(LC_ALL, locale) != nullptr);

    const auto path = win32::get_module_path(copy.module);
    CHECK(path.native() == copy.path.native());
    CHECK(std::filesystem::exists(path));
}

TEST_CASE("get_module_path returns a path longer than MAX_PATH whole", "[win32][path]") {
    // \\?\ lifts the MAX_PATH limit for the file APIs and the loader.
    auto dir = std::filesystem::path(L"\\\\?\\" + std::filesystem::temp_directory_path().native()) /
               (L"PatchFix long " + std::to_wstring(GetCurrentProcessId()));
    const auto root = dir;
    while (dir.native().size() < 2 * MAX_PATH) {
        dir /= std::wstring(60, L'd');
    }

    const DirGuard   root_guard(root);
    const ModuleCopy copy(dir);
    REQUIRE(copy.module != nullptr);

    const auto path = win32::get_module_path(copy.module);
    CHECK(path.native().size() > MAX_PATH);
    CHECK(path.filename() == L"copy.exe");
    CHECK(path.parent_path().filename() == dir.filename());
}
