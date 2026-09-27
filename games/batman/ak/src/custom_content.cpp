#include "games/batman/ak/custom_content.hpp"

#include <cwchar>

#include <algorithm>
#include <filesystem>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace games::batman::ak::custom_content {
    namespace {
        namespace fs = std::filesystem;

        auto iless(const std::wstring &a, const std::wstring &b) -> bool {
            return _wcsicmp(a.c_str(), b.c_str()) < 0;
        }

        auto backslashed(std::wstring s) -> std::wstring {
            std::ranges::replace(s, L'/', L'\\');
            return s;
        }
    } // namespace

    auto resolve_root(const fs::path &exe_dir, std::wstring_view configured) -> fs::path {
        const fs::path root(configured);
        return (root.is_absolute() ? root : exe_dir / root).lexically_normal();
    }

    auto game_prefix(std::wstring_view configured, std::wstring_view folder) -> std::wstring {
        std::wstring out = backslashed(std::wstring(configured));
        while (!out.empty() && out.back() == L'\\') {
            out.pop_back();
        }
        out += L'\\';
        out += folder;
        return out;
    }

    auto list_folders(const fs::path &root, std::error_code &ec) -> std::vector<fs::path> {
        ec = {};
        std::vector<fs::path> out;
        for (fs::directory_iterator it(root, ec), end; !ec && it != end; it.increment(ec)) {
            std::error_code status_ec;
            if (it->is_directory(status_ec) && !status_ec) {
                out.push_back(it->path());
            }
        }
        std::ranges::sort(out, [](const fs::path &a, const fs::path &b) -> bool {
            return iless(a.filename().wstring(), b.filename().wstring());
        });
        return out;
    }

    auto list_folders(const fs::path &root) -> std::vector<fs::path> {
        std::error_code ec;
        return list_folders(root, ec);
    }

    auto build_bundle(const fs::path &folder, std::wstring_view prefix) -> Bundle {
        std::vector<std::wstring> rels;
        std::error_code           ec;
        for (fs::recursive_directory_iterator
                 it(folder, fs::directory_options::skip_permission_denied, ec),
             end;
             !ec && it != end;
             it.increment(ec)) {
            std::error_code status_ec;
            if (it->is_regular_file(status_ec) && !status_ec) {
                rels.push_back(backslashed(it->path().lexically_relative(folder).wstring()));
            }
        }
        std::ranges::sort(rels, iless);

        Bundle bundle {.name     = folder.filename().wstring(),
                       .packages = {},
                       .files    = {},
                       .error    = ec};
        for (const auto &rel : rels) {
            std::wstring game_path = std::wstring(prefix) + L'\\' + rel;
            const auto   ext       = fs::path(rel).extension().wstring();
            (_wcsicmp(ext.c_str(), L".upk") == 0 ? bundle.packages : bundle.files)
                .push_back(std::move(game_path));
        }
        return bundle;
    }
} // namespace games::batman::ak::custom_content
