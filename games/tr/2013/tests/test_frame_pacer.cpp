#include <limits>

#include <catch2/catch_test_macros.hpp>

#include "games/tr/2013/hooks/frame_limiter.hpp"

using games::tr::tr2013::FramePacer;

TEST_CASE("FramePacer does nothing without a period", "[tr2013][limiter]") {
    FramePacer pacer;
    CHECK(pacer.due(1000, 0) == 0);
    CHECK(pacer.due(1001, 0) == 0);
    CHECK(pacer.due(1002, -5) == 0);
}

TEST_CASE("FramePacer waits out the rest of the period", "[tr2013][limiter]") {
    FramePacer pacer;
    CHECK(pacer.due(1000, 100) == 0);
    CHECK(pacer.due(1040, 100) == 60);
    CHECK(pacer.due(1100, 100) == 100);
}

TEST_CASE("FramePacer keeps a steady cadence when frames arrive late", "[tr2013][limiter]") {
    FramePacer pacer;
    CHECK(pacer.due(0, 100) == 0);
    CHECK(pacer.due(100, 100) == 0);
    CHECK(pacer.due(205, 100) == 0);
    CHECK(pacer.due(250, 100) == 50);
}

TEST_CASE("FramePacer resyncs after a stall instead of bursting", "[tr2013][limiter]") {
    FramePacer pacer;
    CHECK(pacer.due(0, 100) == 0);
    CHECK(pacer.due(5000, 100) == 0);
    CHECK(pacer.due(5040, 100) == 60);
}

TEST_CASE("FramePacer resyncs when the period changes", "[tr2013][limiter]") {
    FramePacer pacer;
    CHECK(pacer.due(0, 100) == 0);
    CHECK(pacer.due(10, 1000) == 0);
    CHECK(pacer.due(20, 1000) == 990);
}

TEST_CASE("FramePacer starts fresh after being turned off", "[tr2013][limiter]") {
    FramePacer pacer;
    CHECK(pacer.due(0, 100) == 0);
    CHECK(pacer.due(50, 0) == 0);
    CHECK(pacer.due(60, 100) == 0);
    CHECK(pacer.due(100, 100) == 60);
}

TEST_CASE("frame_period is off for caps below 1 and for NaN", "[tr2013][limiter]") {
    using games::tr::tr2013::frame_period;
    CHECK(frame_period(0.0F, 10'000'000) == 0);
    CHECK(frame_period(0.5F, 10'000'000) == 0);
    CHECK(frame_period(-60.0F, 10'000'000) == 0);
    CHECK(frame_period(std::numeric_limits<float>::quiet_NaN(), 10'000'000) == 0);
}

TEST_CASE("frame_period raises caps below 15 to 15", "[tr2013][limiter]") {
    using games::tr::tr2013::frame_period;
    CHECK(frame_period(1.0F, 15'000'000) == 1'000'000);
    CHECK(frame_period(14.9F, 15'000'000) == 1'000'000);
    CHECK(frame_period(15.0F, 15'000'000) == 1'000'000);
}

TEST_CASE("frame_period divides the clock rate by the cap", "[tr2013][limiter]") {
    using games::tr::tr2013::frame_period;
    CHECK(frame_period(60.0F, 10'000'000) == 166'666);
    CHECK(frame_period(144.0F, 10'000'000) == 69'444);
    CHECK(frame_period(std::numeric_limits<float>::infinity(), 10'000'000) == 0);
}
