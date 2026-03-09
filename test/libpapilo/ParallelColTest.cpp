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

/**
 * @file ParallelColTest.cpp
 * @brief C API tests for ParallelColDetection presolver
 *
 * These tests correspond to the C++ tests in:
 *   test/papilo/presolve/ParallelColDetectionTest.cpp
 */

#include "libpapilo.h"
#include "papilo/external/catch/catch_amalgamated.hpp"
#include <cstdlib>

// Setup problem with parallel columns
// Corresponds to setupProblemWithParallelColumns in ParallelColDetectionTest.cpp
static libpapilo_problem_t*
setupProblemWithParallelColumns( bool first_col_int, bool second_col_int,
                                 double factor, double ub_first_col,
                                 double ub_second_col, double lb_first_col,
                                 double lb_second_col, bool objectiveZero )
{
   libpapilo_problem_builder_t* builder = libpapilo_problem_builder_create();

   libpapilo_problem_builder_set_num_rows( builder, 2 );
   libpapilo_problem_builder_set_num_cols( builder, 2 );
   libpapilo_problem_builder_reserve( builder, 4, 2, 2 );

   // Set objective coefficients
   double obj[] = { objectiveZero ? 0.0 : 1.0,
                    objectiveZero ? 0.0 : 1.0 * factor };
   libpapilo_problem_builder_set_obj_all( builder, obj );
   libpapilo_problem_builder_set_obj_offset( builder, 0.0 );

   // Set variable bounds
   double lb[] = { lb_first_col, lb_second_col };
   double ub[] = { ub_first_col, ub_second_col };
   libpapilo_problem_builder_set_col_lb_all( builder, lb );
   libpapilo_problem_builder_set_col_ub_all( builder, ub );

   // Set variables as integral
   uint8_t integral[] = { first_col_int ? (uint8_t)1 : (uint8_t)0,
                          second_col_int ? (uint8_t)1 : (uint8_t)0 };
   libpapilo_problem_builder_set_col_integral_all( builder, integral );

   // Set row bounds (RHS and LHS equal for equality constraints)
   double rhs[] = { 1.0, 2.0 };
   double lhs[] = { 1.0, 2.0 };
   libpapilo_problem_builder_set_row_rhs_all( builder, rhs );
   libpapilo_problem_builder_set_row_lhs_all( builder, lhs );

   // Add matrix entries with parallel structure:
   // Row 0: 1.0 * col0 + factor * col1 = 1
   // Row 1: 2.0 * col0 + 2*factor * col1 = 2
   libpapilo_problem_builder_add_entry( builder, 0, 0, 1.0 );
   libpapilo_problem_builder_add_entry( builder, 0, 1, 1.0 * factor );
   libpapilo_problem_builder_add_entry( builder, 1, 0, 2.0 );
   libpapilo_problem_builder_add_entry( builder, 1, 1, 2.0 * factor );

   // Set names
   libpapilo_problem_builder_set_problem_name(
       builder, "matrix with parallel columns (1 and 2)" );
   const char* col_names[] = { "c1", "c2" };
   const char* row_names[] = { "A1", "A2" };
   libpapilo_problem_builder_set_col_name_all( builder, col_names );
   libpapilo_problem_builder_set_row_name_all( builder, row_names );

   libpapilo_problem_t* problem = libpapilo_problem_builder_build( builder );
   libpapilo_problem_builder_free( builder );

   return problem;
}

