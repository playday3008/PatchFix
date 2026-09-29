#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace games::batman::ak::custom_content {
    // One custom content folder as the game's FOnlineContent wants it.
    struct Bundle {
        std::wstring              name;     // folder name
        std::vector<std::wstring> packages; // game paths of *.upk
        std::vector<std::wstring> files;    // game paths of everything else
        std::error_code           error;    // first error hit while walking folder, if any
    };

    // The configured custom root on disk: absolute as given, relative from exe_dir.
    auto resolve_root(const std::filesystem::path &exe_dir, std::wstring_view configured)
        -> std::filesystem::path;

    // The path prefix the game sees for `folder`: the configured root text with
    // backslashes and the folder name appended, like the game's own
    // "..\..\DLC\<appid>" bundles.
    auto game_prefix(std::wstring_view configured, std::wstring_view folder) -> std::wstring;

    // Direct subfolders of root in case-insensitive name order; empty when root
    // is missing or unreadable.
    auto list_folders(const std::filesystem::path &root) -> std::vector<std::filesystem::path>;

    // Same, and reports the first error hit listing root (including a missing
    // root) through ec, leaving ec cleared on success.
    auto list_folders(const std::filesystem::path &root, std::error_code &ec)
        -> std::vector<std::filesystem::path>;

    // Every regular file under folder, split into packages and other files,
    // each as prefix + "\" + relative path, in case-insensitive path order.
    // bundle.error carries the first error hit walking folder, if any.
    auto build_bundle(const std::filesystem::path &folder, std::wstring_view prefix) -> Bundle;
} // namespace games::batman::ak::custom_content
