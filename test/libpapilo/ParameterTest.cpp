/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                           */
/* This file is part of the library libpapilo, a fork of PaPILO from ZIB     */
/*                                                                           */
/* Copyright (C) 2020-2025 Zuse Institute Berlin (ZIB)                       */
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

TEST_CASE( "set_param_bool disables presolver", "[parameter]" )
{
   libpapilo_message_t* message = libpapilo_message_create();
   libpapilo_presolve_t* presolve = libpapilo_presolve_create( message );
   libpapilo_presolve_add_default_presolvers( presolve );

   // Disable parallel column detection
   libpapilo_param_result_t result =
       libpapilo_presolve_set_param_bool( presolve, "parallelcols.enabled", 0 );
   REQUIRE( result == LIBPAPILO_PARAM_OK );

   // Disable parallel row detection
   result =
       libpapilo_presolve_set_param_bool( presolve, "parallelrows.enabled", 0 );
   REQUIRE( result == LIBPAPILO_PARAM_OK );

   libpapilo_presolve_free( presolve );
   libpapilo_message_free( message );
}

TEST_CASE( "set_param_bool returns NOT_FOUND for unknown key", "[parameter]" )
{
   libpapilo_message_t* message = libpapilo_message_create();
   libpapilo_presolve_t* presolve = libpapilo_presolve_create( message );
   libpapilo_presolve_add_default_presolvers( presolve );

   libpapilo_param_result_t result =
       libpapilo_presolve_set_param_bool( presolve, "unknown.parameter", 1 );
   REQUIRE( result == LIBPAPILO_PARAM_NOT_FOUND );

   libpapilo_presolve_free( presolve );
   libpapilo_message_free( message );
}

TEST_CASE( "set_param_int sets integer parameter", "[parameter]" )
{
   libpapilo_message_t* message = libpapilo_message_create();
   libpapilo_presolve_t* presolve = libpapilo_presolve_create( message );
   libpapilo_presolve_add_default_presolvers( presolve );

   // Set message verbosity level
   libpapilo_param_result_t result =
       libpapilo_presolve_set_param_int( presolve, "message.verbosity", 0 );
   REQUIRE( result == LIBPAPILO_PARAM_OK );

   libpapilo_presolve_free( presolve );
   libpapilo_message_free( message );
}

TEST_CASE( "parse_param parses string value", "[parameter]" )
{
   libpapilo_message_t* message = libpapilo_message_create();
   libpapilo_presolve_t* presolve = libpapilo_presolve_create( message );
   libpapilo_presolve_add_default_presolvers( presolve );

   // Parse "0" as false for boolean parameter
   libpapilo_param_result_t result =
       libpapilo_presolve_parse_param( presolve, "parallelcols.enabled", "0" );
   REQUIRE( result == LIBPAPILO_PARAM_OK );

   // Parse "1" as true
   result =
       libpapilo_presolve_parse_param( presolve, "parallelcols.enabled", "1" );
   REQUIRE( result == LIBPAPILO_PARAM_OK );

   libpapilo_presolve_free( presolve );
   libpapilo_message_free( message );
}

TEST_CASE( "parse_param returns NOT_FOUND for parse error", "[parameter]" )
{
   libpapilo_message_t* message = libpapilo_message_create();
   libpapilo_presolve_t* presolve = libpapilo_presolve_create( message );
   libpapilo_presolve_add_default_presolvers( presolve );

   // Invalid value for boolean parameter - boost::lexical_cast throws
   // invalid_argument
   libpapilo_param_result_t result = libpapilo_presolve_parse_param(
       presolve, "parallelcols.enabled", "abc" );
   REQUIRE( result == LIBPAPILO_PARAM_NOT_FOUND );

   libpapilo_presolve_free( presolve );
   libpapilo_message_free( message );
}

