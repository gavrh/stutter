add_library("${PROJECT_NAME}" MODULE ${SOURCES} ${HEADERS})

set_target_properties("${PROJECT_NAME}" PROPERTIES
    AUTOMOC ON
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    PREFIX ""
)

target_include_directories(
    "${PROJECT_NAME}" PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_BINARY_DIR}/generated"
)
target_link_libraries(
    "${PROJECT_NAME}" PRIVATE
    Cutter::Cutter
    Rizin::Core
    toml11::toml11
)

set(CUTTER_INSTALL_PLUGDIR "${Cutter_USER_PLUGINDIR}" CACHE STRING "Directory to install Cutter plugin into")
install(TARGETS "${PROJECT_NAME}" DESTINATION "${CUTTER_INSTALL_PLUGDIR}")
