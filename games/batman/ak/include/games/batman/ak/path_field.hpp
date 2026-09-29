#pragma once

#include <cstddef>

#include <mutex>
#include <string>
#include <string_view>
#include <utility>

#include <Windows.h>

#include <mini/ini.h>

#include "core/hooks/registry/parsers.hpp"

namespace games::batman::ak {
    // A string config field. ini_field cannot hold a string because it wraps
    // std::atomic<T>. The INI is UTF-8; a blank value means the default.
    class path_field {
      public:
        path_field(std::string_view section, std::string_view key, std::wstring_view default_value)
            : section_(section),
              key_(key),
              default_(default_value),
              value_(default_value) {}

        auto get() const -> std::wstring {
            const std::scoped_lock lock(mutex_);
            return value_;
        }

        void load_from(mINI::INIStructure &ini) {
            const auto        sec = ini.get(std::string(section_));
            const std::string k(key_);
            const std::string raw  = sec.get(k);
            auto              text = sec.has(k) ? hooks::detail::trim(raw) : std::string_view {};
            if (text.size() >= 2 && text.front() == '"' && text.back() == '"') {
                text = hooks::detail::trim(text.substr(1, text.size() - 2));
            }
            std::wstring val = text.empty() ? default_ : widen(text);

            const std::scoped_lock lock(mutex_);
            value_ = std::move(val);
        }

      private:
        static auto widen(std::string_view text) -> std::wstring {
            const int    len = MultiByteToWideChar(CP_UTF8,
                                                   0,
                                                   text.data(),
                                                   static_cast<int>(text.size()),
                                                   nullptr,
                                                   0);
            std::wstring out(static_cast<std::size_t>(len), L'\0');
            MultiByteToWideChar(CP_UTF8,
                                0,
                                text.data(),
                                static_cast<int>(text.size()),
                                out.data(),
                                len);
            return out;
        }

        std::string_view   section_;
        std::string_view   key_;
        std::wstring       default_;
        mutable std::mutex mutex_;
        std::wstring       value_;
    };
} // namespace games::batman::ak
