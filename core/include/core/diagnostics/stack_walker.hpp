#pragma once

#include <cstddef>
#include <cstdint>

#include <array>
#include <span>
#include <string>

#include <Windows.h>

namespace diagnostics {
    inline constexpr std::size_t k_max_frames  = 64;
    inline constexpr std::size_t k_max_sym_len = 256;

    struct StackFrame {
        std::uintptr_t                  address {};
        std::uintptr_t                  module_base {};
        std::uintptr_t                  module_offset {};
        std::string                     module_name;
        std::array<char, k_max_sym_len> symbol_name {};
        std::uintptr_t                  symbol_offset {};
        bool                            has_symbol {};
    };

    auto capture_stack(const CONTEXT *ctx, std::span<StackFrame> buf) -> std::span<StackFrame>;
    void resolve_modules(std::span<StackFrame> frames);

    // The file name of `module` as UTF-8, or empty on failure.
    auto module_file_name(HMODULE module) -> std::string;
    void resolve_symbols(std::span<StackFrame> frames);
} // namespace diagnostics
