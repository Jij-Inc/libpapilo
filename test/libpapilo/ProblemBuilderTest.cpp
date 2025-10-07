/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                           */
/* This file is part of the library libpapilo, a fork of PaPILO from ZIB     */
/*                                                                           */
/* Copyright (C) 2020-2025 Zuse Institute Berlin (ZIB)                       */
/* Copyright (C) 2025      Jij-Inc.                                          */
/*                                                                           */
/* This program is free software: you can redistribute it and/or modify      */
/* it under the terms of the GNU Lesser General Public License as published  */
/* by the Free Software Foundation, either version 3 of the License, or      */
/* (at your option) any later version.                                       */
/*                                                                           */
/* This program is distributed in the hope that it will be useful,           */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of            */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             */
/* GNU Lesser General Public License for more details.                       */
/*                                                                           */
/* You should have received a copy of the GNU Lesser General Public License  */
/* along with this program.  If not, see <https://www.gnu.org/licenses/>.    */
/*                                                                           */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "catch_amalgamated.hpp"
#include "libpapilo.h"
#include <cmath>
#include <vector>

TEST_CASE( "problem-builder", "[libpapilo]" )
{
   SECTION( "basic builder workflow" )
   {
      // Test Purpose: Verify individual setter functions work correctly and
      // validate that the constructed problem contains the expected data.
      // Uses individual setters (set_obj, set_col_lb, add_entry, etc.) to build
      // a 3-variable, 2-constraint optimization problem with names and bounds.
      // Comprehensive validation using all getter APIs to ensure data
      // integrity. Create builder
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

      // Test new getter APIs

      // Test objective coefficients and offset
      size_t obj_size;
      const double* obj_coeffs =
          libpapilo_problem_get_objective_coefficients( problem, &obj_size );
      REQUIRE( obj_size == 3 );
      REQUIRE( obj_coeffs[0] == 2.0 );
      REQUIRE( obj_coeffs[1] == 3.0 );
      REQUIRE( obj_coeffs[2] == 4.0 );
      REQUIRE( libpapilo_problem_get_objective_offset( problem ) == 5.0 );

      // Test variable bounds
      size_t lb_size, ub_size;
      const double* lower_bounds =
          libpapilo_problem_get_lower_bounds( problem, &lb_size );
      const double* upper_bounds =
          libpapilo_problem_get_upper_bounds( problem, &ub_size );
      REQUIRE( lb_size == 3 );
      REQUIRE( ub_size == 3 );

      REQUIRE( lower_bounds[0] == 0.0 );
      REQUIRE( ( std::isinf( lower_bounds[1] ) && lower_bounds[1] < 0 ) );
      REQUIRE( lower_bounds[2] == 1.0 );

      REQUIRE( upper_bounds[0] == 10.0 );
      REQUIRE( upper_bounds[1] == 5.0 );
      REQUIRE( ( std::isinf( upper_bounds[2] ) && upper_bounds[2] > 0 ) );

      // Test constraint bounds
      size_t lhs_size, rhs_size;
      const double* lhs = libpapilo_problem_get_row_lhs( problem, &lhs_size );
      const double* rhs = libpapilo_problem_get_row_rhs( problem, &rhs_size );
      REQUIRE( lhs_size == 2 );
      REQUIRE( rhs_size == 2 );

      REQUIRE( lhs[0] == 2.0 );
      REQUIRE( ( std::isinf( lhs[1] ) && lhs[1] < 0 ) );
      REQUIRE( ( std::isinf( rhs[0] ) && rhs[0] > 0 ) );
      REQUIRE( rhs[1] == 10.0 );

      // Test row and column sizes
      size_t row_sizes_len, col_sizes_len;
      const int* row_sizes =
          libpapilo_problem_get_row_sizes( problem, &row_sizes_len );
      const int* col_sizes =
          libpapilo_problem_get_col_sizes( problem, &col_sizes_len );
      REQUIRE( row_sizes_len == 2 );
      REQUIRE( col_sizes_len == 3 );
      REQUIRE( row_sizes[0] == 3 ); // row 0: x + y + z (3 entries)
      REQUIRE( row_sizes[1] == 2 ); // row 1: 2*x + y (2 entries)
      REQUIRE( col_sizes[0] == 2 ); // x appears in 2 rows
      REQUIRE( col_sizes[1] == 2 ); // y appears in 2 rows
      REQUIRE( col_sizes[2] == 1 ); // z appears in 1 row

      // Test sparse matrix entries
      const int* row0_cols;
      const double* row0_vals;
      int row0_len = libpapilo_problem_get_row_entries( problem, 0, &row0_cols,
                                                        &row0_vals );
      REQUIRE( row0_len == 3 );
      REQUIRE( row0_vals[0] == 1.0 ); // coefficient for first variable in row 0
      REQUIRE( row0_vals[1] ==
               1.0 ); // coefficient for second variable in row 0
      REQUIRE( row0_vals[2] == 1.0 ); // coefficient for third variable in row 0

      const int* row1_cols;
      const double* row1_vals;
      int row1_len = libpapilo_problem_get_row_entries( problem, 1, &row1_cols,
                                                        &row1_vals );
      REQUIRE( row1_len == 2 );
      REQUIRE( row1_vals[0] == 2.0 ); // coefficient for first variable in row 1
      REQUIRE( row1_vals[1] ==
               1.0 ); // coefficient for second variable in row 1

      // Test column entries
      const int* col0_rows;
      const double* col0_vals;
      int col0_len = libpapilo_problem_get_col_entries( problem, 0, &col0_rows,
                                                        &col0_vals );
      REQUIRE( col0_len == 2 );
      REQUIRE( col0_vals[0] == 1.0 ); // x coefficient in first constraint
      REQUIRE( col0_vals[1] == 2.0 ); // x coefficient in second constraint

      // Test names
      REQUIRE( std::string( libpapilo_problem_get_name( problem ) ) ==
               "test_problem" );
      REQUIRE( std::string(
                   libpapilo_problem_get_variable_name( problem, 0 ) ) == "x" );
      REQUIRE( std::string(
                   libpapilo_problem_get_variable_name( problem, 1 ) ) == "y" );
      REQUIRE( std::string(
                   libpapilo_problem_get_variable_name( problem, 2 ) ) == "z" );
      REQUIRE( std::string( libpapilo_problem_get_constraint_name(
                   problem, 0 ) ) == "constraint1" );
      REQUIRE( std::string( libpapilo_problem_get_constraint_name(
                   problem, 1 ) ) == "constraint2" );

      // Test flags (basic check - exact flag values depend on internal
      // processing) uint8_t col0_flags = libpapilo_problem_get_col_flags(
      // problem, 0 );  // Not used in current test
      uint8_t col1_flags = libpapilo_problem_get_col_flags( problem, 1 );
      uint8_t col2_flags = libpapilo_problem_get_col_flags( problem, 2 );

      // col1 should have LB_INF flag (lower bound is -infinity)
      REQUIRE( ( col1_flags & LIBPAPILO_COLFLAG_LB_INF ) != 0 );
      // col2 should have UB_INF flag (upper bound is +infinity)
      REQUIRE( ( col2_flags & LIBPAPILO_COLFLAG_UB_INF ) != 0 );

      uint8_t row0_flags = libpapilo_problem_get_row_flags( problem, 0 );
      uint8_t row1_flags = libpapilo_problem_get_row_flags( problem, 1 );

      // row0 should have RHS_INF flag (right hand side is +infinity)
      REQUIRE( ( row0_flags & LIBPAPILO_ROWFLAG_RHS_INF ) != 0 );
      // row1 should have LHS_INF flag (left hand side is -infinity)
      REQUIRE( ( row1_flags & LIBPAPILO_ROWFLAG_LHS_INF ) != 0 );

      // Test continuous/integral counts (all variables are continuous by
      // default)
      REQUIRE( libpapilo_problem_get_num_continuous_cols( problem ) == 3 );
      REQUIRE( libpapilo_problem_get_num_integral_cols( problem ) == 0 );

      // Clean up
      libpapilo_problem_free( problem );
      libpapilo_problem_builder_free( builder );
   }

   SECTION( "batch operations" )
   {
      // Test Purpose: Verify batch/array setter functions (*_all methods) work
      // correctly. Uses batch setters (set_obj_all, set_col_lb_all,
      // add_entry_all, etc.) to build the same logical problem structure as the
      // basic workflow, but with different construction methods. Validates that
      // batch operations produce equivalent results.
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

      // Test objective coefficients from batch operation
      size_t obj_size;
      const double* retrieved_obj_coeffs =
          libpapilo_problem_get_objective_coefficients( problem, &obj_size );
      REQUIRE( obj_size == 3 );
      REQUIRE( retrieved_obj_coeffs[0] == 1.0 );
      REQUIRE( retrieved_obj_coeffs[1] == 2.0 );
      REQUIRE( retrieved_obj_coeffs[2] == 3.0 );

      // Test bounds from batch operation
      size_t lb_size, ub_size;
      const double* lower_bounds =
          libpapilo_problem_get_lower_bounds( problem, &lb_size );
      const double* upper_bounds =
          libpapilo_problem_get_upper_bounds( problem, &ub_size );
      REQUIRE( lb_size == 3 );
      REQUIRE( ub_size == 3 );

      REQUIRE( lower_bounds[0] == 0.0 );
      REQUIRE( ( std::isinf( lower_bounds[1] ) && lower_bounds[1] < 0 ) );
      REQUIRE( lower_bounds[2] == 1.0 );

      REQUIRE( upper_bounds[0] == 10.0 );
      REQUIRE( upper_bounds[1] == 5.0 );
      REQUIRE( ( std::isinf( upper_bounds[2] ) && upper_bounds[2] > 0 ) );

      // Test constraint bounds from batch operation
      size_t lhs_size, rhs_size;
      const double* lhs = libpapilo_problem_get_row_lhs( problem, &lhs_size );
      const double* rhs = libpapilo_problem_get_row_rhs( problem, &rhs_size );
      REQUIRE( lhs_size == 2 );
      REQUIRE( rhs_size == 2 );

      REQUIRE( lhs[0] == 2.0 );
      REQUIRE( ( std::isinf( lhs[1] ) && lhs[1] < 0 ) );
      REQUIRE( ( std::isinf( rhs[0] ) && rhs[0] > 0 ) );
      REQUIRE( rhs[1] == 10.0 );

      libpapilo_problem_free( problem );
      libpapilo_problem_builder_free( builder );
   }

   SECTION( "integer variables" )
   {
      // Test Purpose: Verify integer variable designation works correctly.
      // Tests both individual (set_col_integral) and batch
      // (set_col_integral_all) methods for setting variable integrality.
      // Validates that the problem correctly distinguishes between integer and
      // continuous variables.
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

      // Test integer/continuous variable counts
      REQUIRE( libpapilo_problem_get_num_integral_cols( problem ) == 2 );
      REQUIRE( libpapilo_problem_get_num_continuous_cols( problem ) == 1 );

      // Test column flags for integrality
      uint8_t col0_flags = libpapilo_problem_get_col_flags( problem, 0 );
      uint8_t col1_flags = libpapilo_problem_get_col_flags( problem, 1 );
      uint8_t col2_flags = libpapilo_problem_get_col_flags( problem, 2 );

      // col0 and col2 should have INTEGRAL flag
      REQUIRE( ( col0_flags & LIBPAPILO_COLFLAG_INTEGRAL ) != 0 );
      REQUIRE( ( col1_flags & LIBPAPILO_COLFLAG_INTEGRAL ) == 0 );
      REQUIRE( ( col2_flags & LIBPAPILO_COLFLAG_INTEGRAL ) != 0 );

      libpapilo_problem_free( problem );
      libpapilo_problem_builder_free( builder );
   }

   SECTION( "row and column entry methods" )
   {
      // Test Purpose: Verify alternative matrix entry construction methods work
      // correctly. Tests add_row_entries (adding multiple entries for a single
      // row) and add_col_entries (adding multiple entries for a single column)
      // methods. Uses a different problem structure (4 variables, 3
      // constraints) to validate these specialized entry addition functions.
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
