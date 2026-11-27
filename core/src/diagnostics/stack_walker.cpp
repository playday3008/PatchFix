#include "core/diagnostics/stack_walker.hpp"

#include <cstddef>
#include <cstring>

#include <algorithm>
#include <mutex>
#include <string_view>

#include <Windows.h>
#include <DbgHelp.h>

namespace diagnostics {
    namespace {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wglobal-constructors"
        std::mutex g_dbghelp_mutex;
#pragma clang diagnostic pop

        // Every dbghelp export used here, as X(name). Each becomes a `p<name>`
        // member typed from the SDK declaration and resolved by that name.
#ifdef _M_IX86
#    define DBGHELP_EXPORTS(X)      \
        X(SymInitialize)            \
        X(SymFromAddr)              \
        X(StackWalk64)              \
        X(SymFunctionTableAccess64) \
        X(SymGetModuleBase64)
#else
#    define DBGHELP_EXPORTS(X) \
        X(SymInitialize)       \
        X(SymFromAddr)
#endif

        struct DbgHelp {
#define DBGHELP_MEMBER(name) decltype(name) *p##name {};
            DBGHELP_EXPORTS(DBGHELP_MEMBER)
#undef DBGHELP_MEMBER
            bool initialized {};
        };

        // Loaded on first use rather than linked, so a missing or broken dbghelp
        // only costs symbol names. Members stay null if dbghelp or an export is
        // absent; dbghelp ships with every Windows, so that is not retried.
        auto dbghelp() -> const DbgHelp & {
            static const DbgHelp api = [] -> DbgHelp {
                DbgHelp d {};
                auto   *h = LoadLibraryA("dbghelp.dll");
                if (h == nullptr) {
                    return d;
                }
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-function-type-strict"
#define DBGHELP_RESOLVE(name) \
    d.p##name = reinterpret_cast<decltype(name) *>(GetProcAddress(h, #name));
                DBGHELP_EXPORTS(DBGHELP_RESOLVE)
#undef DBGHELP_RESOLVE
#pragma clang diagnostic pop
                d.initialized = d.pSymInitialize != nullptr &&
                                d.pSymInitialize(GetCurrentProcess(), nullptr, TRUE) != FALSE;
                return d;
            }();
            return api;
        }
#undef DBGHELP_EXPORTS
    } // namespace

    auto capture_stack(const CONTEXT *ctx, std::span<StackFrame> buf) -> std::span<StackFrame> {
        CONTEXT local_ctx {};
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage-in-libc-call"
        std::memcpy(&local_ctx, ctx, sizeof(CONTEXT));
#pragma clang diagnostic pop

        auto out = buf.begin();
#ifdef _M_IX86
        // x86 has no unwind tables. StackWalk64 follows the EBP chain and uses
        // FPO records where symbols provide them, so a frame-pointer-omitting
        // function without symbols can end the walk early.
        const auto                        &api = dbghelp();
        const std::unique_lock<std::mutex> lock(g_dbghelp_mutex, std::try_to_lock);
        if (!api.initialized || api.pStackWalk64 == nullptr || !lock.owns_lock()) {
            if (out != buf.end() && local_ctx.Eip != 0) {
                out->address = local_ctx.Eip;
                ++out;
            }
            return buf.first(static_cast<std::size_t>(out - buf.begin()));
        }

        STACKFRAME64 frame {};
        frame.AddrPC.Offset    = local_ctx.Eip;
        frame.AddrPC.Mode      = AddrModeFlat;
        frame.AddrFrame.Offset = local_ctx.Ebp;
        frame.AddrFrame.Mode   = AddrModeFlat;
        frame.AddrStack.Offset = local_ctx.Esp;
        frame.AddrStack.Mode   = AddrModeFlat;
        while (out != buf.end() && api.pStackWalk64(IMAGE_FILE_MACHINE_I386,
                                                    GetCurrentProcess(),
                                                    GetCurrentThread(),
                                                    &frame,
                                                    &local_ctx,
                                                    nullptr,
                                                    api.pSymFunctionTableAccess64,
                                                    api.pSymGetModuleBase64,
                                                    nullptr) != FALSE) {
            if (frame.AddrPC.Offset == 0) {
                break;
            }
            out->address = static_cast<std::uintptr_t>(frame.AddrPC.Offset);
            ++out;
        }
#else
        while (out != buf.end()) {
            if (local_ctx.Rip == 0) {
                break;
            }

            out->address = local_ctx.Rip;
            ++out;

            DWORD64 image_base = 0;
            auto   *fn_entry   = RtlLookupFunctionEntry(local_ctx.Rip, &image_base, nullptr);
            if (fn_entry == nullptr) {
                break;
            }

            const void *handler_data      = nullptr;
            ULONG64     establisher_frame = 0;
            RtlVirtualUnwind(UNW_FLAG_NHANDLER,
                             image_base,
                             local_ctx.Rip,
                             fn_entry,
                             &local_ctx,
                             const_cast<void **>(&handler_data),
                             &establisher_frame,
                             nullptr);
        }
#endif
        return buf.first(static_cast<std::size_t>(out - buf.begin()));
    }

