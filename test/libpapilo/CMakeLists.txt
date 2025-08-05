# Tests for libpapilo C API

set(LIBPAPILO_TESTS
    ProblemBuilderTest.cpp
    PresolveCommonTest.cpp
    SingletonColsTest.cpp
    # Add more test files as needed
)

# Convert file name to kebab-case for test names
# e.g. ProblemCreationTest.cpp -> problem-creation
function(convert_to_test_name PASCAL_STRING OUTPUT_VAR)
    get_filename_component(BASENAME ${PASCAL_STRING} NAME_WE)
    string(REPLACE "Test" "" NO_SUFFIX ${BASENAME})
    string(REGEX REPLACE "([a-z])([A-Z])" "\\1-\\2" KEBAB_INTERMEDIATE ${NO_SUFFIX})
    string(TOLOWER ${KEBAB_INTERMEDIATE} KEBAB_RESULT)
    set(${OUTPUT_VAR} ${KEBAB_RESULT} PARENT_SCOPE)
endfunction()

# Single test executable for all libpapilo tests
include_directories(${CMAKE_SOURCE_DIR}/src/papilo/external/catch)
add_executable(libpapilo_unit_test ${LIBPAPILO_TESTS} ${CMAKE_SOURCE_DIR}/src/papilo/external/catch/catch_amalgamated.cpp)
target_link_libraries(libpapilo_unit_test libpapilo)

# Register test targets for each test file
foreach(TEST_FILE ${LIBPAPILO_TESTS})
    convert_to_test_name(${TEST_FILE} KEBAB_TEST_NAME)
    add_test(NAME libpapilo-${KEBAB_TEST_NAME} COMMAND libpapilo_unit_test "${KEBAB_TEST_NAME}")
    set_tests_properties(libpapilo-${KEBAB_TEST_NAME} PROPERTIES LABELS "libpapilo")
endforeach()    