// Corresponds to parallel_col_detection_2_integer_columns in C++ test
TEST_CASE( "parallel_col_detection_2_integer_columns",
           "[libpapilo][parallel_col]" )
{
   double time = 0.0;
   int cause = -1;

   libpapilo_problem_t* problem =
       setupProblemWithParallelColumns( true, true, 3.0, 10.0, 10.0, 0.0, 0.0,
                                        false );
   libpapilo_num_t* num = libpapilo_num_create();
   libpapilo_presolve_options_t* options = libpapilo_presolve_options_create();
   libpapilo_statistics_t* stats = libpapilo_statistics_create();
   libpapilo_postsolve_storage_t* postsolve =
       libpapilo_postsolve_storage_create( problem, num, options );
   libpapilo_message_t* message = libpapilo_message_create();
   libpapilo_problem_update_t* update = libpapilo_problem_update_create(
       problem, postsolve, stats, options, num, message );

   libpapilo_problem_update_check_changed_activities( update );

   libpapilo_parallel_col_detection_t* presolver =
       libpapilo_parallel_col_detection_create();
   libpapilo_reductions_t* reductions = libpapilo_reductions_create();
   libpapilo_timer_t* timer = libpapilo_timer_create( &time );

   libpapilo_presolve_status_t status =
       libpapilo_parallel_col_detection_execute( presolver, problem, update,
                                                 num, reductions, timer,
                                                 &cause );

   REQUIRE( status == LIBPAPILO_PRESOLVE_STATUS_REDUCED );
   REQUIRE( libpapilo_reductions_get_size( reductions ) == 3 );

   libpapilo_reduction_info_t info0 =
       libpapilo_reductions_get_info( reductions, 0 );
   REQUIRE( info0.row == LIBPAPILO_COL_REDUCTION_LOCKED );
   REQUIRE( info0.col == 1 );
   REQUIRE( info0.newval == 0 );

   libpapilo_reduction_info_t info1 =
       libpapilo_reductions_get_info( reductions, 1 );
   REQUIRE( info1.row == LIBPAPILO_COL_REDUCTION_LOCKED );
   REQUIRE( info1.col == 0 );
   REQUIRE( info1.newval == 0 );

   libpapilo_reduction_info_t info2 =
       libpapilo_reductions_get_info( reductions, 2 );
   REQUIRE( info2.row == LIBPAPILO_COL_REDUCTION_PARALLEL );
   REQUIRE( info2.col == 1 );
   REQUIRE( info2.newval == 0 );

   // Cleanup
   libpapilo_timer_free( timer );
   libpapilo_reductions_free( reductions );
   libpapilo_parallel_col_detection_free( presolver );
   libpapilo_problem_update_free( update );
   libpapilo_message_free( message );
   libpapilo_postsolve_storage_free( postsolve );
   libpapilo_statistics_free( stats );
   libpapilo_presolve_options_free( options );
   libpapilo_num_free( num );
   libpapilo_problem_free( problem );
}

