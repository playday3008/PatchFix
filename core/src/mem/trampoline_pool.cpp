#include "core/mem/trampoline_pool.hpp"

#include <cstdint>

#include <algorithm>
#include <expected>
#include <memory>
#include <string>

#include <Windows.h>

#include <safetyhook/allocator.hpp>

#include "core/win32/pe.hpp"

namespace mem::trampoline_pool {
    namespace {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wglobal-constructors"
        // SafetyHook's global allocator lives only while something owns it, and
        // freed blocks stay reserved only while it lives. Holding it here keeps the
        // pool for every hook created later.
        std::shared_ptr<safetyhook::Allocator> g_allocator;

        auto main_image_result() -> std::expected<std::uintptr_t, std::string> & {
            static std::expected<std::uintptr_t, std::string> instance =
                std::unexpected(std::string("not attempted"));
            return instance;
        }
#pragma clang diagnostic pop

        auto allocator_error_name(safetyhook::Allocator::Error err) -> std::string {
            switch (err) {
                case safetyhook::Allocator::Error::BAD_VIRTUAL_ALLOC:
                    return "BAD_VIRTUAL_ALLOC";
                case safetyhook::Allocator::Error::NO_MEMORY_IN_RANGE:
                    return "NO_MEMORY_IN_RANGE";
            }
            return "unknown allocator error";
        }
    } // namespace

    auto reserve(std::uintptr_t lo, std::uintptr_t hi)
        -> std::expected<std::uintptr_t, std::string> {
        g_allocator = safetyhook::Allocator::global();

        // shortcut: one block shared by every hook (~450 bytes per mid hook, so
        // ~140 hooks); raise k_size if a game approaches that.
        auto block = g_allocator->allocate_near({reinterpret_cast<std::uint8_t *>(lo),
                                                 reinterpret_cast<std::uint8_t *>(hi)},
                                                k_size);
        if (!block) {
            return std::unexpected(allocator_error_name(block.error()));
        }
        // Destroying the allocation returns it to the allocator's freelist; the
        // underlying pages stay reserved for the next near allocation.
        return block->address();
    }

    void reserve_near_main_image() {
        const auto lo       = reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
        const auto sections = win32::enumerate_sections(nullptr);
        if (sections.empty()) {
            main_image_result() = std::unexpected(std::string("main image has no sections"));
            return;
        }
        const auto hi = std::ranges::max(sections, {}, &win32::SectionInfo::end).end();

        main_image_result() = reserve(lo, hi);
    }

    auto main_image_reservation() -> const std::expected<std::uintptr_t, std::string> & {
        return main_image_result();
    }
} // namespace mem::trampoline_pool
