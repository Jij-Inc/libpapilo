# Tests for libpapilo C API

set(LIBPAPILO_TESTS
    ProblemBuilderTest.cpp
    SingletonColsTest.cpp
    SimpleSubstitutionTest.cpp
    ProblemUpdateTest.cpp
    PresolveTest.cpp
    PostsolveTest.cpp
    # Add more test files as needed
)

# Single test executable for all libpapilo tests
include_directories(${CMAKE_SOURCE_DIR}/src/papilo/external/catch)
add_executable(libpapilo_unit_test ${LIBPAPILO_TESTS} ${CMAKE_SOURCE_DIR}/src/papilo/external/catch/catch_amalgamated.cpp)
target_link_libraries(libpapilo_unit_test libpapilo)
target_compile_definitions(libpapilo_unit_test PRIVATE LIBPAPILO_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

# -----------------------------------------------------------------------------
# Fixture regeneration (only if CLI executable is available)
# -----------------------------------------------------------------------------
if(TARGET papilo-executable)
    set(DUAL_FIX_MPS_NEG  ${CMAKE_SOURCE_DIR}/test/instances/dual_fix_neg_inf.mps)
    set(DUAL_FIX_MPS_POS  ${CMAKE_SOURCE_DIR}/test/instances/dual_fix_pos_inf.mps)
    set(DUAL_FIX_PS_NEG   ${CMAKE_SOURCE_DIR}/test/resources/dual_fix_neg_inf.postsolve)
    set(DUAL_FIX_PS_POS   ${CMAKE_SOURCE_DIR}/test/resources/dual_fix_pos_inf.postsolve)

    add_custom_command(
        OUTPUT ${DUAL_FIX_PS_NEG}
        COMMAND $<TARGET_FILE:papilo-executable> presolve -f ${DUAL_FIX_MPS_NEG} -v ${DUAL_FIX_PS_NEG}
        DEPENDS papilo-executable ${DUAL_FIX_MPS_NEG}
        COMMENT "Regenerating dual_fix_neg_inf.postsolve"
        VERBATIM
    )
    add_custom_command(
        OUTPUT ${DUAL_FIX_PS_POS}
        COMMAND $<TARGET_FILE:papilo-executable> presolve -f ${DUAL_FIX_MPS_POS} -v ${DUAL_FIX_PS_POS}
        DEPENDS papilo-executable ${DUAL_FIX_MPS_POS}
        COMMENT "Regenerating dual_fix_pos_inf.postsolve"
        VERBATIM
    )

    add_custom_target(update_dual_fix_postsolve
        DEPENDS ${DUAL_FIX_PS_NEG} ${DUAL_FIX_PS_POS}
        COMMENT "Update both dual_fix_* postsolve fixture archives")
endif()

set(LIBPAPILO_TEST_NAMES 
    "problem-builder"

    # SingletonColsTest.cpp
    "happy-path-singleton-column"
    "happy-path-singleton-column-equation"
    "happy-path-singleton-column-implied-bounds-negative-coeff-pos-bounds"
    "happy-path-singleton-column-implied-bounds-negative-coeff-neg-bounds"
    "happy-path-singleton-column-implied-bounds-positive-coeff-pos-bounds"
    "happy-path-singleton-column-implied-bounds-positive-coeff-neg-bounds"
    "happy-path-singleton-column-infinity-bounds-equation"

    # SimpleSubstitutionTest.cpp
    "simple-substitution-happy-path-for-2-int"
    "simple-substitution-happy-path-for-int-continuous-coeff"
    "simple-substitution-happy-path-for-2-continuous"
    "simple-substitution-happy-path-for-continuous-and-integer"
    "simple-substitution-for-2-int"
    "simple-substitution-2-negative-integer"
    "simple-substitution-feasible-gcd"
    "simple-substitution-non-coprime"
    "simple-substitution-violated-gcd"
    "example_10_1_in_constraint_integer_programming"
    "simple-substitution-should_return_feasible_if_gcd_of_coeff_is_in_rhs"
    "simple-substitution-should_return_infeasible_if_gcd_of_coeff_is_in_rhs"
    
    # ProblemUpdateTest.cpp
    "trivial-presolve-singleton-row"
    "trivial-presolve-singleton-row-pt-2"
    
    # PresolveTest.cpp
    "replacing-variables-is-postponed-by-flag"
    "happy-path-replace-variable"
    "happy-path-substitute-matrix-coefficient-into-objective"
    "happy-path-aggregate-free-column"
    
    # PostsolveTest.cpp
    "finding-the-right-value-in-postsolve-for-a-column-fixed-neg-inf"
    "finding-the-right-value-in-postsolve-for-a-column-fixed-pos-inf"
)

# Register test targets for each test file
foreach(TEST_NAME ${LIBPAPILO_TEST_NAMES})
    add_test(NAME libpapilo-${TEST_NAME} COMMAND libpapilo_unit_test "${TEST_NAME}")
    set_tests_properties(libpapilo-${TEST_NAME} PROPERTIES LABELS "libpapilo")
endforeach()    
