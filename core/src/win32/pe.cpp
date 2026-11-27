#include "core/win32/pe.hpp"

#include <filesystem>
#include <span>
#include <string>
#include <utility>

#include <Windows.h>

namespace win32 {
    auto enumerate_sections(HMODULE hModule) -> std::vector<SectionInfo> {
        auto base = reinterpret_cast<std::uintptr_t>(
            hModule != nullptr ? hModule : GetModuleHandleA(nullptr));

        const auto *dos = reinterpret_cast<const IMAGE_DOS_HEADER *>(base);
        if (dos->e_magic != IMAGE_DOS_SIGNATURE) {
            return {};
        }
        const auto *nt = reinterpret_cast<const IMAGE_NT_HEADERS *>(
            base + static_cast<std::uintptr_t>(dos->e_lfanew));
        if (nt->Signature != IMAGE_NT_SIGNATURE) {
            return {};
        }

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage-in-container"
        auto sections = std::span(IMAGE_FIRST_SECTION(nt), nt->FileHeader.NumberOfSections);
#pragma clang diagnostic pop

        std::vector<SectionInfo> result;
        result.reserve(sections.size());

        for (const auto &sec : sections) {
            const auto *raw_name = reinterpret_cast<const char *>(sec.Name);
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage-in-libc-call"
            auto name = std::string(raw_name, ::strnlen(raw_name, IMAGE_SIZEOF_SHORT_NAME));
#pragma clang diagnostic pop

            result.push_back({
                .name            = std::move(name),
                .base            = base + sec.VirtualAddress,
                .size            = sec.Misc.VirtualSize,
                .characteristics = sec.Characteristics,
            });
        }
        return result;
    }

    auto find_section(HMODULE hModule, std::string_view name) -> std::optional<SectionInfo> {
        for (auto &sec : enumerate_sections(hModule)) {
            if (sec.name == name) {
                return sec;
            }
        }
        return std::nullopt;
    }

    auto get_module_path(HMODULE hModule) -> std::filesystem::path {
        // Paths can exceed MAX_PATH where long paths are enabled. A result that
        // fills the buffer may be truncated, so grow until it does not.
        constexpr DWORD k_max_len = 32768;
        std::wstring    buf(MAX_PATH, L'\0');
        while (true) {
            const DWORD len =
                GetModuleFileNameW(hModule, buf.data(), static_cast<DWORD>(buf.size()));
            if (len == 0) {
                return {};
            }
            if (len < buf.size()) {
                buf.resize(len);
                return {std::move(buf)};
            }
            if (buf.size() >= k_max_len) {
                return {};
            }
            buf.resize(buf.size() * 2);
        }
    }
} // namespace win32
