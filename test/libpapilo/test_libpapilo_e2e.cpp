#include "catch_amalgamated.hpp"
#include "libpapilo.h"
#include <vector>
#include <cmath>

TEST_CASE("libpapilo end-to-end presolve test", "[e2e]") {
    
    SECTION("simple LP presolve - dual fix") {
        // Create a problem similar to DualFixTest's setupMatrixForDualFixFirstColumnOnlyPositiveValues
        // minimize: -x1 - x2
        // subject to:
        //   2*x1 + x2 >= 1
        //   x1 + 2*x2 >= 1
        //   0 <= x1 <= 1
        //   0 <= x2 <= 1
        //
        // Expected: x1 should be fixed to 1 by dual fixing
        
        papilo_t* papilo = papilo_create();
        REQUIRE(papilo != nullptr);
        
        // Set dimensions: 2 variables, 2 constraints, 4 non-zeros
        int result = papilo_set_problem_dimensions(papilo, 2, 2, 4);
        REQUIRE(result == PAPILO_OK);
        
        // Set objective: minimize -x1 - x2
        double obj_coeffs[] = {-1.0, -1.0};
        result = papilo_set_objective(papilo, obj_coeffs, 0.0);
        REQUIRE(result == PAPILO_OK);
        
        // Set variable bounds
        double lb[] = {0.0, 0.0};
        double ub[] = {1.0, 1.0};
        result = papilo_set_col_bounds_all(papilo, lb, ub);
        REQUIRE(result == PAPILO_OK);
        
        // Set constraint bounds (both >= constraints)
        double lhs[] = {1.0, 1.0};
        double rhs[] = {INFINITY, INFINITY};
        result = papilo_set_row_bounds_all(papilo, lhs, rhs);
        REQUIRE(result == PAPILO_OK);
        
        // Add matrix entries
        int rows[] = {0, 0, 1, 1};
        int cols[] = {0, 1, 0, 1};
        double values[] = {2.0, 1.0, 1.0, 2.0};
        result = papilo_add_entries(papilo, 4, rows, cols, values);
        REQUIRE(result == PAPILO_OK);
        
        // Build the problem
        result = papilo_build_problem(papilo);
        REQUIRE(result == PAPILO_OK);
        
        // Run presolve
        papilo_result_t* presolved = papilo_presolve(papilo);
        REQUIRE(presolved != nullptr);
        
        // Check status
        papilo_status_t status = papilo_result_get_status(presolved);
        REQUIRE(status == PAPILO_STATUS_REDUCED);
        
        // Check dimensions - should have fewer variables after presolve
        int new_ncols = papilo_result_get_ncols(presolved);
        int new_nrows = papilo_result_get_nrows(presolved);
        REQUIRE(new_ncols < 2); // At least one variable should be fixed
        
        // Check statistics
        int deleted_cols = 0;
        int deleted_rows = 0;
        result = papilo_result_get_num_deletions(presolved, &deleted_cols, &deleted_rows);
        REQUIRE(result == PAPILO_OK);
        REQUIRE(deleted_cols > 0);
        
        double presolve_time = papilo_result_get_presolve_time(presolved);
        REQUIRE(presolve_time >= 0.0);
        
        papilo_result_free(presolved);
        papilo_free(papilo);
    }
    
    SECTION("infeasible problem detection") {
        // Create an infeasible problem
        // minimize: x
        // subject to:
        //   x >= 1
        //   x <= 0
        
        papilo_t* papilo = papilo_create();
        REQUIRE(papilo != nullptr);
        
        int result = papilo_set_problem_dimensions(papilo, 1, 2, 2);
        REQUIRE(result == PAPILO_OK);
        
        double obj[] = {1.0};
        result = papilo_set_objective(papilo, obj, 0.0);
        REQUIRE(result == PAPILO_OK);
        
        double lb[] = {-INFINITY};
        double ub[] = {INFINITY};
        result = papilo_set_col_bounds_all(papilo, lb, ub);
        REQUIRE(result == PAPILO_OK);
        
        // x >= 1 and x <= 0
        double lhs[] = {1.0, -INFINITY};
        double rhs[] = {INFINITY, 0.0};
        result = papilo_set_row_bounds_all(papilo, lhs, rhs);
        REQUIRE(result == PAPILO_OK);
        
        result = papilo_add_entry(papilo, 0, 0, 1.0); // x >= 1
        REQUIRE(result == PAPILO_OK);
        result = papilo_add_entry(papilo, 1, 0, 1.0); // x <= 0
        REQUIRE(result == PAPILO_OK);
        
        result = papilo_build_problem(papilo);
        REQUIRE(result == PAPILO_OK);
        
        papilo_result_t* presolved = papilo_presolve(papilo);
        REQUIRE(presolved != nullptr);
        
        papilo_status_t status = papilo_result_get_status(presolved);
        REQUIRE(status == PAPILO_STATUS_INFEASIBLE);
        
        papilo_result_free(presolved);
        papilo_free(papilo);
    }
    
    SECTION("singleton column presolve") {
        // Create a problem with a singleton column
        // minimize: x + y + z
        // subject to:
        //   x + y >= 1
        //   z = 2 (singleton column)
        //   x, y, z >= 0
        
        papilo_t* papilo = papilo_create();
        REQUIRE(papilo != nullptr);
        
        int result = papilo_set_problem_dimensions(papilo, 3, 2, 3);
        REQUIRE(result == PAPILO_OK);
        
        double obj[] = {1.0, 1.0, 1.0};
        result = papilo_set_objective(papilo, obj, 0.0);
        REQUIRE(result == PAPILO_OK);
        
        double lb[] = {0.0, 0.0, 0.0};
        double ub[] = {INFINITY, INFINITY, INFINITY};
        result = papilo_set_col_bounds_all(papilo, lb, ub);
        REQUIRE(result == PAPILO_OK);
        
        // First constraint: x + y >= 1
        // Second constraint: z = 2
        double lhs[] = {1.0, 2.0};
        double rhs[] = {INFINITY, 2.0};
        result = papilo_set_row_bounds_all(papilo, lhs, rhs);
        REQUIRE(result == PAPILO_OK);
        
        // Matrix entries
        result = papilo_add_entry(papilo, 0, 0, 1.0); // x in first constraint
        REQUIRE(result == PAPILO_OK);
        result = papilo_add_entry(papilo, 0, 1, 1.0); // y in first constraint
        REQUIRE(result == PAPILO_OK);
        result = papilo_add_entry(papilo, 1, 2, 1.0); // z in second constraint (singleton)
        REQUIRE(result == PAPILO_OK);
        
        result = papilo_build_problem(papilo);
        REQUIRE(result == PAPILO_OK);
        
        papilo_result_t* presolved = papilo_presolve(papilo);
        REQUIRE(presolved != nullptr);
        
        papilo_status_t status = papilo_result_get_status(presolved);
        REQUIRE(status == PAPILO_STATUS_REDUCED);
        
        // Should have reduced the problem
        int new_ncols = papilo_result_get_ncols(presolved);
        int new_nrows = papilo_result_get_nrows(presolved);
        REQUIRE(new_ncols < 3); // z should be eliminated
        REQUIRE(new_nrows < 2); // singleton row should be eliminated
        
        papilo_result_free(presolved);
        papilo_free(papilo);
    }
}