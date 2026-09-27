#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <vector>

#include <Windows.h>

#include <catch2/catch_test_macros.hpp>

#include "games/batman/ak/custom_content.hpp"

namespace fs = std::filesystem;
using namespace games::batman::ak::custom_content;

namespace {
    struct TempDir {
        fs::path path;
        TempDir()
            : path(fs::temp_directory_path() /
                   fs::path(L"patchfix_custom_content" + std::to_wstring(GetCurrentProcessId()))) {
            fs::remove_all(path);
            fs::create_directories(path);
        }
        ~TempDir() {
            std::error_code ec;
            fs::remove_all(path, ec);
        }
        TempDir(const TempDir &)            = delete;
        TempDir &operator=(const TempDir &) = delete;
    };

    void touch(const fs::path &p) {
        fs::create_directories(p.parent_path());
        std::ofstream(p) << "x";
    }
} // namespace

TEST_CASE("list_folders returns sorted subfolders only", "[batman-ak][custom_content]") {
    const TempDir tmp;
    fs::create_directories(tmp.path / L"beta");
    fs::create_directories(tmp.path / L"Alpha");
    fs::create_directories(tmp.path / L"20_gamma");
    touch(tmp.path / L"readme.txt");

    const auto folders = list_folders(tmp.path);
    REQUIRE(folders.size() == 3);
    CHECK(folders[0].filename() == L"20_gamma");
    CHECK(folders[1].filename() == L"Alpha");
    CHECK(folders[2].filename() == L"beta");
}

TEST_CASE("list_folders on a missing root is empty", "[batman-ak][custom_content]") {
    CHECK(list_folders(fs::temp_directory_path() / L"patchfix_no_such_dir").empty());
}

TEST_CASE("list_folders on a missing root reports an error", "[batman-ak][custom_content]") {
    std::error_code ec;
    const auto      folders = list_folders(fs::temp_directory_path() / L"patchfix_no_such_dir", ec);
    CHECK(folders.empty());
    CHECK(ec);
}

TEST_CASE("build_bundle splits packages from files", "[batman-ak][custom_content]") {
    const TempDir tmp;
    const auto    folder = tmp.path / L"Skin";
    touch(folder / L"Content/BmGame/CookedPCConsole/B.UPK");
    touch(folder / L"Content/BmGame/CookedPCConsole/a.upk");
    touch(folder / L"Content/BmGame/Config/BmGame.ini");
    touch(folder / L"Content/BmGame/Localization/INT/GFxUI.int");

    const auto b = build_bundle(folder, L"..\\..\\DLC\\Custom\\Skin");
    CHECK(b.name == L"Skin");
    CHECK(b.packages == std::vector<std::wstring> {
                            L"..\\..\\DLC\\Custom\\Skin\\Content\\BmGame\\CookedPCConsole\\a.upk",
                            L"..\\..\\DLC\\Custom\\Skin\\Content\\BmGame\\CookedPCConsole\\B.UPK",
                        });
    CHECK(b.files ==
          std::vector<std::wstring> {
              L"..\\..\\DLC\\Custom\\Skin\\Content\\BmGame\\Config\\BmGame.ini",
              L"..\\..\\DLC\\Custom\\Skin\\Content\\BmGame\\Localization\\INT\\GFxUI.int",
          });
}

TEST_CASE("build_bundle on an empty folder is empty", "[batman-ak][custom_content]") {
    const TempDir tmp;
    fs::create_directories(tmp.path / L"Empty/Sub");

    const auto b = build_bundle(tmp.path / L"Empty", L"p\\Empty");
    CHECK(b.packages.empty());
    CHECK(b.files.empty());
}

TEST_CASE("build_bundle on a missing folder is empty", "[batman-ak][custom_content]") {
    const TempDir tmp;

    const auto b = build_bundle(tmp.path / L"NoSuchMod", L"p\\NoSuchMod");
    CHECK(b.name == L"NoSuchMod");
    CHECK(b.packages.empty());
    CHECK(b.files.empty());
}

TEST_CASE("build_bundle keeps non-ASCII names", "[batman-ak][custom_content]") {
    const TempDir tmp;
    const auto    folder = tmp.path / L"\u0421\u043A\u0456\u043D"; // U+0421 U+043A U+0456 U+043D
    touch(folder / L"\u0142\u00F3d\u017A.upk");                    // U+0142 U+00F3 d U+017A .upk

    const auto b = build_bundle(folder, L"p\\\u0421\u043A\u0456\u043D");
    CHECK(b.name == L"\u0421\u043A\u0456\u043D");
    CHECK(b.packages ==
          std::vector<std::wstring> {L"p\\\u0421\u043A\u0456\u043D\\\u0142\u00F3d\u017A.upk"});
}

TEST_CASE("game_prefix normalizes separators", "[batman-ak][custom_content]") {
    CHECK(game_prefix(L"..\\..\\DLC\\Custom", L"A") == L"..\\..\\DLC\\Custom\\A");
    CHECK(game_prefix(L"..\\..\\DLC\\Custom\\", L"A") == L"..\\..\\DLC\\Custom\\A");
    CHECK(game_prefix(L"C:/Custom/", L"A") == L"C:\\Custom\\A");
}

TEST_CASE("resolve_root joins relative roots to the exe dir", "[batman-ak][custom_content]") {
    const fs::path exe_dir = L"C:\\Game\\Binaries\\Win64";
    CHECK(resolve_root(exe_dir, L"..\\..\\DLC\\Custom") == fs::path(L"C:\\Game\\DLC\\Custom"));
    CHECK(resolve_root(exe_dir, L"D:\\Custom") == fs::path(L"D:\\Custom"));
}
