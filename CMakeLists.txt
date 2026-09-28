cmake_minimum_required(VERSION 3.20)

project(
    COLA_Julia
    VERSION 0.2.0
    LANGUAGES CXX
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(CTest)
include(CMakePackageConfigHelpers)

find_package(COLA CONFIG REQUIRED)
set(CMAKE_INSTALL_PREFIX "${COLA_DIR}")
include(GNUInstallDirs)
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/cola-prefix.txt" "${COLA_DIR}")
find_program(Julia_EXECUTABLE NAMES julia REQUIRED)

find_package(JlCxx CONFIG REQUIRED)

set(COLA_MODULE_NAME COLA_Julia)
add_subdirectory(bindings)
add_library(${COLA_MODULE_NAME} SHARED COLA_Julia/JuliaFilters.cc COLA_Julia/Module.cc)
target_compile_features(${COLA_MODULE_NAME} PRIVATE cxx_std_20)
target_include_directories(
    ${COLA_MODULE_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
                               $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(
    ${COLA_MODULE_NAME}
    PUBLIC COLA
    PRIVATE JlCxx::cxxwrap_julia JlCxx::cxxwrap_julia_stl
)
add_dependencies(${COLA_MODULE_NAME} COLA_JuliaBindings)
set_target_properties(
    ${COLA_MODULE_NAME}
    PROPERTIES VERSION "${PROJECT_VERSION}"
               SOVERSION "${PROJECT_VERSION_MAJOR}"
               CXX_CLANG_TIDY "${COLA_CMAKE_CXX_CLANG_TIDY}"
               BUILD_WITH_INSTALL_RPATH TRUE
               INSTALL_RPATH "${Julia_LIBRARY_DIR}"
               INSTALL_RPATH_USE_LINK_PATH TRUE
)

install(
    TARGETS ${COLA_MODULE_NAME}
    EXPORT COLA_JuliaTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/${COLA_MODULE_NAME}
)
install(
    EXPORT COLA_JuliaTargets
    NAMESPACE COLA_Julia::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/COLA_Julia
)
configure_package_config_file(
    cmake/COLA_JuliaConfig.cmake.in "${CMAKE_CURRENT_BINARY_DIR}/COLA_JuliaConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/COLA_Julia
)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/COLA_JuliaConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/COLA_JuliaConfig.cmake"
              "${CMAKE_CURRENT_BINARY_DIR}/COLA_JuliaConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/COLA_Julia
)
install(
    DIRECTORY COLA_Julia
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING
    PATTERN "*.hh"
)

if (BUILD_TESTING)
    add_subdirectory(tests)
endif ()

setup_cola_quality_tools(EXCLUDE_PATTERNS ".*/generated/.*" ".*/build-[^/]+/.*")
