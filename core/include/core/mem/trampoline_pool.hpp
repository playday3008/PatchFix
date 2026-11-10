#pragma once

#include <cstddef>
#include <cstdint>

#include <expected>
#include <string>

namespace mem::trampoline_pool {
    inline constexpr std::size_t k_size = 0x1'0000;

    /// Reserves k_size bytes within rel32 reach of both lo and hi and keeps them
    /// for later hooks, so their trampolines can use a 5-byte jump even once the
    /// rest of that range is taken. Returns the address of the reserved block.
    [[nodiscard]] auto reserve(std::uintptr_t lo, std::uintptr_t hi)
        -> std::expected<std::uintptr_t, std::string>;

    /// Runs reserve() over the main executable's image and stores the result.
    void reserve_near_main_image();

    /// Result of reserve_near_main_image(), or an error if it has not run.
    [[nodiscard]] auto main_image_reservation()
        -> const std::expected<std::uintptr_t, std::string> &;
} // namespace mem::trampoline_pool
