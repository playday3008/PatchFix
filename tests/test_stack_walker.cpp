#include <cstddef>

#include <array>
#include <span>

#include <Windows.h>

#include <catch2/catch_test_macros.hpp>

#include "core/diagnostics/stack_walker.hpp"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wlanguage-extension-token"

namespace {
    // Walks from a faulting CONTEXT, as the crash handler does. The filter runs
    // before unwinding, so the raising frames are still on the stack.
    std::span<diagnostics::StackFrame> g_buf;
    std::size_t                        g_captured = 0;

    auto filter(EXCEPTION_POINTERS *ep) -> int {
        g_captured = diagnostics::capture_stack(ep->ContextRecord, g_buf).size();
        return EXCEPTION_EXECUTE_HANDLER;
    }

    [[gnu::noinline]] void raise_and_capture() {
        __try {
            RaiseException(0xE0000001, 0, 0, nullptr);
        } __except (filter(GetExceptionInformation())) {
        }
    }

    // Two more real frames between the raise and the test body.
    volatile int g_sink = 0;

    [[gnu::noinline]] void level2() {
        raise_and_capture();
        g_sink = g_sink + 1;
    }

    [[gnu::noinline]] void level1() {
        level2();
        g_sink = g_sink + 1;
    }

    auto capture_into(std::span<diagnostics::StackFrame> buf) -> std::size_t {
        g_buf      = buf;
        g_captured = 0;
        level1();
        return g_captured;
    }
} // namespace

#pragma clang diagnostic pop

TEST_CASE("capture_stack walks past the faulting frame", "[stack_walker]") {
    std::array<diagnostics::StackFrame, diagnostics::k_max_frames> buf {};
    auto                                                           n = capture_into(buf);

    REQUIRE(n >= 4);
    for (std::size_t i = 0; i < n; ++i) {
        CHECK(buf.at(i).address != 0);
    }
}

TEST_CASE("capture_stack stops at the buffer size", "[stack_walker]") {
    std::array<diagnostics::StackFrame, 2> buf {};

    CHECK(capture_into(buf) == 2);
}
