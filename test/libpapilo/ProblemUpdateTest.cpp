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

// Helper function to setup problem for singleton row test
static libpapilo_problem_t*
setupProblemPresolveSingletonRow()
{
   libpapilo_problem_builder_t* builder = libpapilo_problem_builder_create();

   // Problem setup:
   // 2x + y + z = 3
   // z = 1
   // with x, y, z ∈ [0, 3] for x, [0, 7] for y and z, all integer

   libpapilo_problem_builder_set_num_rows( builder, 2 );
   libpapilo_problem_builder_set_num_cols( builder, 3 );
   libpapilo_problem_builder_reserve( builder, 4, 2, 3 );

   // Set objective coefficients
   double obj[] = { 3.0, 1.0, 1.0 };
   libpapilo_problem_builder_set_obj_all( builder, obj );
   libpapilo_problem_builder_set_obj_offset( builder, 0.0 );

   // Set variable bounds
   double lb[] = { 0.0, 0.0, 0.0 };
   double ub[] = { 3.0, 7.0, 7.0 };
   libpapilo_problem_builder_set_col_lb_all( builder, lb );
   libpapilo_problem_builder_set_col_ub_all( builder, ub );

   // Set variables as integral
   uint8_t integral[] = { 1, 1, 1 };
   libpapilo_problem_builder_set_col_integral_all( builder, integral );

   // Set row bounds (RHS only)
   double rhs[] = { 3.0, 1.0 };
   libpapilo_problem_builder_set_row_rhs_all( builder, rhs );

   // Add matrix entries
   libpapilo_problem_builder_add_entry( builder, 0, 0, 2.0 ); // 2x
   libpapilo_problem_builder_add_entry( builder, 0, 1, 1.0 ); // y
   libpapilo_problem_builder_add_entry( builder, 0, 2, 1.0 ); // z
   libpapilo_problem_builder_add_entry( builder, 1, 2, 1.0 ); // z = 1

   // Set names
   libpapilo_problem_builder_set_problem_name( builder,
                                               "matrix for singleton row" );
   const char* col_names[] = { "x", "y", "z" };
   const char* row_names[] = { "A1", "A2" };
   libpapilo_problem_builder_set_col_name_all( builder, col_names );
   libpapilo_problem_builder_set_row_name_all( builder, row_names );

   libpapilo_problem_t* problem = libpapilo_problem_builder_build( builder );
   libpapilo_problem_builder_free( builder );

   return problem;
}

// Helper function to setup problem for singleton row fixed test
static libpapilo_problem_t*
setupProblemPresolveSingletonRowFixed()
{
   libpapilo_problem_builder_t* builder = libpapilo_problem_builder_create();

   // Same problem as above but with LHS = RHS for row 1
   libpapilo_problem_builder_set_num_rows( builder, 2 );
   libpapilo_problem_builder_set_num_cols( builder, 3 );
   libpapilo_problem_builder_reserve( builder, 4, 2, 3 );

   // Set objective coefficients
   double obj[] = { 3.0, 1.0, 1.0 };
   libpapilo_problem_builder_set_obj_all( builder, obj );
   libpapilo_problem_builder_set_obj_offset( builder, 0.0 );

   // Set variable bounds
   double lb[] = { 0.0, 0.0, 0.0 };
   double ub[] = { 3.0, 7.0, 7.0 };
   libpapilo_problem_builder_set_col_lb_all( builder, lb );
   libpapilo_problem_builder_set_col_ub_all( builder, ub );

   // Set variables as integral
   uint8_t integral[] = { 1, 1, 1 };
   libpapilo_problem_builder_set_col_integral_all( builder, integral );

   // Set row bounds
   double rhs[] = { 3.0, 1.0 };
   libpapilo_problem_builder_set_row_rhs_all( builder, rhs );

   // Add matrix entries
   libpapilo_problem_builder_add_entry( builder, 0, 0, 2.0 ); // 2x
   libpapilo_problem_builder_add_entry( builder, 0, 1, 1.0 ); // y
   libpapilo_problem_builder_add_entry( builder, 0, 2, 1.0 ); // z
   libpapilo_problem_builder_add_entry( builder, 1, 2, 1.0 ); // z = 1

   // Set names
   libpapilo_problem_builder_set_problem_name(
       builder, "matrix for singleton row fixed" );
   const char* col_names[] = { "x", "y", "z" };
   const char* row_names[] = { "A1", "A2" };
   libpapilo_problem_builder_set_col_name_all( builder, col_names );
   libpapilo_problem_builder_set_row_name_all( builder, row_names );

   libpapilo_problem_t* problem = libpapilo_problem_builder_build( builder );

   // Modify row 1 to have LHS = RHS
   libpapilo_problem_modify_row_lhs( problem, 1, 1.0 );

   libpapilo_problem_builder_free( builder );

   return problem;
}

