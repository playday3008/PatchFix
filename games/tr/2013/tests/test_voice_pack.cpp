#include <catch2/catch_test_macros.hpp>

#include "games/tr/2013/hooks/voice_language.hpp"

using games::tr::tr2013::is_voice_pack;
using games::tr::tr2013::voice_bigfile;

TEST_CASE("is_voice_pack rejects the stub bigfiles", "[tr2013][voice]") {
    CHECK_FALSE(is_voice_pack(0));
    CHECK_FALSE(is_voice_pack(152'400));
    CHECK_FALSE(is_voice_pack(183'700));
    CHECK_FALSE(is_voice_pack((64ULL << 20U) - 1));
}

TEST_CASE("is_voice_pack accepts the full bigfiles", "[tr2013][voice]") {
    CHECK(is_voice_pack(64ULL << 20U));
    CHECK(is_voice_pack(1'067'450'000));
}

TEST_CASE("voice_bigfile names the file the menu checks", "[tr2013][voice]") {
    CHECK(voice_bigfile(0) == "BIGFILE_ENGLISH.000.tiger");
    CHECK(voice_bigfile(9) == "BIGFILE_RUSSIAN.000.tiger");
    CHECK(voice_bigfile(10) == "BIGFILE_CZECH.000.tiger");
    CHECK(voice_bigfile(15) == "BIGFILE_CHINESE.000.tiger");
    CHECK(voice_bigfile(5) == "Japanese\\BIGFILE_JAPANESE.000.tiger");
}

TEST_CASE("voice_bigfile has no file for ids outside the language table", "[tr2013][voice]") {
    CHECK(voice_bigfile(-1).empty());
    CHECK(voice_bigfile(16).empty());
}
