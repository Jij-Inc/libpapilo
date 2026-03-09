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

// Setup problem with parallel columns
// Based on ParallelColDetectionTest.cpp::setupProblemWithParallelColumns
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

TEST_CASE( "mark_parallel_cols_creates_reduction", "[libpapilo][parallel_col]" )
{
   // Test that mark_parallel_cols creates a reduction entry
   libpapilo_reductions_t* reductions = libpapilo_reductions_create();

   // Begin transaction
   libpapilo_reductions_begin_transaction( reductions );

   // Lock both columns
   libpapilo_reductions_lock_col_bounds( reductions, 0 );
   libpapilo_reductions_lock_col_bounds( reductions, 1 );

   // Mark columns as parallel (col1=1 merged into col2=0)
   libpapilo_reductions_mark_parallel_cols( reductions, 1, 0 );

   // End transaction
   libpapilo_reductions_end_transaction( reductions );

   // Verify reduction was created
   int size = libpapilo_reductions_get_size( reductions );
   REQUIRE( size == 3 ); // 2 locks + 1 parallel

   // Check the parallel reduction entry
   libpapilo_reduction_info_t info =
       libpapilo_reductions_get_info( reductions, 2 );

   // row should be ColReduction::PARALLEL = -12
   REQUIRE( info.row == LIBPAPILO_COL_REDUCTION_PARALLEL );
   // col should be col1 (the eliminated column)
   REQUIRE( info.col == 1 );
   // newval should be col2 (the merged column)
   REQUIRE( info.newval == 0.0 );

   libpapilo_reductions_free( reductions );
}

TEST_CASE( "parallel_col_apply_reductions", "[libpapilo][parallel_col]" )
{
   // Test applying parallel column reductions through ProblemUpdate
   libpapilo_problem_t* problem = setupProblemWithParallelColumns(
       false, false, 2.0, 10.0, 10.0, 0.0, 0.0, false );

   libpapilo_num_t* num = libpapilo_num_create();
   libpapilo_presolve_options_t* options = libpapilo_presolve_options_create();
   libpapilo_statistics_t* stats = libpapilo_statistics_create();
   libpapilo_postsolve_storage_t* postsolve =
       libpapilo_postsolve_storage_create( problem, num, options );
   libpapilo_message_t* message = libpapilo_message_create();
   libpapilo_problem_update_t* update = libpapilo_problem_update_create(
       problem, postsolve, stats, options, num, message );

   // Setup problem state
   libpapilo_problem_recompute_locks( problem );
   libpapilo_problem_update_trivial_column_presolve( update );
   libpapilo_problem_recompute_all_activities( problem );

   // Create reductions
   libpapilo_reductions_t* reductions = libpapilo_reductions_create();

   // Begin transaction
   libpapilo_reductions_begin_transaction( reductions );

   // Lock both columns
   libpapilo_reductions_lock_col_bounds( reductions, 0 );
   libpapilo_reductions_lock_col_bounds( reductions, 1 );

   // Mark columns as parallel (col1=1 merged into col2=0)
   libpapilo_reductions_mark_parallel_cols( reductions, 1, 0 );

   // End transaction
   libpapilo_reductions_end_transaction( reductions );

   // Apply reductions
   libpapilo_presolve_t* presolve = libpapilo_presolve_create( message );
   libpapilo_presolve_add_default_presolvers( presolve );

   int num_rounds = 0;
   int num_changes = 0;
   libpapilo_presolve_apply_reductions( presolve, 0, reductions, update,
                                        &num_rounds, &num_changes );

   REQUIRE( num_rounds == 1 );
   REQUIRE( num_changes == 1 );

   // Verify column 1 is now substituted/eliminated
   REQUIRE( libpapilo_problem_is_col_substituted( problem, 1 ) == 1 );

   // Check postsolve storage has the ParallelCol entry
   size_t types_size = 0;
   const libpapilo_postsolve_reduction_type_t* types =
       libpapilo_postsolve_storage_get_types( postsolve, &types_size );
   REQUIRE( types != NULL );
   REQUIRE( types_size > 0 );

   // Find the ParallelCol entry
   bool found_parallel_col = false;
   for( size_t i = 0; i < types_size; i++ )
   {
      if( types[i] == LIBPAPILO_POSTSOLVE_REDUCTION_PARALLEL_COL )
      {
         found_parallel_col = true;
         break;
      }
   }
   REQUIRE( found_parallel_col );

   // Cleanup
   libpapilo_presolve_free( presolve );
   libpapilo_reductions_free( reductions );
   libpapilo_problem_update_free( update );
   libpapilo_message_free( message );
   libpapilo_postsolve_storage_free( postsolve );
   libpapilo_statistics_free( stats );
   libpapilo_presolve_options_free( options );
   libpapilo_num_free( num );
   libpapilo_problem_free( problem );
}
