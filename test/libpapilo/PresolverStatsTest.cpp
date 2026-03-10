/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                           */
/* This file is part of the library libpapilo, a fork of PaPILO from ZIB     */
/*                                                                           */
/* Copyright (C) 2025      Jij-Inc.                                          */
/*                                                                           */
/* Licensed under the Apache License, Version 2.0 (the "License");           */
/* you may not use this file except in compliance with the License.          */
/* You may obtain a copy of the License at                                   */
/*                                                                           */
/*     http://www.apache.org/licenses/LICENSE-2.0                            */
/*                                                                           */
/* Unless required by applicable law or agreed to in writing, software       */
/* distributed under the License is distributed on an "AS IS" BASIS,         */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  */
/* See the License for the specific language governing permissions and       */
/* limitations under the License.                                            */
/*                                                                           */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "libpapilo.h"
#include "papilo/external/catch/catch_amalgamated.hpp"
#include <cstring>

// Helper function to create a simple test problem
static libpapilo_problem_t*
create_test_problem()
{
   auto* builder = libpapilo_problem_builder_create();

   // Create a simple problem with 3 variables and 2 constraints
   libpapilo_problem_builder_set_num_cols( builder, 3 );
   libpapilo_problem_builder_set_num_rows( builder, 2 );

   // Set objective: minimize x + 2y + 3z
   double obj[] = { 1.0, 2.0, 3.0 };
   libpapilo_problem_builder_set_obj_all( builder, obj );

   // Set variable bounds
   double lb[] = { 0.0, 0.0, 0.0 };
   double ub[] = { 10.0, 10.0, 10.0 };
   libpapilo_problem_builder_set_col_lb_all( builder, lb );
   libpapilo_problem_builder_set_col_ub_all( builder, ub );

   // Set constraints
   double lhs[] = { 1.0, 2.0 };
   double rhs[] = { 5.0, 8.0 };
   libpapilo_problem_builder_set_row_lhs_all( builder, lhs );
   libpapilo_problem_builder_set_row_rhs_all( builder, rhs );

   // Add constraint matrix entries
   // Row 0: x + y <= 5
   libpapilo_problem_builder_add_entry( builder, 0, 0, 1.0 );
   libpapilo_problem_builder_add_entry( builder, 0, 1, 1.0 );

   // Row 1: 2x + z >= 2
   libpapilo_problem_builder_add_entry( builder, 1, 0, 2.0 );
   libpapilo_problem_builder_add_entry( builder, 1, 2, 1.0 );

   // Make first variable integer
   uint8_t integral[] = { 1, 0, 0 };
   libpapilo_problem_builder_set_col_integral_all( builder, integral );

   auto* problem = libpapilo_problem_builder_build( builder );
   libpapilo_problem_builder_free( builder );

   return problem;
}

