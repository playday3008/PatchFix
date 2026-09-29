#include <catch2/catch_test_macros.hpp>

#include "games/batman/ak/subscribers.hpp"

using games::batman::ak::Subscribers;

namespace {
    int s_calls_a    = 0;
    int s_calls_b    = 0;
    int s_order_a    = -1;
    int s_order_b    = -1;
    int s_next_order = 0;

    void handler_a(UObject *, UFunction *, void *) {
        ++s_calls_a;
        s_order_a = s_next_order++;
    }

    void handler_b(UObject *, UFunction *, void *) {
        ++s_calls_b;
        s_order_b = s_next_order++;
    }
} // namespace

TEST_CASE("subscribers are called in order, once per dispatch", "[batman-ak][subscribers]") {
    s_calls_a = s_calls_b = 0;
    s_order_a = s_order_b = -1;
    s_next_order          = 0;
    Subscribers<2> subs;

    CHECK(subs.add(&handler_a));
    CHECK(subs.add(&handler_b));
    subs.dispatch(nullptr, nullptr, nullptr);

    CHECK(s_calls_a == 1);
    CHECK(s_calls_b == 1);
    CHECK(s_order_a < s_order_b);
}

TEST_CASE("subscribe refuses past capacity", "[batman-ak][subscribers]") {
    s_calls_a = s_calls_b = 0;
    Subscribers<1> subs;

    CHECK(subs.add(&handler_a));
    CHECK_FALSE(subs.add(&handler_b));
    subs.dispatch(nullptr, nullptr, nullptr);

    CHECK(s_calls_a == 1);
    CHECK(s_calls_b == 0);
}
