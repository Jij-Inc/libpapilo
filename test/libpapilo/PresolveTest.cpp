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

#include "libpapilo.h"
#include "papilo/external/catch/catch_amalgamated.hpp"
#include <cstdlib>
#include <vector>

static const int ELIMINATED = -1;

// Helper function to setup problem with multiple presolving options
static libpapilo_problem_t*
setupProblemWithMultiplePresolvingOptions()
{
   // 2x + y + z = 2           for simple probing
   //      z + w = 1           w = singleton column can be replaced & simple
   //                          substitution
   libpapilo_problem_builder_t* builder = libpapilo_problem_builder_create();

   libpapilo_problem_builder_set_num_rows( builder, 2 );
   libpapilo_problem_builder_set_num_cols( builder, 4 );
   libpapilo_problem_builder_reserve( builder, 5, 2, 4 );

   // Set objective coefficients
   double obj[] = { 3.0, 1.0, 1.0, 1.0 };
   libpapilo_problem_builder_set_obj_all( builder, obj );
   libpapilo_problem_builder_set_obj_offset( builder, 0.0 );

   // Set variable bounds
   double lb[] = { 0.0, 0.0, 0.0, 0.0 };
   double ub[] = { 1.0, 1.0, 1.0, 1.0 };
   libpapilo_problem_builder_set_col_lb_all( builder, lb );
   libpapilo_problem_builder_set_col_ub_all( builder, ub );

   // Set variables as integral
   uint8_t integral[] = { 1, 1, 1, 1 };
   libpapilo_problem_builder_set_col_integral_all( builder, integral );

   // Set row bounds (both LHS and RHS)
   double rhs[] = { 2.0, 1.0 };
   double lhs[] = { 2.0, 1.0 };
   libpapilo_problem_builder_set_row_rhs_all( builder, rhs );
   libpapilo_problem_builder_set_row_lhs_all( builder, lhs );

   // Add matrix entries
   libpapilo_problem_builder_add_entry( builder, 0, 0, 2.0 ); // 2x
   libpapilo_problem_builder_add_entry( builder, 0, 1, 1.0 ); // y
   libpapilo_problem_builder_add_entry( builder, 0, 2, 1.0 ); // z
   libpapilo_problem_builder_add_entry( builder, 1, 2, 1.0 ); // z
   libpapilo_problem_builder_add_entry( builder, 1, 3, 1.0 ); // w

   // Set names
   libpapilo_problem_builder_set_problem_name(
       builder, "matrix for testing with multiple options" );
   const char* col_names[] = { "c1", "c2", "c3", "c4" };
   const char* row_names[] = { "A1", "A2" };
   libpapilo_problem_builder_set_col_name_all( builder, col_names );
   libpapilo_problem_builder_set_row_name_all( builder, row_names );

   libpapilo_problem_t* problem = libpapilo_problem_builder_build( builder );
   libpapilo_problem_builder_free( builder );

   return problem;
}

// Helper function to apply reductions
static std::pair<int, int>
applyReductions( libpapilo_problem_t* problem,
                 libpapilo_reductions_t* reductions,
                 bool postpone_substitutions )
{
   // Create required objects
   libpapilo_num_t* num = libpapilo_num_create();
   libpapilo_presolve_options_t* options = libpapilo_presolve_options_create();
   libpapilo_statistics_t* statistics = libpapilo_statistics_create();
   libpapilo_postsolve_storage_t* postsolve =
       libpapilo_postsolve_storage_create( problem, num, options );
   libpapilo_message_t* message = libpapilo_message_create();

   // Create ProblemUpdate
   libpapilo_problem_update_t* update = libpapilo_problem_update_create(
       problem, postsolve, statistics, options, num, message );

   // Set up problem state
   libpapilo_problem_recompute_locks( problem );
   libpapilo_problem_update_trivial_column_presolve( update );
   libpapilo_problem_recompute_all_activities( problem );
   libpapilo_problem_update_set_postpone_substitutions(
       update, postpone_substitutions ? 1 : 0 );

   // Apply reductions using presolve
   libpapilo_presolve_t* presolve = libpapilo_presolve_create();
   libpapilo_presolve_add_default_presolvers( presolve );

   int num_rounds = 0;
   int num_changes = 0;
   libpapilo_presolve_apply_reductions( presolve, 0, reductions, update,
                                        &num_rounds, &num_changes );

   // Clean up
   libpapilo_presolve_free( presolve );
   libpapilo_problem_update_free( update );
   libpapilo_message_free( message );
   libpapilo_postsolve_storage_free( postsolve );
   libpapilo_statistics_free( statistics );
   libpapilo_presolve_options_free( options );
   libpapilo_num_free( num );

   return { num_rounds, num_changes };
}

