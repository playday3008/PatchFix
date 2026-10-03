#include <catch2/catch_test_macros.hpp>

#include "games/batman/ak/sdk.hpp"

using games::batman::ak::sdk::RetryGate;

TEST_CASE("RetryGate allows the first attempt at any count", "[batman-ak][sdk]") {
    const RetryGate gate;
    CHECK(gate.should_attempt(0));
    CHECK(gate.should_attempt(42));
}

TEST_CASE("RetryGate blocks a retry while the count is unchanged", "[batman-ak][sdk]") {
    RetryGate gate;
    gate.record_failure(10);
    CHECK_FALSE(gate.should_attempt(10));
}

TEST_CASE("RetryGate allows a retry once the count changes", "[batman-ak][sdk]") {
    RetryGate gate;
    gate.record_failure(10);
    CHECK(gate.should_attempt(11));
    CHECK(gate.should_attempt(9));
}

TEST_CASE("RetryGate tracks the count from the most recent failure", "[batman-ak][sdk]") {
    RetryGate gate;
    gate.record_failure(10);
    gate.record_failure(11);
    CHECK_FALSE(gate.should_attempt(11));
    CHECK(gate.should_attempt(12));
}
