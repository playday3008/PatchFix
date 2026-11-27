#pragma once

#include <cstddef>
#include <cstdint>

#include <array>
#include <optional>
#include <string>
#include <string_view>

#include "core/hooks/registry/config_base.hpp"
#include "core/hooks/registry/dep_list.hpp"
#include "core/hooks/registry/hook_traits.hpp"

#include "games/tr/2013/game_data.hpp"

namespace games::tr::tr2013 {
    struct VoiceLanguageHook {};

    // Languages without voice-over ship a bigfile of under 200 KB, full voice
    // packs about 1 GB.
    inline constexpr std::uint64_t k_min_voice_pack_bytes = 64ULL << 20U;

    [[nodiscard]] constexpr auto is_voice_pack(std::uint64_t size) -> bool {
        return size >= k_min_voice_pack_bytes;
    }

    // The bigfile the options menu checks for voice language `id`, relative to
    // the game directory, or empty for an id outside the game's language table.
    // Japanese lives in its own directory, which the game also uses for every
    // language when `japanese_dir` (set for a Japanese user locale) is on.
    [[nodiscard]] inline auto voice_bigfile(int id, bool japanese_dir) -> std::string {
        static constexpr auto k_names = std::to_array<std::string_view>({
            "ENGLISH",
            "FRENCH",
            "GERMAN",
            "ITALIAN",
            "SPANISH",
            "JAPANESE",
            "PORTUGUESE",
            "POLISH",
            "UKENGLISH",
            "RUSSIAN",
            "CZECH",
            "DUTCH",
            "HUNGARIAN",
            "ARABIC",
            "KOREAN",
            "CHINESE",
        });
        if (id < 0 || static_cast<std::size_t>(id) >= k_names.size()) {
            return {};
        }
        std::string path = id == 5 || japanese_dir ? "Japanese\\" : "";
        path += "BIGFILE_";
        path += k_names.at(static_cast<std::size_t>(id));
        path += ".000.tiger";
        return path;
    }
} // namespace games::tr::tr2013

namespace hooks {
    template<>
    struct HookTraits<games::tr::tr2013::VoiceLanguageHook> {
        using Addrs        = games::game_data<games::tr::TR2013>::ResolvedAddresses;
        using PatternField = std::optional<std::uintptr_t> Addrs::*;

        static constexpr std::string_view name = "VoiceLanguage";

        using hard_deps = dep_list<>;
        using soft_deps = dep_list<>;

        static constexpr auto required_patterns = std::array<PatternField, 2> {
            &Addrs::vo_menu_apply,
            &Addrs::settings_getter_call,
        };
        static constexpr auto optional_patterns = std::array<PatternField, 3> {
            &Addrs::vo_menu_row_lookup,
            &Addrs::vo_menu_list_check,
            &Addrs::vo_japanese_dir_flag,
        };

        using Config = empty_config;

        static auto install(const Addrs &addrs) -> bool;
    };
} // namespace hooks
