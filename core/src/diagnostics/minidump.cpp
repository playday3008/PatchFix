#include "core/diagnostics/minidump.hpp"

#include <filesystem>
#include <format>
#include <string>

#include <Windows.h>
#include <DbgHelp.h>

#include <spdlog/spdlog.h>

#include "core/diagnostics/crash_logger.hpp"
#include "core/win32/pe.hpp"
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

        // <plugin dir>\<plugin name>.<timestamp>.dmp, or patchfix_crash.<timestamp>.dmp
        // in the working directory when the plugin's own path is unavailable.
        auto build_dump_path() -> std::filesystem::path {
            HMODULE               self = nullptr;
            std::filesystem::path dir;
            std::wstring          stem = L"patchfix_crash";
            if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                                       GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                   reinterpret_cast<LPCWSTR>(&build_dump_path),
                                   &self) != FALSE) {
                const auto path = win32::get_module_path(self);
                if (!path.empty()) {
                    dir  = path.parent_path();
                    stem = path.stem().native();
                }
            }

            SYSTEMTIME st {};
            GetLocalTime(&st);
            return dir / std::format(L"{}.{:04}{:02}{:02}_{:02}{:02}{:02}.dmp",
                                     stem,
                                     st.wYear,
                                     st.wMonth,
                                     st.wDay,
                                     st.wHour,
                                     st.wMinute,
                                     st.wSecond);
        }
    } // namespace

    void write_minidump(EXCEPTION_POINTERS *ep) {
        auto *fn = get_dump_fn();
        if (fn == nullptr) {
            log().warn("Minidump: dbghelp.dll not available");
            return;
        }

        const auto        dump_path = build_dump_path();
        const std::string dump_name = win32::path_to_utf8(dump_path);

        HANDLE hFile = CreateFileW(dump_path.c_str(),
                                   GENERIC_WRITE,
                                   0,
                                   nullptr,
                                   CREATE_ALWAYS,
                                   FILE_ATTRIBUTE_NORMAL,
                                   nullptr);
        if (hFile == INVALID_HANDLE_VALUE) {
            log().warn("Minidump: failed to create {}", dump_name);
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
            log().critical("Minidump written: {}", dump_name);
        } else {
            log().warn("Minidump: MiniDumpWriteDump failed (error {})", GetLastError());
        }
    }
} // namespace diagnostics
