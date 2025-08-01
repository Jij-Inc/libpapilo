# Tests for libpapilo C API

# Include Catch2 headers
include_directories(${CMAKE_SOURCE_DIR}/src/papilo/external/catch)

# Test executable for problem construction
add_executable(test_problem_construction TestMain.cpp test_problem_construction.cpp)
target_link_libraries(test_problem_construction libpapilo papilo)

# Register tests with CTest
add_test(NAME test_problem_construction COMMAND test_problem_construction)

# Test executable for end-to-end test
add_executable(test_libpapilo_e2e TestMain.cpp test_libpapilo_e2e.cpp)
target_link_libraries(test_libpapilo_e2e libpapilo papilo)

# Register tests with CTest
add_test(NAME test_libpapilo_e2e COMMAND test_libpapilo_e2e)