TEST_CASE( "trivial-presolve-singleton-row", "[libpapilo]" )
{
   // Create problem
   libpapilo_problem_t* problem = setupProblemPresolveSingletonRow();

   // Create required objects
   libpapilo_num_t* num = libpapilo_num_create();
   libpapilo_presolve_options_t* options = libpapilo_presolve_options_create();
   libpapilo_presolve_options_set_dualreds( options, 0 );
   libpapilo_statistics_t* statistics = libpapilo_statistics_create();
   libpapilo_postsolve_storage_t* postsolve =
       libpapilo_postsolve_storage_create( problem, num, options );
   libpapilo_message_t* message = libpapilo_message_create();

   // Create ProblemUpdate
   libpapilo_problem_update_t* update = libpapilo_problem_update_create(
       problem, postsolve, statistics, options, num, message );

   // Execute trivial presolve
   libpapilo_presolve_status_t status =
       libpapilo_problem_update_trivial_presolve( update );

   // Check that presolve succeeded (not infeasible or unbounded)
   REQUIRE( ( status == LIBPAPILO_PRESOLVE_STATUS_UNCHANGED ||
              status == LIBPAPILO_PRESOLVE_STATUS_REDUCED ) );

   // Check results
   int size;
   const double* upper_bounds =
       libpapilo_problem_get_upper_bounds( problem, &size );
   REQUIRE( upper_bounds[2] == 1.0 );

   REQUIRE( libpapilo_problem_is_row_redundant( problem, 1 ) == 1 );

   // Clean up
   libpapilo_problem_update_free( update );
   libpapilo_message_free( message );
   libpapilo_postsolve_storage_free( postsolve );
   libpapilo_statistics_free( statistics );
   libpapilo_presolve_options_free( options );
   libpapilo_num_free( num );
   libpapilo_problem_free( problem );
}

TEST_CASE( "trivial-presolve-singleton-row-pt-2", "[libpapilo]" )
{
   // Create problem
   libpapilo_problem_t* problem = setupProblemPresolveSingletonRowFixed();

   // Create required objects
   libpapilo_num_t* num = libpapilo_num_create();
   libpapilo_presolve_options_t* options = libpapilo_presolve_options_create();
   libpapilo_presolve_options_set_dualreds( options, 0 );
   libpapilo_statistics_t* statistics = libpapilo_statistics_create();
   libpapilo_postsolve_storage_t* postsolve =
       libpapilo_postsolve_storage_create( problem, num, options );
   libpapilo_message_t* message = libpapilo_message_create();

   // Create ProblemUpdate
   libpapilo_problem_update_t* update = libpapilo_problem_update_create(
       problem, postsolve, statistics, options, num, message );

   // Execute trivial presolve
   libpapilo_presolve_status_t status =
       libpapilo_problem_update_trivial_presolve( update );

   // Check that presolve succeeded (not infeasible or unbounded)
   REQUIRE( ( status == LIBPAPILO_PRESOLVE_STATUS_UNCHANGED ||
              status == LIBPAPILO_PRESOLVE_STATUS_REDUCED ) );

   // Check results
   int size;
   const double* upper_bounds =
       libpapilo_problem_get_upper_bounds( problem, &size );
   const double* lower_bounds =
       libpapilo_problem_get_lower_bounds( problem, &size );

   REQUIRE( upper_bounds[2] == 1.0 );
   REQUIRE( lower_bounds[2] == 1.0 );
   REQUIRE( libpapilo_problem_is_row_redundant( problem, 1 ) == 1 );

   int singleton_cols_count =
       libpapilo_problem_update_get_singleton_cols_count( update );
   REQUIRE( singleton_cols_count == 2 );

   // Clean up
   libpapilo_problem_update_free( update );
   libpapilo_message_free( message );
   libpapilo_postsolve_storage_free( postsolve );
   libpapilo_statistics_free( statistics );
   libpapilo_presolve_options_free( options );
   libpapilo_num_free( num );
   libpapilo_problem_free( problem );
}

TEST_CASE( "problem-update-owns-num-and-message", "[libpapilo]" )
{
   // Create problem
   libpapilo_problem_t* problem = setupProblemPresolveSingletonRow();

   // Create required objects
   libpapilo_num_t* num = libpapilo_num_create();
   libpapilo_presolve_options_t* options = libpapilo_presolve_options_create();
   libpapilo_presolve_options_set_dualreds( options, 0 );
   libpapilo_statistics_t* statistics = libpapilo_statistics_create();
   libpapilo_postsolve_storage_t* postsolve =
       libpapilo_postsolve_storage_create( problem, num, options );
   libpapilo_message_t* message = libpapilo_message_create();

   // Create ProblemUpdate
   libpapilo_problem_update_t* update = libpapilo_problem_update_create(
       problem, postsolve, statistics, options, num, message );

   // Immediately free original num and message to verify that update holds
   // its own copies and remains fully functional.
   libpapilo_message_free( message );
   libpapilo_num_free( num );

   // Execute trivial presolve; should work without accessing freed objects
   libpapilo_presolve_status_t status =
       libpapilo_problem_update_trivial_presolve( update );

   // Check that presolve succeeded (not infeasible or unbounded)
   REQUIRE( ( status == LIBPAPILO_PRESOLVE_STATUS_UNCHANGED ||
              status == LIBPAPILO_PRESOLVE_STATUS_REDUCED ) );

   // Check results
   int size;
   const double* upper_bounds =
       libpapilo_problem_get_upper_bounds( problem, &size );
   REQUIRE( upper_bounds[2] == 1.0 );
   REQUIRE( libpapilo_problem_is_row_redundant( problem, 1 ) == 1 );

   // Clean up remaining objects
   libpapilo_problem_update_free( update );
   libpapilo_postsolve_storage_free( postsolve );
   libpapilo_statistics_free( statistics );
   libpapilo_presolve_options_free( options );
   libpapilo_problem_free( problem );
}
