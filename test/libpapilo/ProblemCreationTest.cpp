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
    
    SECTION("data retrieval after construction") {
        // Create and build a problem, then verify data can be retrieved correctly
        papilo_t* papilo = papilo_create();
        REQUIRE(papilo != nullptr);
        
        // Set dimensions: 2 variables, 2 constraints, 4 non-zeros
        int result = papilo_set_problem_dimensions(papilo, 2, 2, 4);
        REQUIRE(result == PAPILO_OK);
        
        // Set objective: minimize 2*x + 3*y + 1.5
        double obj_coeffs[] = {2.0, 3.0};
        result = papilo_set_objective(papilo, obj_coeffs, 1.5);
        REQUIRE(result == PAPILO_OK);
        
        // Set variable bounds
        double lb[] = {-1.0, 0.0};
        double ub[] = {5.0, INFINITY};
        result = papilo_set_col_bounds_all(papilo, lb, ub);
        REQUIRE(result == PAPILO_OK);
        
        // Set constraint bounds
        double lhs[] = {1.0, -INFINITY};
        double rhs[] = {INFINITY, 3.0};
        result = papilo_set_row_bounds_all(papilo, lhs, rhs);
        REQUIRE(result == PAPILO_OK);
        
        // Add matrix entries
        int rows[] = {0, 0, 1, 1};
        int cols[] = {0, 1, 0, 1};
        double values[] = {1.0, 1.0, 2.0, 1.0};
        result = papilo_add_entries(papilo, 4, rows, cols, values);
        REQUIRE(result == PAPILO_OK);
        
        // Build the problem
        result = papilo_build_problem(papilo);
        REQUIRE(result == PAPILO_OK);
        
        // Test dimension retrieval
        REQUIRE(papilo_get_nrows(papilo) == 2);
        REQUIRE(papilo_get_ncols(papilo) == 2);
        REQUIRE(papilo_get_nnz(papilo) == 4);
        
        // Test objective retrieval
        double retrieved_coeffs[2];
        double retrieved_offset;
        result = papilo_get_objective(papilo, retrieved_coeffs, &retrieved_offset);
        REQUIRE(result == PAPILO_OK);
        REQUIRE(retrieved_coeffs[0] == Approx(2.0));
        REQUIRE(retrieved_coeffs[1] == Approx(3.0));
        REQUIRE(retrieved_offset == Approx(1.5));
        
        // Test variable bounds retrieval (all bounds)
        double retrieved_lb[2], retrieved_ub[2];
        result = papilo_get_col_bounds_all(papilo, retrieved_lb, retrieved_ub);
        REQUIRE(result == PAPILO_OK);
        REQUIRE(retrieved_lb[0] == Approx(-1.0));
        REQUIRE(retrieved_lb[1] == Approx(0.0));
        REQUIRE(retrieved_ub[0] == Approx(5.0));
        REQUIRE(std::isinf(retrieved_ub[1]));
        
        // Test variable bounds retrieval (individual)
        double single_lb, single_ub;
        result = papilo_get_col_bounds(papilo, 0, &single_lb, &single_ub);
        REQUIRE(result == PAPILO_OK);
        REQUIRE(single_lb == Approx(-1.0));
        REQUIRE(single_ub == Approx(5.0));
        
        // Test constraint bounds retrieval (all bounds)  
        double retrieved_lhs[2], retrieved_rhs[2];
        result = papilo_get_row_bounds_all(papilo, retrieved_lhs, retrieved_rhs);
        REQUIRE(result == PAPILO_OK);
        REQUIRE(retrieved_lhs[0] == Approx(1.0));
        REQUIRE(std::isinf(retrieved_lhs[1]) && retrieved_lhs[1] < 0);
        REQUIRE(std::isinf(retrieved_rhs[0]));
        REQUIRE(retrieved_rhs[1] == Approx(3.0));
        
        // Test constraint bounds retrieval (individual)
        double single_lhs, single_rhs;
        result = papilo_get_row_bounds(papilo, 1, &single_lhs, &single_rhs);
        REQUIRE(result == PAPILO_OK);
        REQUIRE(std::isinf(single_lhs) && single_lhs < 0);
        REQUIRE(single_rhs == Approx(3.0));
        
        // Test matrix retrieval
        int retrieved_rows[4], retrieved_cols[4];
        double retrieved_values[4];
        result = papilo_get_matrix(papilo, retrieved_rows, retrieved_cols, retrieved_values);
        REQUIRE(result == PAPILO_OK);
        
        // Verify matrix entries (note: order may vary due to column-wise storage)
        std::vector<std::tuple<int, int, double>> expected = {
            {0, 0, 1.0}, {1, 0, 2.0}, {0, 1, 1.0}, {1, 1, 1.0}
        };
        std::vector<std::tuple<int, int, double>> retrieved;
        for (int i = 0; i < 4; ++i) {
            retrieved.emplace_back(retrieved_rows[i], retrieved_cols[i], retrieved_values[i]);
        }
        std::sort(expected.begin(), expected.end());
        std::sort(retrieved.begin(), retrieved.end());
        
        for (int i = 0; i < 4; ++i) {
            REQUIRE(std::get<0>(retrieved[i]) == std::get<0>(expected[i]));
            REQUIRE(std::get<1>(retrieved[i]) == std::get<1>(expected[i]));
            REQUIRE(std::get<2>(retrieved[i]) == Approx(std::get<2>(expected[i])));
        }
        
        papilo_free(papilo);
    }
}