// Helper to get matrix entry
static double
getEntry( libpapilo_problem_t* problem, int row, int column )
{
   const int* cols;
   const double* vals;
   int size = libpapilo_problem_get_row_entries( problem, row, &cols, &vals );
   if( size < 0 || column >= size )
      return 0.0;
   return vals[column];
}

// Helper to get column index in row
static int
getRowIndex( libpapilo_problem_t* problem, int row, int column )
{
   const int* cols;
   const double* vals;
   int size = libpapilo_problem_get_row_entries( problem, row, &cols, &vals );
   if( size < 0 || column >= size )
      return -1;
   return cols[column];
}

TEST_CASE( "replacing-variables-is-postponed-by-flag", "[libpapilo]" )
{
   libpapilo_problem_t* problem = setupProblemWithMultiplePresolvingOptions();
   libpapilo_reductions_t* reductions = libpapilo_reductions_create();

   // substitute x = 1 - y (result of simple probing)
   libpapilo_reductions_replace_col( reductions, 0, 1, -1, 0 );
   libpapilo_reductions_replace_col( reductions, 0, 2, -1, 0 );

   std::pair<int, int> result = applyReductions( problem, reductions, true );

   REQUIRE( result.first == 2 );
   REQUIRE( result.second == 0 );

   libpapilo_reductions_free( reductions );
   libpapilo_problem_free( problem );
}

TEST_CASE( "happy-path-replace-variable", "[libpapilo]" )
{
   libpapilo_problem_t* problem = setupProblemWithMultiplePresolvingOptions();
   libpapilo_reductions_t* reductions = libpapilo_reductions_create();

   // substitute x = 1 - y (result of simple probing)
   libpapilo_reductions_replace_col( reductions, 0, 1, -1, 0 );
   libpapilo_reductions_replace_col( reductions, 0, 2, -1, 0 );

   std::pair<int, int> result = applyReductions( problem, reductions, false );

   REQUIRE( result.first == 2 );
   REQUIRE( result.second == 2 );

   // Check objective coefficients
   int size;
   double* obj_coeffs =
       libpapilo_problem_get_objective_coefficients_mutable( problem, &size );
   REQUIRE( size == 4 );
   REQUIRE( obj_coeffs[0] == 0.0 );
   REQUIRE( obj_coeffs[1] == -2.0 );
   REQUIRE( obj_coeffs[2] == 1.0 );
   REQUIRE( obj_coeffs[3] == 1.0 );

   // Check problem structure
   REQUIRE( libpapilo_problem_get_nrows( problem ) == 2 );

   // Check column sizes
   const int* col_sizes = libpapilo_problem_get_col_sizes( problem, &size );
   REQUIRE( col_sizes[0] == ELIMINATED );
   REQUIRE( col_sizes[1] == 1 );
   REQUIRE( col_sizes[2] == 2 );
   REQUIRE( col_sizes[3] == 1 );

   // Check constraint RHS
   const double* rhs_vals =
       libpapilo_problem_get_row_right_hand_sides( problem, &size );
   REQUIRE( rhs_vals[0] == 2 );
   REQUIRE( rhs_vals[1] == 1 );

   // Check constraint LHS
   const double* lhs_vals =
       libpapilo_problem_get_row_left_hand_sides( problem, &size );
   REQUIRE( lhs_vals[0] == 2 );
   REQUIRE( lhs_vals[1] == 1 );

   // Check row flags
   REQUIRE( libpapilo_problem_is_row_redundant( problem, 0 ) == 0 );

   // Check column substituted flag
   REQUIRE( libpapilo_problem_is_col_substituted( problem, 0 ) == 1 );

   // Check row sizes
   const int* row_sizes = libpapilo_problem_get_row_sizes( problem, &size );
   REQUIRE( row_sizes[0] == 2 );
   REQUIRE( row_sizes[1] == 2 );

   // Check matrix entries for row 0
   REQUIRE( getRowIndex( problem, 0, 0 ) == 1 );
   REQUIRE( getRowIndex( problem, 0, 1 ) == 2 );
   REQUIRE( getEntry( problem, 0, 0 ) == -1 );
   REQUIRE( getEntry( problem, 0, 1 ) == 1.0 );

   // Check matrix entries for row 1
   REQUIRE( getRowIndex( problem, 1, 0 ) == 2 );
   REQUIRE( getRowIndex( problem, 1, 1 ) == 3 );
   REQUIRE( getEntry( problem, 1, 0 ) == 1.0 );
   REQUIRE( getEntry( problem, 1, 1 ) == 1.0 );

   libpapilo_reductions_free( reductions );
   libpapilo_problem_free( problem );
}

