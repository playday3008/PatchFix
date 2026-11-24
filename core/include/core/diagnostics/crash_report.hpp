#pragma once

#include <cstdint>

#include <string_view>

#include <Windows.h>

namespace diagnostics {
    /// Instruction, stack and frame pointer of a CONTEXT, for either architecture.
    [[nodiscard]] inline auto context_ip(const CONTEXT *ctx) -> std::uintptr_t {
#ifdef _M_IX86
        return static_cast<std::uintptr_t>(ctx->Eip);
#else
        return static_cast<std::uintptr_t>(ctx->Rip);
#endif
    }
    [[nodiscard]] inline auto context_sp(const CONTEXT *ctx) -> std::uintptr_t {
#ifdef _M_IX86
        return static_cast<std::uintptr_t>(ctx->Esp);
#else
        return static_cast<std::uintptr_t>(ctx->Rsp);
#endif
    }
    [[nodiscard]] inline auto context_bp(const CONTEXT *ctx) -> std::uintptr_t {
#ifdef _M_IX86
        return static_cast<std::uintptr_t>(ctx->Ebp);
#else
        return static_cast<std::uintptr_t>(ctx->Rbp);
#endif
    }

    auto exception_code_name(std::uint32_t code) -> std::string_view;
    auto is_hardware_exception(std::uint32_t code) -> bool;

    [[nodiscard]] auto callback_fault_filter(EXCEPTION_POINTERS *ep,
                                             std::string_view hook_name = "hook callback") -> int;
    [[nodiscard]] auto install_fault_filter(EXCEPTION_POINTERS *ep, std::string_view hook_name)
        -> int;

    void log_crash_report(EXCEPTION_POINTERS *ep, std::string_view context_name);
    void log_crash_report_lightweight(EXCEPTION_POINTERS *ep);
    void log_patch_attribution(EXCEPTION_POINTERS *ep);
} // namespace diagnostics
