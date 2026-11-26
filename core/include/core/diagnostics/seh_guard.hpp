#pragma once

#include <atomic>
#include <concepts>
#include <string_view>
#include <type_traits>

#include <Windows.h>

#include <safetyhook/context.hpp>

#include "core/logger.hpp" // IWYU pragma: keep

#include "core/diagnostics/crash_report.hpp"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wlanguage-extension-token"

namespace diagnostics {
    // A hook callback is default-constructed and called inside __try, which
    // cannot unwind a destructor; its name labels the crash report and log.
    template<typename F>
    concept HookCallback = std::default_initializable<F> && std::is_trivially_destructible_v<F> &&
                           requires(F f, safetyhook::Context &regs) {
                               { F::name } -> std::convertible_to<std::string_view>;
                               f(regs);
                           };

    template<HookCallback Functor>
    void guarded_callback(safetyhook::Context &regs) {
        static std::atomic<bool> s_faulted {false};
        if (s_faulted.load(std::memory_order_relaxed)) {
            return;
        }
        __try {
            Functor {}(regs);
        } __except (callback_fault_filter(GetExceptionInformation(), Functor::name)) {
            s_faulted.store(true, std::memory_order_relaxed);
            log::get(Functor::name)->critical("Hook callback crashed — permanently disabled");
        }
    }

    auto guarded_install(bool (*fn)(const void *), const void *addrs, std::string_view hook_name)
        -> bool;
} // namespace diagnostics

#pragma clang diagnostic pop
