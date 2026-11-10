#include "core/mem/hook.hpp"

#include <cstdint>

#include <expected>
#include <format>
#include <string>
#include <string_view>
#include <utility>

#include <safetyhook/allocator.hpp>
#include <safetyhook/inline_hook.hpp>
#include <safetyhook/mid_hook.hpp>

namespace mem {
    namespace {
        auto allocator_error_name(safetyhook::Allocator::Error err) -> std::string_view {
            switch (err) {
                case safetyhook::Allocator::Error::BAD_VIRTUAL_ALLOC:
                    return "BAD_VIRTUAL_ALLOC";
                case safetyhook::Allocator::Error::NO_MEMORY_IN_RANGE:
                    return "NO_MEMORY_IN_RANGE";
            }
            return "unknown";
        }

        auto inline_error_name(const safetyhook::InlineHook::Error &err) -> std::string_view {
            using E = safetyhook::InlineHook::Error;
            switch (err.type) {
                case E::BAD_ALLOCATION:
                    return "BAD_ALLOCATION";
                case E::FAILED_TO_DECODE_INSTRUCTION:
                    return "FAILED_TO_DECODE_INSTRUCTION";
                case E::SHORT_JUMP_IN_TRAMPOLINE:
                    return "SHORT_JUMP_IN_TRAMPOLINE";
                case E::IP_RELATIVE_INSTRUCTION_OUT_OF_RANGE:
                    return "IP_RELATIVE_INSTRUCTION_OUT_OF_RANGE";
                case E::UNSUPPORTED_INSTRUCTION_IN_TRAMPOLINE:
                    return "UNSUPPORTED_INSTRUCTION_IN_TRAMPOLINE";
                case E::FAILED_TO_UNPROTECT:
                    return "FAILED_TO_UNPROTECT";
                case E::NOT_ENOUGH_SPACE:
                    return "NOT_ENOUGH_SPACE";
                default:
                    return "unknown";
            }
        }

        // SafetyHook tries a rel32 jump first and falls back to an absolute one only
        // when the rel32 trampoline cannot be placed. The fallback's error is the one
        // returned, so re-run the near allocation to report why the first path failed.
        auto near_allocation_status(std::uintptr_t addr) -> std::string {
            auto probe = safetyhook::Allocator::global()->allocate_near(
                {reinterpret_cast<std::uint8_t *>(addr)},
                1);
            if (!probe) {
                return std::format("near allocation failed: {}",
                                   allocator_error_name(probe.error()));
            }
            return std::format("near allocation ok at 0x{:X}", probe->address());
        }

        // SafetyHook's error structs are tagged unions; each read below is gated on the tag.
        // NOLINTBEGIN(cppcoreguidelines-pro-type-union-access)
        auto describe_inline_error(std::uintptr_t addr, const safetyhook::InlineHook::Error &err)
            -> std::string {
            if (err.type == safetyhook::InlineHook::Error::BAD_ALLOCATION) {
                return std::format("MidHook: inline hook failed: trampoline allocation ({})",
                                   allocator_error_name(err.allocator_error));
            }
            auto msg = std::format("MidHook: inline hook failed: {} at 0x{:X}",
                                   inline_error_name(err),
                                   reinterpret_cast<std::uintptr_t>(err.ip));
            if (err.type == safetyhook::InlineHook::Error::IP_RELATIVE_INSTRUCTION_OUT_OF_RANGE) {
                msg += std::format(" ({})", near_allocation_status(addr));
            }
            return msg;
        }
    } // namespace

    auto MidHook::create(std::uintptr_t addr, void (*callback)(Registers &))
        -> std::expected<MidHook, std::string> {
        auto result = safetyhook::MidHook::create(reinterpret_cast<void *>(addr), callback);
        if (!result) {
            auto &err = result.error();
            switch (err.type) {
                case safetyhook::MidHook::Error::BAD_ALLOCATION:
                    return std::unexpected(std::format("MidHook: stub allocation failed ({})",
                                                       allocator_error_name(err.allocator_error)));
                case safetyhook::MidHook::Error::BAD_INLINE_HOOK:
                    return std::unexpected(describe_inline_error(addr, err.inline_hook_error));
                default:
                    return std::unexpected(std::string("MidHook: unknown error"));
            }
        }
        MidHook hook;
        hook.inner_ = std::move(*result);
        return hook;
    }
    // NOLINTEND(cppcoreguidelines-pro-type-union-access)
} // namespace mem