TEST_CASE( "per-presolver-statistics-are-tracked-correctly",
           "[presolve][statistics]" )
{
   // Create a test problem
   auto* problem = create_test_problem();
   REQUIRE( problem != nullptr );

   // Create message handler
   auto* message = libpapilo_message_create();
   REQUIRE( message != nullptr );
   libpapilo_message_set_verbosity_level( message, 0 ); // quiet

   // Create and configure presolve
   auto* presolve = libpapilo_presolve_create( message );
   REQUIRE( presolve != nullptr );
   libpapilo_presolve_add_default_presolvers( presolve );

   // Run presolve
   libpapilo_postsolve_storage_t* postsolve = nullptr;
   libpapilo_statistics_t* statistics = nullptr;

   auto status = libpapilo_presolve_apply_full( presolve, problem, &postsolve,
                                                &statistics );

   // Check that presolve ran successfully
   REQUIRE( status != LIBPAPILO_PRESOLVE_STATUS_INFEASIBLE );
   REQUIRE( status != LIBPAPILO_PRESOLVE_STATUS_UNBOUNDED );
   REQUIRE( status != LIBPAPILO_PRESOLVE_STATUS_UNBOUNDED_OR_INFEASIBLE );

   // Check that statistics were created
   REQUIRE( statistics != nullptr );

   // Get overall statistics
   size_t nrounds = libpapilo_statistics_get_nrounds( statistics );
   size_t ndeletedcols = libpapilo_statistics_get_ndeletedcols( statistics );
   size_t ndeletedrows = libpapilo_statistics_get_ndeletedrows( statistics );
   double presolvetime = libpapilo_statistics_get_presolvetime( statistics );

   INFO( "Presolve rounds: " << nrounds );
   INFO( "Deleted columns: " << ndeletedcols );
   INFO( "Deleted rows: " << ndeletedrows );
   INFO( "Presolve time: " << presolvetime << " seconds" );

   // Get per-presolver statistics
   size_t num_presolvers =
       libpapilo_statistics_get_num_presolvers( statistics );
   REQUIRE( num_presolvers > 0 );
   INFO( "Number of presolvers: " << num_presolvers );

   // Track if any presolver was successful
   bool any_successful = false;
   size_t total_transactions = 0;
   int total_applied = 0;

   for( size_t i = 0; i < num_presolvers; ++i )
   {
      const char* name =
          libpapilo_statistics_get_presolver_name( statistics, i );
      size_t ncalls =
          libpapilo_statistics_get_presolver_ncalls( statistics, i );
      size_t nsuccessful =
          libpapilo_statistics_get_presolver_nsuccessful( statistics, i );
      size_t ntransactions =
          libpapilo_statistics_get_presolver_ntransactions( statistics, i );
      size_t napplied =
          libpapilo_statistics_get_presolver_napplied( statistics, i );
      double exectime =
          libpapilo_statistics_get_presolver_exectime( statistics, i );

      REQUIRE( name != nullptr );
      REQUIRE( strlen( name ) > 0 );
      REQUIRE( ncalls >= 0 );
      REQUIRE( nsuccessful >= 0 );
      REQUIRE( nsuccessful <= ncalls );
      REQUIRE( ntransactions >= 0 );
      REQUIRE( napplied >= 0 );
      REQUIRE( napplied <= ntransactions );
      REQUIRE( exectime >= 0.0 );

      if( nsuccessful > 0 )
      {
         any_successful = true;
         INFO( "Presolver '" << name << "' was successful " << nsuccessful
                             << " times out of " << ncalls << " calls" );
         INFO( "  Applied " << napplied << " out of " << ntransactions
                            << " transactions" );
         INFO( "  Execution time: " << exectime << " seconds" );
      }

      total_transactions += ntransactions;
      total_applied += napplied;
   }

   // If presolve reduced the problem, at least one presolver should have been
   // successful
   if( status == LIBPAPILO_PRESOLVE_STATUS_REDUCED )
   {
      REQUIRE( any_successful );
      INFO( "Total transactions: " << total_transactions );
      INFO( "Total applied: " << total_applied );
   }

   // Clean up
   libpapilo_problem_free( problem );
   libpapilo_presolve_free( presolve );
   libpapilo_message_free( message );
   libpapilo_postsolve_storage_free( postsolve );
   libpapilo_statistics_free( statistics );
}

TEST_CASE( "per-presolver-statistics-match-overall-statistics",
           "[presolve][statistics]" )
{
   // Create a test problem
   auto* problem = create_test_problem();
   REQUIRE( problem != nullptr );

   // Create message handler
   auto* message = libpapilo_message_create();
   REQUIRE( message != nullptr );
   libpapilo_message_set_verbosity_level( message, 0 ); // quiet for tests

   // Create and configure presolve
   auto* presolve = libpapilo_presolve_create( message );
   REQUIRE( presolve != nullptr );
   libpapilo_presolve_add_default_presolvers( presolve );

   // Run presolve
   libpapilo_postsolve_storage_t* postsolve = nullptr;
   libpapilo_statistics_t* statistics = nullptr;

   auto status = libpapilo_presolve_apply_full( presolve, problem, &postsolve,
                                                &statistics );

   // Check that presolve ran successfully
   REQUIRE( status != LIBPAPILO_PRESOLVE_STATUS_INFEASIBLE );
   REQUIRE( status != LIBPAPILO_PRESOLVE_STATUS_UNBOUNDED );
   REQUIRE( status != LIBPAPILO_PRESOLVE_STATUS_UNBOUNDED_OR_INFEASIBLE );

   REQUIRE( statistics != nullptr );

   // Get overall statistics
   size_t overall_applied = libpapilo_statistics_get_ntsxapplied( statistics );

   // Sum up per-presolver applied transactions
   size_t num_presolvers =
       libpapilo_statistics_get_num_presolvers( statistics );
   size_t sum_applied = 0;

   for( size_t i = 0; i < num_presolvers; ++i )
   {
      size_t napplied =
          libpapilo_statistics_get_presolver_napplied( statistics, i );
      sum_applied += napplied;
   }

   // The sum of per-presolver applied transactions should equal overall applied
   INFO( "Overall applied transactions: " << overall_applied );
   INFO( "Sum of per-presolver applied: " << sum_applied );
   REQUIRE( sum_applied == overall_applied );

   // Clean up
   libpapilo_problem_free( problem );
   libpapilo_presolve_free( presolve );
   libpapilo_message_free( message );
   libpapilo_postsolve_storage_free( postsolve );
   libpapilo_statistics_free( statistics );
}
