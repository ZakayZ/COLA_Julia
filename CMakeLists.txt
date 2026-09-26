cmake_minimum_required(VERSION 3.20)

project(
    COLA_Julia
    VERSION 0.2.0
    LANGUAGES CXX
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(CTest)
include(GNUInstallDirs)

find_package(COLA REQUIRED)
find_program(Julia_EXECUTABLE NAMES julia REQUIRED)

execute_process(
    COMMAND "${Julia_EXECUTABLE}" --startup-file=no -e
            "using CxxWrap, WrapIt; print(CxxWrap.prefix_path())"
    OUTPUT_VARIABLE COLA_JULIA_CXXWRAP_PREFIX
    OUTPUT_STRIP_TRAILING_WHITESPACE COMMAND_ERROR_IS_FATAL ANY
)
list(PREPEND CMAKE_PREFIX_PATH "${COLA_JULIA_CXXWRAP_PREFIX}")
set(JlCxx_DIR
    "${COLA_JULIA_CXXWRAP_PREFIX}/lib/cmake/JlCxx"
    CACHE PATH "CxxWrap toolchain" FORCE
)
find_package(JlCxx REQUIRED)

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
               BUILD_RPATH
               "${Julia_LIBRARY_DIR};${COLA_JULIA_CXXWRAP_PREFIX}/${CMAKE_INSTALL_LIBDIR}"
               BUILD_WITH_INSTALL_RPATH TRUE
               INSTALL_RPATH "${Julia_LIBRARY_DIR}"
               INSTALL_RPATH_USE_LINK_PATH TRUE
)

install(TARGETS ${COLA_MODULE_NAME} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/${COLA_MODULE_NAME})
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
