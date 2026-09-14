include(FetchContent)

find_package(Cutter REQUIRED)
find_package(Rizin REQUIRED)

find_package(toml11 QUIET)
if(NOT toml11_FOUND)
    FetchContent_Declare(
        toml11
        GIT_REPOSITORY https://github.com/ToruNiina/toml11.git
        GIT_TAG v4.4.0
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(toml11)
endif()
