set(PROJECT_DISPLAY_NAME "Stutter")
set(PROJECT_DESCRIPTION "AI-assisted reverse engineering for Cutter")
set(PROJECT_REPO "https://github.com/gavrh/stutter")
set(PROJECT_AUTHOR "Gavin Holmes")

set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "Hide CMake deprecation warnings" FORCE)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/constants.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/constants.h"
)

