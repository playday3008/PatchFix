project(Batman.ArkhamKnight
    LANGUAGES CXX
    VERSION 0.1.0
)

include("${CMAKE_SOURCE_DIR}/cmake/deps/batmanak_sdk.cmake")

patchfix_add_game(ARCH x64)
if(TARGET batman-ak)
    target_link_libraries(batman-ak PRIVATE BmAK::UDK)
    if(TARGET PatchFix.Tests)
        target_link_libraries(PatchFix.Tests PRIVATE BmAK::UDK)
    endif()
endif()
