#pragma once

#include <cstddef>

#include <array>
#include <atomic>

class UObject;
class UFunction;

namespace games::batman::ak {
    // Usually called on the game thread before UObject::ProcessEvent runs.
    // `fn` may be null. Handlers must be cheap: compare cached pointers or
    // FName indices, never GetFullName strings.
    using Handler = void (*)(UObject *self, UFunction *fn, void *params);

    // Append-only handler list. add() may run while dispatch() runs on another
    // thread: the slot is written before the count that publishes it.
    // add() itself is single-writer: concurrent add() calls are not supported,
    // only one caller may add() at a time (concurrent with dispatch() is fine).
    template<std::size_t Capacity>
    class Subscribers {
      public:
        auto add(Handler handler) -> bool {
            const auto n = count_.load(std::memory_order_relaxed);
            if (n == Capacity) {
                return false;
            }
            handlers_.at(n) = handler;
            count_.store(n + 1, std::memory_order_release);
            return true;
        }

        void dispatch(UObject *self, UFunction *fn, void *params) const {
            const auto n = count_.load(std::memory_order_acquire);
            for (std::size_t i = 0; i < n; ++i) {
                handlers_.at(i)(self, fn, params);
            }
        }

      private:
        std::array<Handler, Capacity> handlers_ {};
        std::atomic<std::size_t>      count_ {0};
    };
} // namespace games::batman::ak
