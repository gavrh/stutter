cmake_minimum_required(VERSION 3.12)
project(stutter-cutter-plugin)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Cutter REQUIRED)
find_package(Rizin REQUIRED)
set(CUTTER_INSTALL_PLUGDIR "${Cutter_USER_PLUGINDIR}" CACHE STRING "Directory to install Cutter plugin into")

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
)

add_library("${PROJECT_NAME}" MODULE ${SOURCES} ${HEADERS})

set_target_properties("${PROJECT_NAME}" PROPERTIES
    AUTOMOC ON
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    PREFIX ""
)

target_include_directories("${PROJECT_NAME}" PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries("${PROJECT_NAME}" PRIVATE Cutter::Cutter Rizin::Core)
install(TARGETS "${PROJECT_NAME}" DESTINATION "${CUTTER_INSTALL_PLUGDIR}")