TEST_CASE( "set_param_bool returns NOT_FOUND before presolvers are added",
           "[parameter]" )
{
   libpapilo_message_t* message = libpapilo_message_create();
   libpapilo_presolve_t* presolve = libpapilo_presolve_create( message );
   // Note: NOT calling add_default_presolvers

   // This should fail because parallelcols presolver is not added yet
   libpapilo_param_result_t result =
       libpapilo_presolve_set_param_bool( presolve, "parallelcols.enabled", 0 );
   REQUIRE( result == LIBPAPILO_PARAM_NOT_FOUND );

   libpapilo_presolve_free( presolve );
   libpapilo_message_free( message );
}

// Helper function to create a problem with parallel columns
// (same structure as ParallelColDetectionTest.cpp)
static libpapilo_problem_t*
createProblemWithParallelColumns()
{
   libpapilo_problem_builder_t* builder = libpapilo_problem_builder_create();

   libpapilo_problem_builder_set_num_rows( builder, 2 );
   libpapilo_problem_builder_set_num_cols( builder, 2 );
   libpapilo_problem_builder_reserve( builder, 4, 2, 2 );

   // Set objective coefficients (parallel: obj[1] = 3 * obj[0])
   double obj[] = { 1.0, 3.0 };
   libpapilo_problem_builder_set_obj_all( builder, obj );
   libpapilo_problem_builder_set_obj_offset( builder, 0.0 );

   // Set variable bounds
   double lb[] = { 0.0, 0.0 };
   double ub[] = { 10.0, 10.0 };
   libpapilo_problem_builder_set_col_lb_all( builder, lb );
   libpapilo_problem_builder_set_col_ub_all( builder, ub );

   // Set variables as integral
   uint8_t integral[] = { 1, 1 };
   libpapilo_problem_builder_set_col_integral_all( builder, integral );

   // Set row bounds (equality constraints)
   double rhs[] = { 1.0, 2.0 };
   double lhs[] = { 1.0, 2.0 };
   libpapilo_problem_builder_set_row_rhs_all( builder, rhs );
   libpapilo_problem_builder_set_row_lhs_all( builder, lhs );

   // Add matrix entries with parallel structure:
   // Row 0: 1.0 * col0 + 3.0 * col1 = 1
   // Row 1: 2.0 * col0 + 6.0 * col1 = 2
   libpapilo_problem_builder_add_entry( builder, 0, 0, 1.0 );
   libpapilo_problem_builder_add_entry( builder, 0, 1, 3.0 );
   libpapilo_problem_builder_add_entry( builder, 1, 0, 2.0 );
   libpapilo_problem_builder_add_entry( builder, 1, 1, 6.0 );

   libpapilo_problem_builder_set_problem_name( builder, "parallel_cols_test" );

   libpapilo_problem_t* problem = libpapilo_problem_builder_build( builder );
   libpapilo_problem_builder_free( builder );

   return problem;
}

// Helper function to check if postsolve contains PARALLEL_COL reduction
static bool
postsolveContainsParallelCol( libpapilo_postsolve_storage_t* postsolve )
{
   size_t size = 0;
   const libpapilo_postsolve_reduction_type_t* types =
       libpapilo_postsolve_storage_get_types( postsolve, &size );

   if( types == nullptr )
      return false;

   for( size_t i = 0; i < size; ++i )
   {
      if( types[i] == LIBPAPILO_POSTSOLVE_REDUCTION_PARALLEL_COL )
         return true;
   }
   return false;
}

// Helper function to disable all presolvers except the ones we want to test
static void
disableAllPresolversExcept( libpapilo_presolve_t* presolve,
                            const char* keepEnabled )
{
   // Disable all common presolvers
   const char* presolvers[] = {
       "coefftightening.enabled", "colsingleton.enabled",
       "domcol.enabled",          "doubletoneq.enabled",
       "dualfix.enabled",         "dualinfer.enabled",
       "fixcontinuous.enabled",   "implint.enabled",
       "parallelcols.enabled",    "parallelrows.enabled",
       "probing.enabled",         "propagation.enabled",
       "simpleprobing.enabled",   "simplifyineq.enabled",
       "sparsify.enabled",        "stuffing.enabled",
       "substitution.enabled" };

   for( const char* p : presolvers )
   {
      if( strcmp( p, keepEnabled ) != 0 )
      {
         libpapilo_presolve_set_param_bool( presolve, p, 0 );
      }
   }
}

