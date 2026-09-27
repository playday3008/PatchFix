include_guard(GLOBAL)
include(FetchContent)

# The CodeRed-generated Batman: Arkham Knight SDK as a static library.
# Point FETCHCONTENT_SOURCE_DIR_BMAK-UDK at a local checkout to build against
# a regenerated SDK.
FetchContent_Declare(BmAK-UDK
    GIT_REPOSITORY https://github.com/playday3008/BmAK-UDK.git
    GIT_TAG        fbf7830ff31aa931cb15207f574fd61d7bca6571
    GIT_PROGRESS   TRUE
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(BmAK-UDK)
