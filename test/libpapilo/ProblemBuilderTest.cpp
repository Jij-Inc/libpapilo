#include "catch_amalgamated.hpp"
#include "libpapilo.h"
#include <cmath>
#include <vector>

TEST_CASE( "problem-builder", "[libpapilo]" )
{
   SECTION( "basic builder workflow" )
   {
      // Create builder
      libpapilo_problem_builder_t* builder = libpapilo_problem_builder_create();
      REQUIRE( builder != nullptr );

      // Set dimensions
      libpapilo_problem_builder_set_num_cols( builder, 3 );
      libpapilo_problem_builder_set_num_rows( builder, 2 );

      // Verify dimensions
      REQUIRE( libpapilo_problem_builder_get_num_cols( builder ) == 3 );
      REQUIRE( libpapilo_problem_builder_get_num_rows( builder ) == 2 );

      // Set objective: minimize 2*x + 3*y + 4*z + 5
      libpapilo_problem_builder_set_obj( builder, 0, 2.0 );
      libpapilo_problem_builder_set_obj( builder, 1, 3.0 );
      libpapilo_problem_builder_set_obj( builder, 2, 4.0 );
      libpapilo_problem_builder_set_obj_offset( builder, 5.0 );

      // Set variable bounds
      // x in [0, 10], y in [-inf, 5], z in [1, inf]
      libpapilo_problem_builder_set_col_lb( builder, 0, 0.0 );
      libpapilo_problem_builder_set_col_ub( builder, 0, 10.0 );

      libpapilo_problem_builder_set_col_lb( builder, 1, -INFINITY );
      libpapilo_problem_builder_set_col_ub( builder, 1, 5.0 );

      libpapilo_problem_builder_set_col_lb( builder, 2, 1.0 );
      libpapilo_problem_builder_set_col_ub( builder, 2, INFINITY );

      // Set row bounds
      // row 0: x + y + z >= 2  (lhs = 2, rhs = inf)
      // row 1: 2*x + y <= 10   (lhs = -inf, rhs = 10)
      libpapilo_problem_builder_set_row_lhs( builder, 0, 2.0 );
      libpapilo_problem_builder_set_row_rhs( builder, 0, INFINITY );

      libpapilo_problem_builder_set_row_lhs( builder, 1, -INFINITY );
      libpapilo_problem_builder_set_row_rhs( builder, 1, 10.0 );

      // Add matrix entries
      // Row 0: x + y + z
      libpapilo_problem_builder_add_entry( builder, 0, 0, 1.0 );
      libpapilo_problem_builder_add_entry( builder, 0, 1, 1.0 );
      libpapilo_problem_builder_add_entry( builder, 0, 2, 1.0 );

      // Row 1: 2*x + y
      libpapilo_problem_builder_add_entry( builder, 1, 0, 2.0 );
      libpapilo_problem_builder_add_entry( builder, 1, 1, 1.0 );

      // Set names
      libpapilo_problem_builder_set_problem_name( builder, "test_problem" );
      libpapilo_problem_builder_set_col_name( builder, 0, "x" );
      libpapilo_problem_builder_set_col_name( builder, 1, "y" );
      libpapilo_problem_builder_set_col_name( builder, 2, "z" );
      libpapilo_problem_builder_set_row_name( builder, 0, "constraint1" );
      libpapilo_problem_builder_set_row_name( builder, 1, "constraint2" );

      // Build the problem
      libpapilo_problem_t* problem = libpapilo_problem_builder_build( builder );
      REQUIRE( problem != nullptr );

      // Verify problem dimensions
      REQUIRE( libpapilo_problem_get_nrows( problem ) == 2 );
      REQUIRE( libpapilo_problem_get_ncols( problem ) == 3 );
      REQUIRE( libpapilo_problem_get_nnz( problem ) == 5 );

      // Clean up
      libpapilo_problem_free( problem );
      libpapilo_problem_builder_free( builder );
   }

   SECTION( "batch operations" )
   {
      libpapilo_problem_builder_t* builder = libpapilo_problem_builder_create();
      REQUIRE( builder != nullptr );

      // Reserve space
      libpapilo_problem_builder_reserve( builder, 6, 2, 3 );

      // Set dimensions
      libpapilo_problem_builder_set_num_cols( builder, 3 );
      libpapilo_problem_builder_set_num_rows( builder, 2 );

      // Set objective all at once
      double obj_coeffs[] = { 1.0, 2.0, 3.0 };
      libpapilo_problem_builder_set_obj_all( builder, obj_coeffs );

      // Set bounds all at once
      double col_lb[] = { 0.0, -INFINITY, 1.0 };
      double col_ub[] = { 10.0, 5.0, INFINITY };
      libpapilo_problem_builder_set_col_lb_all( builder, col_lb );
      libpapilo_problem_builder_set_col_ub_all( builder, col_ub );

      double row_lhs[] = { 2.0, -INFINITY };
      double row_rhs[] = { INFINITY, 10.0 };
      libpapilo_problem_builder_set_row_lhs_all( builder, row_lhs );
      libpapilo_problem_builder_set_row_rhs_all( builder, row_rhs );

      // Add entries all at once
      int rows[] = { 0, 0, 0, 1, 1 };
      int cols[] = { 0, 1, 2, 0, 1 };
      double vals[] = { 1.0, 1.0, 1.0, 2.0, 1.0 };
      libpapilo_problem_builder_add_entry_all( builder, 5, rows, cols, vals );

      // Build and verify
      libpapilo_problem_t* problem = libpapilo_problem_builder_build( builder );
      REQUIRE( problem != nullptr );
      REQUIRE( libpapilo_problem_get_nnz( problem ) == 5 );

      libpapilo_problem_free( problem );
      libpapilo_problem_builder_free( builder );
   }

   SECTION( "integer variables" )
   {
      libpapilo_problem_builder_t* builder = libpapilo_problem_builder_create();
      REQUIRE( builder != nullptr );

      libpapilo_problem_builder_set_num_cols( builder, 3 );
      libpapilo_problem_builder_set_num_rows( builder, 1 );

      // Set some variables as integer
      libpapilo_problem_builder_set_col_integral( builder, 0,
                                                  1 ); // x is integer
      libpapilo_problem_builder_set_col_integral( builder, 1,
                                                  0 ); // y is continuous
      libpapilo_problem_builder_set_col_integral( builder, 2,
                                                  1 ); // z is integer

      // Alternative: set all at once
      uint8_t integrality[] = { 1, 0, 1 };
      libpapilo_problem_builder_set_col_integral_all( builder, integrality );

      libpapilo_problem_t* problem = libpapilo_problem_builder_build( builder );
      REQUIRE( problem != nullptr );

      libpapilo_problem_free( problem );
      libpapilo_problem_builder_free( builder );
   }

   SECTION( "row and column entry methods" )
   {
      libpapilo_problem_builder_t* builder = libpapilo_problem_builder_create();
      REQUIRE( builder != nullptr );

      libpapilo_problem_builder_set_num_cols( builder, 4 );
      libpapilo_problem_builder_set_num_rows( builder, 3 );

      // Add entries for row 0: x + 2*y + 3*z
      int row0_cols[] = { 0, 1, 2 };
      double row0_vals[] = { 1.0, 2.0, 3.0 };
      libpapilo_problem_builder_add_row_entries( builder, 0, 3, row0_cols,
                                                 row0_vals );

      // Add entries for column 3: appears in rows 1 and 2
      int col3_rows[] = { 1, 2 };
      double col3_vals[] = { 4.0, 5.0 };
      libpapilo_problem_builder_add_col_entries( builder, 3, 2, col3_rows,
                                                 col3_vals );

      libpapilo_problem_t* problem = libpapilo_problem_builder_build( builder );
      REQUIRE( problem != nullptr );
      REQUIRE( libpapilo_problem_get_nnz( problem ) == 5 );

      libpapilo_problem_free( problem );
      libpapilo_problem_builder_free( builder );
   }
}