TEST_CASE( "disabling parallelcols prevents parallel column detection",
           "[parameter][behavior]" )
{
   // Test 1: With ONLY parallelcols enabled, parallel columns should be
   // detected
   {
      libpapilo_problem_t* problem = createProblemWithParallelColumns();
      libpapilo_message_t* message = libpapilo_message_create();
      libpapilo_message_set_verbosity_level( message, 0 ); // quiet

      libpapilo_presolve_t* presolve = libpapilo_presolve_create( message );
      libpapilo_presolve_add_default_presolvers( presolve );

      // Disable all presolvers except parallelcols
      disableAllPresolversExcept( presolve, "parallelcols.enabled" );

      // Explicitly enable parallel column detection
      libpapilo_param_result_t result = libpapilo_presolve_set_param_bool(
          presolve, "parallelcols.enabled", 1 );
      REQUIRE( result == LIBPAPILO_PARAM_OK );

      libpapilo_postsolve_storage_t* postsolve = nullptr;
      libpapilo_statistics_t* statistics = nullptr;

      libpapilo_presolve_status_t status = libpapilo_presolve_apply_full(
          presolve, problem, &postsolve, &statistics );
      (void)status; // unused in this test

      // Check that parallel columns were detected
      REQUIRE( postsolve != nullptr );
      bool hasParallelCol = postsolveContainsParallelCol( postsolve );
      INFO( "With parallelcols.enabled=1, PARALLEL_COL should be detected" );
      REQUIRE( hasParallelCol == true );

      // Cleanup
      libpapilo_statistics_free( statistics );
      libpapilo_postsolve_storage_free( postsolve );
      libpapilo_presolve_free( presolve );
      libpapilo_message_free( message );
      libpapilo_problem_free( problem );
   }

   // Test 2: With parallelcols disabled (and all others disabled too),
   // no PARALLEL_COL reduction should occur
   {
      libpapilo_problem_t* problem = createProblemWithParallelColumns();
      libpapilo_message_t* message = libpapilo_message_create();
      libpapilo_message_set_verbosity_level( message, 0 ); // quiet

      libpapilo_presolve_t* presolve = libpapilo_presolve_create( message );
      libpapilo_presolve_add_default_presolvers( presolve );

      // Disable ALL presolvers including parallelcols
      disableAllPresolversExcept( presolve, "" ); // empty = disable all
      libpapilo_param_result_t result = libpapilo_presolve_set_param_bool(
          presolve, "parallelcols.enabled", 0 );
      REQUIRE( result == LIBPAPILO_PARAM_OK );

      libpapilo_postsolve_storage_t* postsolve = nullptr;
      libpapilo_statistics_t* statistics = nullptr;

      libpapilo_presolve_status_t status = libpapilo_presolve_apply_full(
          presolve, problem, &postsolve, &statistics );
      (void)status; // unused in this test

      // Check that parallel columns were NOT detected
      REQUIRE( postsolve != nullptr );
      bool hasParallelCol = postsolveContainsParallelCol( postsolve );
      INFO(
          "With parallelcols.enabled=0, PARALLEL_COL should NOT be detected" );
      REQUIRE( hasParallelCol == false );

      // Cleanup
      libpapilo_statistics_free( statistics );
      libpapilo_postsolve_storage_free( postsolve );
      libpapilo_presolve_free( presolve );
      libpapilo_message_free( message );
      libpapilo_problem_free( problem );
   }
}
