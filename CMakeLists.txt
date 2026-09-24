cmake_minimum_required(VERSION 3.16)
project(stutter-cutter-plugin VERSION 0.0.1 LANGUAGES CXX)

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

find_package(Cutter REQUIRED)
find_package(Rizin REQUIRED)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Network)
find_package(SQLite3 REQUIRED)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp"
)
file(GLOB_RECURSE RESOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/*.qrc"
)

add_library("${PROJECT_NAME}" MODULE ${SOURCES} ${HEADERS} ${RESOURCES})

set_target_properties("${PROJECT_NAME}" PROPERTIES
    AUTOMOC ON
    AUTORCC ON
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    PREFIX ""
)

target_include_directories(
    "${PROJECT_NAME}" PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_BINARY_DIR}/generated"
)
target_link_libraries(
    "${PROJECT_NAME}" PRIVATE
    Cutter::Cutter
    Rizin::Core
    Qt${QT_VERSION_MAJOR}::Network
    SQLite3::SQLite3
)

set(CUTTER_INSTALL_PLUGDIR "${Cutter_USER_PLUGINDIR}" CACHE STRING "Directory to install Cutter plugin into")
install(TARGETS "${PROJECT_NAME}" DESTINATION "${CUTTER_INSTALL_PLUGDIR}")
