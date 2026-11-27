#include "core/diagnostics/minidump.hpp"

#include <array>
#include <format>
#include <string>
#include <string_view>

#include <Windows.h>
#include <DbgHelp.h>

#include <spdlog/spdlog.h>

#include "core/diagnostics/crash_logger.hpp"
#include "core/win32/string.hpp"

namespace diagnostics {
    namespace {
        using MiniDumpWriteDump_t = decltype(MiniDumpWriteDump);

        auto get_dump_fn() -> MiniDumpWriteDump_t * {
            // Null-forever if dbghelp is absent on first call;
            // acceptable (always present on Windows).
            static auto *h_dbghelp = LoadLibraryA("dbghelp.dll");
            if (h_dbghelp == nullptr) {
                return nullptr;
            }
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-function-type-strict"
            static auto *fn = reinterpret_cast<MiniDumpWriteDump_t *>(
                GetProcAddress(h_dbghelp, "MiniDumpWriteDump"));
            return fn;
#pragma clang diagnostic pop
        }

        void build_dump_path(std::array<wchar_t, MAX_PATH> &out) {
            std::array<wchar_t, MAX_PATH> module_path {};
            HMODULE                       hSelf = nullptr;

            const bool has_self =
                GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                                       GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                   reinterpret_cast<LPCSTR>(&build_dump_path),
                                   &hSelf) != FALSE;

            std::wstring_view dir;
            std::wstring_view stem = L"patchfix_crash";

            if (has_self && GetModuleFileNameW(hSelf,
                                               module_path.data(),
                                               static_cast<DWORD>(module_path.size())) != FALSE) {
                const std::wstring_view path(module_path.data());
                const auto              sep = path.rfind(L'\\');
                dir                 = (sep != std::wstring_view::npos) ? path.substr(0, sep + 1)
                                                                       : std::wstring_view {};
                const auto filename = (sep != std::wstring_view::npos) ? path.substr(sep + 1)
                                                                       : path;
                const auto dot      = filename.rfind(L'.');
                stem = (dot != std::wstring_view::npos) ? filename.substr(0, dot) : filename;
            }

            SYSTEMTIME st {};
            GetLocalTime(&st);

            auto result = std::format_to_n(out.data(),
                                           out.size() - 1,
                                           L"{}{}.{:04}{:02}{:02}_{:02}{:02}{:02}.dmp",
                                           dir,
                                           stem,
                                           st.wYear,
                                           st.wMonth,
                                           st.wDay,
                                           st.wHour,
                                           st.wMinute,
                                           st.wSecond);
            *result.out = L'\0';
        }
    } // namespace

    void write_minidump(EXCEPTION_POINTERS *ep) {
        auto *fn = get_dump_fn();
        if (fn == nullptr) {
            log().warn("Minidump: dbghelp.dll not available");
            return;
        }

        std::array<wchar_t, MAX_PATH> dump_path {};
        build_dump_path(dump_path);
        const std::string dump_name = win32::wchar_to_utf8(dump_path.data(), -1);

        HANDLE hFile = CreateFileW(dump_path.data(),
                                   GENERIC_WRITE,
                                   0,
                                   nullptr,
                                   CREATE_ALWAYS,
                                   FILE_ATTRIBUTE_NORMAL,
                                   nullptr);
        if (hFile == INVALID_HANDLE_VALUE) {
            log().warn("Minidump: failed to create {}", dump_name.c_str());
            return;
        }

        MINIDUMP_EXCEPTION_INFORMATION mei {};
        mei.ThreadId          = GetCurrentThreadId();
        mei.ExceptionPointers = ep;
        mei.ClientPointers    = FALSE;

        // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
        auto dump_type = static_cast<MINIDUMP_TYPE>(
            static_cast<unsigned>(MiniDumpWithThreadInfo) |
            static_cast<unsigned>(MiniDumpWithIndirectlyReferencedMemory));

        const BOOL ok =
            fn(GetCurrentProcess(), GetCurrentProcessId(), hFile, dump_type, &mei, nullptr, nullptr);
        CloseHandle(hFile);

        if (ok != FALSE) {
            log().critical("Minidump written: {}", dump_name.c_str());
        } else {
            log().warn("Minidump: MiniDumpWriteDump failed (error {})", GetLastError());
        }
    }
} // namespace diagnostics
