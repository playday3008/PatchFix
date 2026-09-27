#include <string>

#include <catch2/catch_test_macros.hpp>

#include <mini/ini.h>

#include "games/batman/ak/path_field.hpp"

using games::batman::ak::path_field;

TEST_CASE("path_field starts at its default", "[batman-ak][path_field]") {
    const path_field field("DLCLoader", "CustomRoot", L"..\\..\\DLC\\Custom");
    CHECK(field.get() == L"..\\..\\DLC\\Custom");
}

TEST_CASE("path_field loads a configured value", "[batman-ak][path_field]") {
    mINI::INIStructure ini;
    ini["DLCLoader"]["CustomRoot"] = "D:\\Custom";

    path_field field("DLCLoader", "CustomRoot", L"..\\..\\DLC\\Custom");
    field.load_from(ini);
    CHECK(field.get() == L"D:\\Custom");
}

TEST_CASE("path_field decodes UTF-8", "[batman-ak][path_field]") {
    mINI::INIStructure ini;
    ini["DLCLoader"]["CustomRoot"] = "D:\\\xD0\x9C\xD0\xBE\xD0\xB4\xD0\xB8"; // "Modi" in Cyrillic

    path_field field("DLCLoader", "CustomRoot", L"x");
    field.load_from(ini);
    CHECK(field.get() == L"D:\\\u041C\u043E\u0434\u0438");
}

TEST_CASE("path_field trims and falls back on blank", "[batman-ak][path_field]") {
    mINI::INIStructure ini;
    ini["DLCLoader"]["CustomRoot"] = "   ";

    path_field field("DLCLoader", "CustomRoot", L"..\\..\\DLC\\Custom");
    field.load_from(ini);
    CHECK(field.get() == L"..\\..\\DLC\\Custom");

    ini["DLCLoader"]["CustomRoot"] = "  D:\\Custom  ";
    field.load_from(ini);
    CHECK(field.get() == L"D:\\Custom");
}

TEST_CASE("path_field returns to default when the key goes away", "[batman-ak][path_field]") {
    mINI::INIStructure ini;
    ini["DLCLoader"]["CustomRoot"] = "D:\\Custom";

    path_field field("DLCLoader", "CustomRoot", L"..\\..\\DLC\\Custom");
    field.load_from(ini);
    ini["DLCLoader"].remove("CustomRoot");
    field.load_from(ini);
    CHECK(field.get() == L"..\\..\\DLC\\Custom");
}