    void module_file_name(HMODULE module, std::array<char, MAX_PATH> &out) {
        // The wide name is never truncated by the ANSI code page's longer
        // encoding; only the file name is converted, to UTF-8, into `out`.
        std::array<wchar_t, MAX_PATH> path {};
        const DWORD len = GetModuleFileNameW(module, path.data(), static_cast<DWORD>(path.size()));
        if (len == 0) {
            out.front() = '\0';
            return;
        }
        const std::wstring_view full(path.data(), len);
        const auto              sep  = full.rfind(L'\\');
        const auto              name = sep != std::wstring_view::npos ? full.substr(sep + 1) : full;
        const int               written = WideCharToMultiByte(CP_UTF8,
                                                              0,
                                                              name.data(),
                                                              static_cast<int>(name.size()),
                                                              out.data(),
                                                              static_cast<int>(out.size() - 1),
                                                              nullptr,
                                                              nullptr);
        out.at(static_cast<std::size_t>(written > 0 ? written : 0)) = '\0';
    }

    void resolve_modules(std::span<StackFrame> frames) {
        for (auto &frame : frames) {
            HMODULE hModule = nullptr;
            if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                                       GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                   reinterpret_cast<LPCSTR>(frame.address),
                                   &hModule) == FALSE) {
                continue;
            }
            frame.module_base   = reinterpret_cast<std::uintptr_t>(hModule);
            frame.module_offset = frame.address - frame.module_base;

            module_file_name(hModule, frame.module_name);
        }
    }

    void resolve_symbols(std::span<StackFrame> frames) {
        const auto &api = dbghelp();
        if (!api.initialized || api.pSymFromAddr == nullptr) {
            return;
        }

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage"
        alignas(SYMBOL_INFO) std::array<std::byte, sizeof(SYMBOL_INFO) + k_max_sym_len> buf {};
        auto *symbol = reinterpret_cast<SYMBOL_INFO *>(buf.data());
#pragma clang diagnostic pop
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen   = k_max_sym_len - 1;

        // DbgHelp requires caller-side serialization. This is reachable from the
        // per-hook callback filter, which runs on whatever game thread faulted, so
        // two threads can arrive here at once. try_lock rather than lock: a thread
        // already in DbgHelp may be the one that is stuck, and a crash report
        // without symbols beats a crash reporter that hangs.
        const std::unique_lock<std::mutex> sym_lock(g_dbghelp_mutex, std::try_to_lock);
        if (!sym_lock.owns_lock()) {
            return;
        }

        for (auto &frame : frames) {
            DWORD64 displacement = 0;
            if (api.pSymFromAddr(GetCurrentProcess(), frame.address, &displacement, symbol) !=
                FALSE) {
                // SYMBOL_INFO::Name is declared CHAR[1] but is a flexible array member; the
                // real storage is the k_max_sym_len tail of buf. Decay it explicitly.
                const auto *name     = static_cast<const char *>(symbol->Name);
                auto        name_len = std::char_traits<char>::length(name);
                auto        copy_len = std::min(name_len, k_max_sym_len - 1);
                auto        name_end = std::copy_n(name, copy_len, frame.symbol_name.begin());
                *name_end            = '\0';
                frame.symbol_offset  = displacement;
                frame.has_symbol     = true;
            }
        }
    }
} // namespace diagnostics
