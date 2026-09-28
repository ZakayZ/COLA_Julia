set(TEST_JULIA_PROJECT "${CMAKE_BINARY_DIR}/calculation")
add_test(
    NAME cola-julia-prepare
    COMMAND
        "${Julia_EXECUTABLE}" --startup-file=no "${CMAKE_CURRENT_SOURCE_DIR}/prepare.jl"
        "${TEST_JULIA_PROJECT}" "${PROJECT_SOURCE_DIR}"
        "${PROJECT_SOURCE_DIR}/examples/julia-filters"
)
set_tests_properties(cola-julia-prepare PROPERTIES FIXTURES_SETUP JuliaEnvironment)

add_test(NAME cola-julia-native-tests
         COMMAND "${Julia_EXECUTABLE}" --startup-file=no --project=${TEST_JULIA_PROJECT} -e
                 "using Pkg; Pkg.test([\"COLA\", \"JuliaTestFilters\"])"
)
set_tests_properties(cola-julia-native-tests PROPERTIES FIXTURES_REQUIRED JuliaEnvironment)

add_executable(cola-julia-filters-test julia_filters_test.cc)
target_compile_features(cola-julia-filters-test PRIVATE cxx_std_20)
# Intentionally link only COLA: exercise the real dlopen module loader.
target_link_libraries(cola-julia-filters-test PRIVATE COLA)
add_dependencies(cola-julia-filters-test ${COLA_MODULE_NAME})
add_test(NAME cola-julia-all-filters COMMAND cola-julia-filters-test
                                             "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}"
)
set_tests_properties(
    cola-julia-all-filters PROPERTIES FIXTURES_REQUIRED JuliaEnvironment ENVIRONMENT
                                      "JULIA_PROJECT=${TEST_JULIA_PROJECT}"
)
