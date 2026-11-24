#pragma once

#include <Windows.h>

namespace diagnostics {
    using CrashReporter = void (*)(EXCEPTION_POINTERS *ep);

    /// Makes unhandled_filter the process top-level exception filter and keeps it
    /// there by hooking SetUnhandledExceptionFilter. Filters the game installs,
    /// before or after this call, are chained to after `report` runs. Returns
    /// false when the hook cannot be created. A second call is a no-op.
    [[nodiscard]] auto install_unhandled_filter(CrashReporter report) -> bool;

    /// Removes the hook and hands the top-level filter back to the last one the
    /// game installed. No-op when not installed.
    void uninstall_unhandled_filter();

    /// The registered top-level filter: reports once per thread, then returns
    /// what the chained filter returns, or EXCEPTION_CONTINUE_SEARCH without one.
    auto WINAPI unhandled_filter(EXCEPTION_POINTERS *ep) -> LONG;
} // namespace diagnostics
