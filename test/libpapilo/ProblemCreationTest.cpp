#include "catch_amalgamated.hpp"
#include "libpapilo.h"
#include <vector>
#include <cmath>

TEST_CASE("problem-creation", "[libpapilo]") {
    
    SECTION("basic problem construction") {
        // Create a simple LP problem:
        // minimize: 2*x + 3*y
        // subject to: x + y >= 1
        //            2*x + y <= 3
        //            x >= 0, y >= 0
        
        papilo_t* papilo = papilo_create();
        REQUIRE(papilo != nullptr);
        
        // Set dimensions: 2 variables, 2 constraints, 4 non-zeros
        int result = papilo_set_problem_dimensions(papilo, 2, 2, 4);
        REQUIRE(result == PAPILO_OK);
        
        // Set objective: minimize 2*x + 3*y
        double obj_coeffs[] = {2.0, 3.0};
        result = papilo_set_objective(papilo, obj_coeffs, 0.0);
        REQUIRE(result == PAPILO_OK);
        
        // Set variable bounds: x >= 0, y >= 0
        double lb[] = {0.0, 0.0};
        double ub[] = {INFINITY, INFINITY};
        result = papilo_set_col_bounds_all(papilo, lb, ub);
        REQUIRE(result == PAPILO_OK);
        
        // Set constraint bounds
        // x + y >= 1  => lhs = 1, rhs = inf
        // 2*x + y <= 3 => lhs = -inf, rhs = 3
        double lhs[] = {1.0, -INFINITY};
        double rhs[] = {INFINITY, 3.0};
        result = papilo_set_row_bounds_all(papilo, lhs, rhs);
        REQUIRE(result == PAPILO_OK);
        
        // Add matrix entries
        // Row 0: x + y (entries: (0,0,1), (0,1,1))
        // Row 1: 2*x + y (entries: (1,0,2), (1,1,1))
        int rows[] = {0, 0, 1, 1};
        int cols[] = {0, 1, 0, 1};
        double values[] = {1.0, 1.0, 2.0, 1.0};
        result = papilo_add_entries(papilo, 4, rows, cols, values);
        REQUIRE(result == PAPILO_OK);
        
        // Build the problem
        result = papilo_build_problem(papilo);
        REQUIRE(result == PAPILO_OK);
        
        papilo_free(papilo);
    }
    
    SECTION("error handling - invalid parameters") {
        papilo_t* papilo = papilo_create();
        REQUIRE(papilo != nullptr);
        
        // Test negative dimensions
        int result = papilo_set_problem_dimensions(papilo, -1, 2, 4);
        REQUIRE(result == PAPILO_ERROR_INVALID_PARAMETER);
        
        // Test null pointers
        result = papilo_set_objective(nullptr, nullptr, 0.0);
        REQUIRE(result == PAPILO_ERROR_INVALID_PARAMETER);
        
        // Test building without setting dimensions
        result = papilo_build_problem(papilo);
        REQUIRE(result == PAPILO_ERROR_INVALID_PARAMETER);
        
        papilo_free(papilo);
    }
    
    SECTION("error handling - invalid state") {
        papilo_t* papilo = papilo_create();
        REQUIRE(papilo != nullptr);
        
        // Set dimensions and build
        int result = papilo_set_problem_dimensions(papilo, 2, 2, 4);
        REQUIRE(result == PAPILO_OK);
        
        double obj[] = {1.0, 1.0};
        result = papilo_set_objective(papilo, obj, 0.0);
        REQUIRE(result == PAPILO_OK);
        
        result = papilo_build_problem(papilo);
        REQUIRE(result == PAPILO_OK);
        
        // Try to modify after building - should fail
        result = papilo_set_objective(papilo, obj, 0.0);
        REQUIRE(result == PAPILO_ERROR_INVALID_STATE);
        
        result = papilo_add_entry(papilo, 0, 0, 1.0);
        REQUIRE(result == PAPILO_ERROR_INVALID_STATE);
        
        papilo_free(papilo);
    }
    
    SECTION("individual bound setting") {
        papilo_t* papilo = papilo_create();
        REQUIRE(papilo != nullptr);
        
        int result = papilo_set_problem_dimensions(papilo, 2, 3, 6);
        REQUIRE(result == PAPILO_OK);
        
        // Set individual column bounds
        result = papilo_set_col_bounds(papilo, 0, -1.0, 1.0);
        REQUIRE(result == PAPILO_OK);
        
        result = papilo_set_col_bounds(papilo, 1, 0.0, INFINITY);
        REQUIRE(result == PAPILO_OK);
        
        result = papilo_set_col_bounds(papilo, 2, -INFINITY, 0.0);
        REQUIRE(result == PAPILO_OK);
        
        // Set individual row bounds
        result = papilo_set_row_bounds(papilo, 0, 5.0, 5.0); // equality
        REQUIRE(result == PAPILO_OK);
        
        result = papilo_set_row_bounds(papilo, 1, -INFINITY, 10.0);
        REQUIRE(result == PAPILO_OK);
        
        // Test out of bounds indices
        result = papilo_set_col_bounds(papilo, 3, 0.0, 1.0);
        REQUIRE(result == PAPILO_ERROR_INVALID_PARAMETER);
        
        result = papilo_set_row_bounds(papilo, 2, 0.0, 1.0);
        REQUIRE(result == PAPILO_ERROR_INVALID_PARAMETER);
        
        papilo_free(papilo);
    }
}