TEST_CASE( "happy-path-substitute-matrix-coefficient-into-objective",
           "[libpapilo]" )
{
   libpapilo_problem_t* problem = setupProblemWithMultiplePresolvingOptions();
   libpapilo_reductions_t* reductions = libpapilo_reductions_create();

   // Use transaction for atomic operations
   libpapilo_reductions_begin_transaction( reductions );
   libpapilo_reductions_lock_col_bounds( reductions, 3 );
   libpapilo_reductions_lock_row( reductions, 1 );
   libpapilo_reductions_substitute_col_in_objective( reductions, 3, 1 );
   libpapilo_reductions_mark_row_redundant( reductions, 1 );
   libpapilo_reductions_end_transaction( reductions );

   applyReductions( problem, reductions, false );

   // Check objective coefficients
   int size;
   double* obj_coeffs =
       libpapilo_problem_get_objective_coefficients_mutable( problem, &size );
   REQUIRE( obj_coeffs[0] == 3.0 );
   REQUIRE( obj_coeffs[1] == 1.0 );
   REQUIRE( obj_coeffs[2] == 0.0 );
   REQUIRE( obj_coeffs[3] == 0.0 );

   // Check problem structure
   REQUIRE( libpapilo_problem_get_nrows( problem ) == 2 );

   // Check upper bounds
   const double* upper_bounds =
       libpapilo_problem_get_upper_bounds( problem, &size );
   REQUIRE( upper_bounds[0] == 1.0 );
   REQUIRE( upper_bounds[1] == 1.0 );
   REQUIRE( upper_bounds[2] == 1.0 );
   REQUIRE( upper_bounds[3] == 0.0 );

   // Check column flags
   REQUIRE( libpapilo_problem_is_col_substituted( problem, 3 ) == 1 );

   // Check row flags
   REQUIRE( libpapilo_problem_is_row_redundant( problem, 1 ) == 1 );

   // Check row sizes
   const int* row_sizes = libpapilo_problem_get_row_sizes( problem, &size );
   REQUIRE( row_sizes[0] == 3 );
   REQUIRE( row_sizes[1] == 2 );

   // Check matrix entries for row 1
   REQUIRE( getRowIndex( problem, 1, 0 ) == 2 );
   REQUIRE( getRowIndex( problem, 1, 1 ) == 3 );
   REQUIRE( getEntry( problem, 1, 0 ) == 1.0 );
   REQUIRE( getEntry( problem, 1, 1 ) == 1.0 );

   libpapilo_reductions_free( reductions );
   libpapilo_problem_free( problem );
}

TEST_CASE( "happy-path-aggregate-free-column", "[libpapilo]" )
{
   libpapilo_problem_t* problem = setupProblemWithMultiplePresolvingOptions();
   libpapilo_reductions_t* reductions = libpapilo_reductions_create();

   // replace last column with z = 1 - w
   libpapilo_reductions_begin_transaction( reductions );
   libpapilo_reductions_lock_col_bounds( reductions, 3 );
   libpapilo_reductions_lock_row( reductions, 1 );
   libpapilo_reductions_aggregate_free_col( reductions, 3, 1 );
   libpapilo_reductions_end_transaction( reductions );

   std::pair<int, int> result = applyReductions( problem, reductions, false );

   REQUIRE( result.first == 1 );
   REQUIRE( result.second == 1 );

   // Check objective coefficients
   int size;
   double* obj_coeffs =
       libpapilo_problem_get_objective_coefficients_mutable( problem, &size );
   REQUIRE( obj_coeffs[0] == 3.0 );
   REQUIRE( obj_coeffs[1] == 1.0 );
   REQUIRE( obj_coeffs[2] == 0.0 );
   REQUIRE( obj_coeffs[3] == 0.0 );

   // Check bounds
   const double* upper_bounds =
       libpapilo_problem_get_upper_bounds( problem, &size );
   REQUIRE( upper_bounds[0] == 1.0 );
   REQUIRE( upper_bounds[1] == 1.0 );
   REQUIRE( upper_bounds[2] == 1.0 );
   REQUIRE( upper_bounds[3] == 1.0 );

   const double* lower_bounds =
       libpapilo_problem_get_lower_bounds( problem, &size );
   REQUIRE( lower_bounds[0] == 0.0 );
   REQUIRE( lower_bounds[1] == 0.0 );
   REQUIRE( lower_bounds[2] == 0.0 );
   REQUIRE( lower_bounds[3] == 0.0 );

   // Check column sizes
   const int* col_sizes = libpapilo_problem_get_col_sizes( problem, &size );
   REQUIRE( col_sizes[0] == 1 );
   REQUIRE( col_sizes[1] == 1 );
   REQUIRE( col_sizes[2] == 1 );
   REQUIRE( col_sizes[3] == ELIMINATED );

   REQUIRE( libpapilo_problem_get_nrows( problem ) == 2 );
   REQUIRE( libpapilo_problem_is_row_redundant( problem, 1 ) == 1 );

   libpapilo_reductions_free( reductions );
   libpapilo_problem_free( problem );
}