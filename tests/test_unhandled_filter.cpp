#include <tuple>

#include <Windows.h>

#include <catch2/catch_test_macros.hpp>

#include "core/diagnostics/unhandled_filter.hpp"

namespace {
    int g_reports = 0;
    int g_a_calls = 0;
    int g_b_calls = 0;
    int g_depth   = 0;

    EXCEPTION_POINTERS *g_a_ep = nullptr;

    auto WINAPI fake_a(EXCEPTION_POINTERS *ep) -> LONG {
        ++g_a_calls;
        g_a_ep = ep;
        return EXCEPTION_EXECUTE_HANDLER;
    }

    auto WINAPI fake_b(EXCEPTION_POINTERS * /*ep*/) -> LONG {
        ++g_b_calls;
        return EXCEPTION_EXECUTE_HANDLER;
    }

    void count_report(EXCEPTION_POINTERS * /*ep*/) {
        ++g_reports;
    }

    // Stands in for a reporter that faults, or a chained filter that calls
    // back into the top-level filter on the same thread.
    void reentrant_report(EXCEPTION_POINTERS *ep) {
        ++g_reports;
        if (g_depth++ == 0) {
            std::ignore = diagnostics::unhandled_filter(ep);
        }
    }

    void faulting_report(EXCEPTION_POINTERS * /*ep*/) {
        ++g_reports;
        RaiseException(EXCEPTION_ACCESS_VIOLATION, 0, 0, nullptr);
    }

    struct Installed {
        explicit Installed(diagnostics::CrashReporter report) {
            g_reports = g_a_calls = g_b_calls = g_depth = 0;
            g_a_ep                                      = nullptr;
            REQUIRE(diagnostics::install_unhandled_filter(report));
        }
        ~Installed() {
            diagnostics::uninstall_unhandled_filter();
            SetUnhandledExceptionFilter(nullptr);
        }
        Installed(const Installed &)                     = delete;
        Installed(Installed &&)                          = delete;
        auto operator=(const Installed &) -> Installed & = delete;
        auto operator=(Installed &&) -> Installed &      = delete;
    };

    class FakeException {
      public:
        explicit FakeException(DWORD code = EXCEPTION_ACCESS_VIOLATION) {
            m_record.ExceptionCode     = code;
            m_pointers.ExceptionRecord = &m_record;
            m_pointers.ContextRecord   = &m_context;
        }

        auto get() -> EXCEPTION_POINTERS * { return &m_pointers; }

      private:
        EXCEPTION_RECORD   m_record {};
        CONTEXT            m_context {};
        EXCEPTION_POINTERS m_pointers {};
    };
} // namespace

TEST_CASE("chains to a filter set before install", "[unhandled_filter]") {
    SetUnhandledExceptionFilter(fake_a);
    const Installed installed(count_report);
    FakeException   ex;

    CHECK(diagnostics::unhandled_filter(ex.get()) == EXCEPTION_EXECUTE_HANDLER);
    CHECK(g_reports == 1);
    CHECK(g_a_calls == 1);
}

TEST_CASE("the hook keeps ours on top and chains to the latest filter", "[unhandled_filter]") {
    SetUnhandledExceptionFilter(nullptr);
    const Installed installed(count_report);
    FakeException   ex;

    CHECK(SetUnhandledExceptionFilter(fake_a) == nullptr);
    CHECK(SetUnhandledExceptionFilter(fake_b) == &fake_a);

    CHECK(diagnostics::unhandled_filter(ex.get()) == EXCEPTION_EXECUTE_HANDLER);
    CHECK(g_b_calls == 1);
    CHECK(g_a_calls == 0);
}

TEST_CASE("a null previous filter continues the search", "[unhandled_filter]") {
    const Installed installed(count_report);
    FakeException   ex;

    SetUnhandledExceptionFilter(nullptr);

    CHECK(diagnostics::unhandled_filter(ex.get()) == EXCEPTION_CONTINUE_SEARCH);
    CHECK(g_reports == 1);
}

TEST_CASE("re-entry skips the report but still chains", "[unhandled_filter]") {
    const Installed installed(reentrant_report);
    FakeException   ex;

    SetUnhandledExceptionFilter(fake_a);

    CHECK(diagnostics::unhandled_filter(ex.get()) == EXCEPTION_EXECUTE_HANDLER);
    CHECK(g_reports == 1);
    CHECK(g_a_calls == 2);
}

TEST_CASE("uninstall restores the previous filter", "[unhandled_filter]") {
    {
        const Installed installed(count_report);
        SetUnhandledExceptionFilter(fake_a);
        diagnostics::uninstall_unhandled_filter();

        CHECK(SetUnhandledExceptionFilter(nullptr) == &fake_a);
    }
}

TEST_CASE("a stack overflow skips the report but still chains", "[unhandled_filter]") {
    const Installed installed(count_report);
    FakeException   ex(EXCEPTION_STACK_OVERFLOW);

    SetUnhandledExceptionFilter(fake_a);

    CHECK(diagnostics::unhandled_filter(ex.get()) == EXCEPTION_EXECUTE_HANDLER);
    CHECK(g_reports == 0);
    CHECK(g_a_calls == 1);
}

TEST_CASE("a faulting reporter still chains the original exception", "[unhandled_filter]") {
    const Installed installed(faulting_report);
    FakeException   ex;

    SetUnhandledExceptionFilter(fake_a);

    CHECK(diagnostics::unhandled_filter(ex.get()) == EXCEPTION_EXECUTE_HANDLER);
    CHECK(g_reports == 1);
    CHECK(g_a_calls == 1);
    CHECK(g_a_ep == ex.get());
}
