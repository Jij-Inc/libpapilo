# Tests for libpapilo C API

set(LIBPAPILO_TESTS
    ProblemBuilderTest.cpp
    PresolveCommonTest.cpp
    SingletonColsTest.cpp
    # Add more test files as needed
)

# Single test executable for all libpapilo tests
include_directories(${CMAKE_SOURCE_DIR}/src/papilo/external/catch)
add_executable(libpapilo_unit_test ${LIBPAPILO_TESTS} ${CMAKE_SOURCE_DIR}/src/papilo/external/catch/catch_amalgamated.cpp)
target_link_libraries(libpapilo_unit_test libpapilo)

set(LIBPAPILO_TEST_NAMES 
    "problem-builder"
    "presolve-common"

    # SingletonColsTest.cpp
    "happy-path-singleton-column"
    "happy-path-singleton-column-equation"
    "happy-path-singleton-column-implied-bounds-negative-coeff-pos-bounds"
    "happy-path-singleton-column-implied-bounds-negative-coeff-neg-bounds"
    "happy-path-singleton-column-implied-bounds-positive-coeff-pos-bounds"
    "happy-path-singleton-column-implied-bounds-positive-coeff-neg-bounds"
    "happy-path-singleton-column-infinity-bounds-equation"
)

# Register test targets for each test file
foreach(TEST_NAME ${LIBPAPILO_TEST_NAMES})
    add_test(NAME libpapilo-${TEST_NAME} COMMAND libpapilo_unit_test "${TEST_NAME}")
    set_tests_properties(libpapilo-${TEST_NAME} PROPERTIES LABELS "libpapilo")
endforeach()    
