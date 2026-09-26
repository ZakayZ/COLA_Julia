# Generate against the same COLA headers used to compile the adapter.
get_target_property(COLA_INCLUDE_DIRS COLA INTERFACE_INCLUDE_DIRECTORIES)
find_path(
    COLA_EVENT_INCLUDE_DIR EventData.hh
    HINTS ${COLA_INCLUDE_DIRS}
    PATH_SUFFIXES COLA REQUIRED
)
file(GLOB COLA_BINDING_HEADERS CONFIGURE_DEPENDS "${COLA_EVENT_INCLUDE_DIR}/*.hh")

set(BINDING_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
configure_file(wrapit.toml.in wrapit.toml @ONLY)
add_custom_command(
    OUTPUT "${BINDING_OUTPUT_DIR}/jlCOLANative.cxx" "${BINDING_OUTPUT_DIR}/jlCOLANative.h"
           "${BINDING_OUTPUT_DIR}/Wrapper.h" "${BINDING_OUTPUT_DIR}/dbg_msg.h"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${BINDING_OUTPUT_DIR}"
    COMMAND
        "${Julia_EXECUTABLE}" --startup-file=no -e
        "using WrapIt; exit(wrapit(ARGS[1], force=true, returncode=true))"
        "${CMAKE_CURRENT_BINARY_DIR}/wrapit.toml"
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/wrapit.toml" "${CMAKE_CURRENT_SOURCE_DIR}/veto.txt"
            ${COLA_BINDING_HEADERS}
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMENT "Generating COLA CxxWrap bindings"
    VERBATIM
)

# Include the generated definitions in the manual-accessor translation unit so
# CxxWrap type traits have one consistent definition.
add_library(COLA_JuliaBindings SHARED "${BINDING_OUTPUT_DIR}/jlCOLANative.cxx" JuliaBindings.cc)
set_source_files_properties(
    "${BINDING_OUTPUT_DIR}/jlCOLANative.cxx" PROPERTIES HEADER_FILE_ONLY TRUE
)
target_include_directories(
    COLA_JuliaBindings PRIVATE "${BINDING_OUTPUT_DIR}" "${COLA_EVENT_INCLUDE_DIR}"
)
target_compile_features(COLA_JuliaBindings PRIVATE cxx_std_20)
target_link_libraries(COLA_JuliaBindings PRIVATE COLA JlCxx::cxxwrap_julia JlCxx::cxxwrap_julia_stl)
set_target_properties(
    COLA_JuliaBindings PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/julia/COLA/deps"
                                  INSTALL_RPATH_USE_LINK_PATH TRUE
)

set(COLA_API_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../julia/COLA")
file(
    GLOB_RECURSE COLA_API_FILES CONFIGURE_DEPENDS
    RELATIVE "${COLA_API_SOURCE_DIR}"
    "${COLA_API_SOURCE_DIR}/*.jl" "${COLA_API_SOURCE_DIR}/Project.toml"
)
foreach (file IN LISTS COLA_API_FILES)
    configure_file(
        "${COLA_API_SOURCE_DIR}/${file}" "${CMAKE_BINARY_DIR}/julia/COLA/${file}" COPYONLY
    )
endforeach ()
install(
    DIRECTORY "${COLA_API_SOURCE_DIR}"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/COLA_Julia/julia"
    PATTERN "Manifest.toml" EXCLUDE
    PATTERN "deps" EXCLUDE
)
install(TARGETS COLA_JuliaBindings
        LIBRARY DESTINATION "${CMAKE_INSTALL_DATADIR}/COLA_Julia/julia/COLA/deps"
)
