#pragma once

#include <cstdint>

#include <algorithm>
#include <memory>
#include <span>
#include <string>
#include <string_view>

#include "games/batman/ak/game_data.hpp"

#include "GameDefines.hpp"

namespace games::batman::ak::sdk {
    using Addrs = game_data<ArkhamKnight>::ResolvedAddresses;

    // The game's appRealloc(ptr, size, align).
    using ReallocFn = void *(*)(void *ptr, std::uint32_t size, std::uint32_t align);

    // Points the SDK's GObjects and GNames at the game's globals and stores the
    // game allocator. Call once, before any SDK object access.
    auto init(const Addrs &addrs) -> bool;

    namespace detail {
        // Layout of FString and TArray<T>, whose fields the SDK keeps private.
        struct RawArray {
            void        *data;
            std::int32_t num;
            std::int32_t max;
        };
        static_assert(sizeof(RawArray) == sizeof(FString));
        static_assert(sizeof(RawArray) == sizeof(TArray<FString>));

        inline auto raw(FString &s) -> RawArray & {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast): asserted layout above
            return reinterpret_cast<RawArray &>(s);
        }

        inline auto raw(TArray<FString> &a) -> RawArray & {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast): asserted layout above
            return reinterpret_cast<RawArray &>(a);
        }
    } // namespace detail

    // Builders for strings and arrays handed to the game. Every buffer comes
    // from `alloc`; the plugin never frees these buffers, the game may.
    inline auto make_fstring(ReallocFn alloc, std::wstring_view s) -> FString {
        const auto n   = static_cast<std::uint32_t>(s.size() + 1);
        auto      *buf = static_cast<wchar_t *>(alloc(nullptr, n * sizeof(wchar_t), 8));
        std::ranges::copy(s, buf);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic): terminator write
        buf[s.size()] = L'\0';

        FString out;
        detail::raw(out) = {.data = buf,
                            .num  = static_cast<std::int32_t>(n),
                            .max  = static_cast<std::int32_t>(n)};
        return out;
    }

    inline auto make_string_array(ReallocFn alloc, std::span<const std::wstring> items)
        -> TArray<FString> {
        TArray<FString> out;
        if (items.empty()) {
            return out;
        }

        const auto    n    = static_cast<std::uint32_t>(items.size());
        auto         *data = static_cast<FString *>(alloc(nullptr, n * sizeof(FString), 8));
        std::uint32_t i    = 0;
        for (const auto &item : items) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic): contiguous alloc
            std::construct_at(data + i, make_fstring(alloc, item));
            ++i;
        }
        detail::raw(out) = {.data = data,
                            .num  = static_cast<std::int32_t>(n),
                            .max  = static_cast<std::int32_t>(n)};
        return out;
    }

    // The same, using the game allocator stored by init.
    auto make_fstring(std::wstring_view s) -> FString;
    auto make_string_array(std::span<const std::wstring> items) -> TArray<FString>;

    // Frees a buffer from the game allocator stored by init (appRealloc to
    // size 0). Null is a no-op.
    void free_buffer(void *ptr);

    // Whether a lookup that failed last time should be retried, given the
    // current size of the table it failed against. GNames only grows, so a
    // lookup can only start succeeding once something new was added to it;
    // this lets a caller skip repeating a full scan on every call until
    // that actually happens.
    class RetryGate {
      public:
        [[nodiscard]] auto should_attempt(std::int32_t count) const -> bool {
            return !attempted_ || count != last_count_;
        }

        void record_failure(std::int32_t count) {
            attempted_  = true;
            last_count_ = count;
        }

      private:
        bool         attempted_  = false;
        std::int32_t last_count_ = 0;
    };

    // Resolves an FName once and caches the result. The lookup is a linear
    // scan of every registered name, so this retries a failed lookup only
    // when RetryGate says GNames has grown since the last attempt, rather
    // than on every call.
    class DeferredName {
      public:
        // component and name are used only in log messages, e.g. "TutorialLines", "GetLines".
        constexpr DeferredName(std::string_view component, std::string_view name) noexcept
            : component_(component),
              name_(name) {}

        // -1 while the name has not resolved yet.
        [[nodiscard]] auto index() -> int;

      private:
        std::string_view component_;
        std::string_view name_;
        RetryGate        gate_;
        int              index_       = -1;
        bool             logged_miss_ = false;
    };
} // namespace games::batman::ak::sdk
