#include "core/diagnostics/unhandled_filter.hpp"

#include <atomic>
#include <safetyhook.hpp>
#include <utility>

#include <Windows.h>

namespace diagnostics {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wglobal-constructors"
    namespace {
        safetyhook::InlineHook                    g_hook;
        std::atomic<LPTOP_LEVEL_EXCEPTION_FILTER> g_previous {nullptr};
        std::atomic<CrashReporter>                g_report {nullptr};

        // A reporter that faults, or a chained filter that calls back into the
        // top-level filter, re-enters on the same thread. Report only once.
        thread_local bool tl_in_filter = false;

        // The OS filter stays ours; what the game asked for becomes the chain.
        auto WINAPI hk_set_unhandled_exception_filter(LPTOP_LEVEL_EXCEPTION_FILTER filter)
            -> LPTOP_LEVEL_EXCEPTION_FILTER {
            return g_previous.exchange(filter);
        }
    } // namespace
#pragma clang diagnostic pop

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wlanguage-extension-token"
    auto WINAPI unhandled_filter(EXCEPTION_POINTERS *ep) -> LONG {
        auto *report = g_report.load(std::memory_order_acquire);
        // A stack overflow leaves a few KB of stack, far less than the reporter
        // needs; its stack probe would fault again and the process would die
        // before the chained filter ran.
        const bool overflow = ep->ExceptionRecord->ExceptionCode == EXCEPTION_STACK_OVERFLOW;
        if (report != nullptr && !overflow && !tl_in_filter) {
            tl_in_filter = true;
            // Game crashes often come from heap corruption that the reporter can
            // trip over. A fault in it must not replace the original crash, or
            // the game's own filter would never see it.
            __try {
                report(ep);
            } __except (EXCEPTION_EXECUTE_HANDLER) {
            }
            tl_in_filter = false;
        }
        auto *previous = g_previous.load(std::memory_order_acquire);
        return previous != nullptr ? previous(ep) : EXCEPTION_CONTINUE_SEARCH;
    }
#pragma clang diagnostic pop

    auto install_unhandled_filter(CrashReporter report) -> bool {
        if (g_hook) {
            return true;
        }

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-function-type-strict"
        auto *target = reinterpret_cast<decltype(SetUnhandledExceptionFilter) *>(
            GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "SetUnhandledExceptionFilter"));
#pragma clang diagnostic pop
        if (target == nullptr) {
            return false;
        }

        // Created disarmed so the original is callable through g_hook before any
        // caller can reach the detour: register ours, capture what was there,
        // and only then arm.
        auto result = safetyhook::InlineHook::create(
            target,
            reinterpret_cast<void *>(&hk_set_unhandled_exception_filter),
            safetyhook::InlineHook::StartDisabled);
        if (!result) {
            return false;
        }
        g_hook = std::move(*result);

        g_report.store(report, std::memory_order_release);
        g_previous.store(g_hook.stdcall<LPTOP_LEVEL_EXCEPTION_FILTER>(&unhandled_filter),
                         std::memory_order_release);

        if (!g_hook.enable()) {
            g_hook.stdcall<LPTOP_LEVEL_EXCEPTION_FILTER>(g_previous.exchange(nullptr));
            g_report.store(nullptr, std::memory_order_release);
            g_hook.reset();
            return false;
        }
        return true;
    }

    void uninstall_unhandled_filter() {
        if (!g_hook) {
            return;
        }
        g_hook.reset();
        SetUnhandledExceptionFilter(g_previous.exchange(nullptr));
        g_report.store(nullptr, std::memory_order_release);
    }
} // namespace diagnostics