// Corresponds to parallel_col_detection_2_continuous_columns in C++ test
TEST_CASE( "parallel_col_detection_2_continuous_columns",
           "[libpapilo][parallel_col]" )
{
   double time = 0.0;
   int cause = -1;

   libpapilo_problem_t* problem =
       setupProblemWithParallelColumns( false, false, 2.0, 10.0, 10.0, 0.0, 0.0,
                                        false );
   libpapilo_num_t* num = libpapilo_num_create();
   libpapilo_presolve_options_t* options = libpapilo_presolve_options_create();
   libpapilo_statistics_t* stats = libpapilo_statistics_create();
   libpapilo_postsolve_storage_t* postsolve =
       libpapilo_postsolve_storage_create( problem, num, options );
   libpapilo_message_t* message = libpapilo_message_create();
   libpapilo_problem_update_t* update = libpapilo_problem_update_create(
       problem, postsolve, stats, options, num, message );

   libpapilo_problem_update_check_changed_activities( update );

   libpapilo_parallel_col_detection_t* presolver =
       libpapilo_parallel_col_detection_create();
   libpapilo_reductions_t* reductions = libpapilo_reductions_create();
   libpapilo_timer_t* timer = libpapilo_timer_create( &time );

   libpapilo_presolve_status_t status =
       libpapilo_parallel_col_detection_execute( presolver, problem, update,
                                                 num, reductions, timer,
                                                 &cause );

   REQUIRE( status == LIBPAPILO_PRESOLVE_STATUS_REDUCED );
   REQUIRE( libpapilo_reductions_get_size( reductions ) == 3 );

   libpapilo_reduction_info_t info0 =
       libpapilo_reductions_get_info( reductions, 0 );
   REQUIRE( info0.row == LIBPAPILO_COL_REDUCTION_LOCKED );
   REQUIRE( info0.col == 1 );
   REQUIRE( info0.newval == 0 );

   libpapilo_reduction_info_t info1 =
       libpapilo_reductions_get_info( reductions, 1 );
   REQUIRE( info1.row == LIBPAPILO_COL_REDUCTION_LOCKED );
   REQUIRE( info1.col == 0 );
   REQUIRE( info1.newval == 0 );

   libpapilo_reduction_info_t info2 =
       libpapilo_reductions_get_info( reductions, 2 );
   REQUIRE( info2.row == LIBPAPILO_COL_REDUCTION_PARALLEL );
   REQUIRE( info2.col == 1 );
   REQUIRE( info2.newval == 0 );

   // Cleanup
   libpapilo_timer_free( timer );
   libpapilo_reductions_free( reductions );
   libpapilo_parallel_col_detection_free( presolver );
   libpapilo_problem_update_free( update );
   libpapilo_message_free( message );
   libpapilo_postsolve_storage_free( postsolve );
   libpapilo_statistics_free( stats );
   libpapilo_presolve_options_free( options );
   libpapilo_num_free( num );
   libpapilo_problem_free( problem );
}

// Corresponds to parallel_col_detection_obj_not_parallel in C++ test
TEST_CASE( "parallel_col_detection_obj_not_parallel",
           "[libpapilo][parallel_col]" )
{
   double time = 0.0;
   int cause = -1;

   // Create problem where columns are parallel in constraints but not in
   // objective
   libpapilo_problem_t* problem =
       setupProblemWithParallelColumns( true, true, 1.0, 10.0, 10.0, 0.0, 0.0,
                                        false );

   // Modify objective to break parallelism: obj = [3, 2] instead of [1, 1]
   size_t obj_size = 0;
   double* obj_coeffs =
       libpapilo_problem_get_objective_coefficients_mutable( problem,
                                                             &obj_size );
   obj_coeffs[0] = 3.0;
   obj_coeffs[1] = 2.0;

   libpapilo_num_t* num = libpapilo_num_create();
   libpapilo_presolve_options_t* options = libpapilo_presolve_options_create();
   libpapilo_statistics_t* stats = libpapilo_statistics_create();
   libpapilo_postsolve_storage_t* postsolve =
       libpapilo_postsolve_storage_create( problem, num, options );
   libpapilo_message_t* message = libpapilo_message_create();
   libpapilo_problem_update_t* update = libpapilo_problem_update_create(
       problem, postsolve, stats, options, num, message );

   libpapilo_problem_update_check_changed_activities( update );

   libpapilo_parallel_col_detection_t* presolver =
       libpapilo_parallel_col_detection_create();
   libpapilo_reductions_t* reductions = libpapilo_reductions_create();
   libpapilo_timer_t* timer = libpapilo_timer_create( &time );

   libpapilo_presolve_status_t status =
       libpapilo_parallel_col_detection_execute( presolver, problem, update,
                                                 num, reductions, timer,
                                                 &cause );

   // No reductions because objective coefficients break parallelism
   REQUIRE( status == LIBPAPILO_PRESOLVE_STATUS_UNCHANGED );

   // Cleanup
   libpapilo_timer_free( timer );
   libpapilo_reductions_free( reductions );
   libpapilo_parallel_col_detection_free( presolver );
   libpapilo_problem_update_free( update );
   libpapilo_message_free( message );
   libpapilo_postsolve_storage_free( postsolve );
   libpapilo_statistics_free( stats );
   libpapilo_presolve_options_free( options );
   libpapilo_num_free( num );
   libpapilo_problem_free( problem );
}
