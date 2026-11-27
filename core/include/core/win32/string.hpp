#pragma once

#include <cstddef>

#include <filesystem>
#include <string>

#include <Windows.h>

namespace win32 {
    inline auto wchar_to_utf8(LPCWCH src, int src_len) -> std::string {
        const int len = WideCharToMultiByte(CP_UTF8, 0, src, src_len, nullptr, 0, nullptr, nullptr);
        if (len <= 0) {
            return {};
        }
        std::string result(static_cast<std::size_t>(len), '\0');
        WideCharToMultiByte(CP_UTF8, 0, src, src_len, result.data(), len, nullptr, nullptr);
        return result;
    }

    // For logging: path::string() converts through the C runtime code page and
    // throws for a character that page lacks.
    inline auto path_to_utf8(const std::filesystem::path &path) -> std::string {
        const auto &wide = path.native();
        return wchar_to_utf8(wide.c_str(), static_cast<int>(wide.size()));
    }
} // namespace win32
