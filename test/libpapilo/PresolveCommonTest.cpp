#include "catch_amalgamated.hpp"
#include "libpapilo.h"
#include <cmath>

TEST_CASE( "presolve-common", "[libpapilo]" )
{
   SECTION( "utility objects creation and destruction" )
   {
      // Test Purpose: Verify all presolve utility objects can be created and
      // freed without errors

      // Test Num object
      libpapilo_num_t* num = libpapilo_num_create();
      REQUIRE( num != nullptr );
      libpapilo_num_free( num );

      // Test Timer object
      double time = 0.0;
      libpapilo_timer_t* timer = libpapilo_timer_create( &time );
      REQUIRE( timer != nullptr );
      libpapilo_timer_free( timer );

      // Test Message object
      libpapilo_message_t* message = libpapilo_message_create();
      REQUIRE( message != nullptr );
      libpapilo_message_free( message );

      // Test Statistics object
      libpapilo_statistics_t* statistics = libpapilo_statistics_create();
      REQUIRE( statistics != nullptr );
      libpapilo_statistics_free( statistics );

      // Test PresolveOptions object
      libpapilo_presolve_options_t* options =
          libpapilo_presolve_options_create();
      REQUIRE( options != nullptr );
      libpapilo_presolve_options_free( options );

      // Test Reductions object
      libpapilo_reductions_t* reductions = libpapilo_reductions_create();
      REQUIRE( reductions != nullptr );
      REQUIRE( libpapilo_reductions_get_size( reductions ) == 0 );
      libpapilo_reductions_free( reductions );

      // Test SingletonCols presolver object
      libpapilo_singleton_cols_t* presolver = libpapilo_singleton_cols_create();
      REQUIRE( presolver != nullptr );
      libpapilo_singleton_cols_free( presolver );
   }

   SECTION( "postsolve storage creation" )
   {
      // Test Purpose: Verify PostsolveStorage can be created with proper
      // dependencies

      // Create a simple problem
      libpapilo_problem_builder_t* builder = libpapilo_problem_builder_create();
      libpapilo_problem_builder_set_num_cols( builder, 2 );
      libpapilo_problem_builder_set_num_rows( builder, 1 );
      libpapilo_problem_builder_set_obj( builder, 0, 1.0 );
      libpapilo_problem_builder_set_obj( builder, 1, 1.0 );
      libpapilo_problem_builder_set_col_lb( builder, 0, 0.0 );
      libpapilo_problem_builder_set_col_lb( builder, 1, 0.0 );
      libpapilo_problem_builder_set_col_ub( builder, 0, 10.0 );
      libpapilo_problem_builder_set_col_ub( builder, 1, 10.0 );
      libpapilo_problem_builder_set_row_lhs( builder, 0, -INFINITY );
      libpapilo_problem_builder_set_row_rhs( builder, 0, 1.0 );
      libpapilo_problem_builder_add_entry( builder, 0, 0, 1.0 );
      libpapilo_problem_builder_add_entry( builder, 0, 1, 1.0 );

      libpapilo_problem_t* problem = libpapilo_problem_builder_build( builder );
      REQUIRE( problem != nullptr );

      // Create dependencies
      libpapilo_num_t* num = libpapilo_num_create();
      libpapilo_presolve_options_t* options =
          libpapilo_presolve_options_create();

      // Test PostsolveStorage creation
      libpapilo_postsolve_storage_t* postsolve =
          libpapilo_postsolve_storage_create( problem, num, options );
      REQUIRE( postsolve != nullptr );

      // Clean up
      libpapilo_postsolve_storage_free( postsolve );
      libpapilo_presolve_options_free( options );
      libpapilo_num_free( num );
      libpapilo_problem_free( problem );
      libpapilo_problem_builder_free( builder );
   }

   SECTION( "problem update creation and operations" )
   {
      // Test Purpose: Verify ProblemUpdate can be created and basic operations
      // work

      // Create a simple problem
      libpapilo_problem_builder_t* builder = libpapilo_problem_builder_create();
      libpapilo_problem_builder_set_num_cols( builder, 2 );
      libpapilo_problem_builder_set_num_rows( builder, 1 );
      libpapilo_problem_builder_set_obj( builder, 0, 1.0 );
      libpapilo_problem_builder_set_obj( builder, 1, 1.0 );
      libpapilo_problem_builder_set_col_lb( builder, 0, 0.0 );
      libpapilo_problem_builder_set_col_lb( builder, 1, 0.0 );
      libpapilo_problem_builder_set_col_ub( builder, 0, 10.0 );
      libpapilo_problem_builder_set_col_ub( builder, 1, 10.0 );
      libpapilo_problem_builder_set_row_lhs( builder, 0, -INFINITY );
      libpapilo_problem_builder_set_row_rhs( builder, 0, 1.0 );
      libpapilo_problem_builder_add_entry( builder, 0, 0, 1.0 );
      libpapilo_problem_builder_add_entry( builder, 0, 1, 1.0 );

      libpapilo_problem_t* problem = libpapilo_problem_builder_build( builder );
      REQUIRE( problem != nullptr );

      // Create all dependencies
      libpapilo_num_t* num = libpapilo_num_create();
      double time = 0.0;
      libpapilo_timer_t* timer = libpapilo_timer_create( &time );
      libpapilo_message_t* message = libpapilo_message_create();
      libpapilo_presolve_options_t* options =
          libpapilo_presolve_options_create();
      libpapilo_statistics_t* statistics = libpapilo_statistics_create();
      libpapilo_postsolve_storage_t* postsolve =
          libpapilo_postsolve_storage_create( problem, num, options );

      // Test ProblemUpdate creation
      libpapilo_problem_update_t* update = libpapilo_problem_update_create(
          problem, postsolve, statistics, options, num, message );
      REQUIRE( update != nullptr );

      // Test trivial column presolve operation
      libpapilo_problem_update_trivial_column_presolve( update );

      // Test getting reductions (should return empty object)
      libpapilo_reductions_t* reductions =
          libpapilo_problem_update_get_reductions( update );
      REQUIRE( reductions != nullptr );
      REQUIRE( libpapilo_reductions_get_size( reductions ) == 0 );

      // Clean up
      libpapilo_reductions_free( reductions );
      libpapilo_problem_update_free( update );
      libpapilo_postsolve_storage_free( postsolve );
      libpapilo_statistics_free( statistics );
      libpapilo_presolve_options_free( options );
      libpapilo_message_free( message );
      libpapilo_timer_free( timer );
      libpapilo_num_free( num );
      libpapilo_problem_free( problem );
      libpapilo_problem_builder_free( builder );
   }

   SECTION( "problem modification api" )
   {
      // Test Purpose: Verify problem modification functions work without errors

      // Create a simple problem
      libpapilo_problem_builder_t* builder = libpapilo_problem_builder_create();
      libpapilo_problem_builder_set_num_cols( builder, 2 );
      libpapilo_problem_builder_set_num_rows( builder, 1 );
      libpapilo_problem_builder_set_obj( builder, 0, 1.0 );
      libpapilo_problem_builder_set_obj( builder, 1, 1.0 );
      libpapilo_problem_builder_set_col_lb( builder, 0, 0.0 );
      libpapilo_problem_builder_set_col_lb( builder, 1, 0.0 );
      libpapilo_problem_builder_set_col_ub( builder, 0, 10.0 );
      libpapilo_problem_builder_set_col_ub( builder, 1, 10.0 );
      libpapilo_problem_builder_set_row_lhs( builder, 0, -INFINITY );
      libpapilo_problem_builder_set_row_rhs( builder, 0, 1.0 );
      libpapilo_problem_builder_add_entry( builder, 0, 0, 1.0 );
      libpapilo_problem_builder_add_entry( builder, 0, 1, 1.0 );

      libpapilo_problem_t* problem = libpapilo_problem_builder_build( builder );
      REQUIRE( problem != nullptr );

      // Test problem modification functions (these should not crash)
      libpapilo_problem_modify_row_lhs( problem, 0, 0.5 );
      libpapilo_problem_recompute_locks( problem );
      libpapilo_problem_recompute_activities( problem );

      // Clean up
      libpapilo_problem_free( problem );
      libpapilo_problem_builder_free( builder );
   }

   SECTION( "singleton cols presolver execution" )
   {
      // Test Purpose: Verify SingletonCols presolver can be executed without
      // errors This is a minimal test - comprehensive tests will be in
      // SingletonColsTest.cpp

      // Create a simple problem with a singleton column
      libpapilo_problem_builder_t* builder = libpapilo_problem_builder_create();
      libpapilo_problem_builder_set_num_cols( builder, 2 );
      libpapilo_problem_builder_set_num_rows( builder, 1 );
      libpapilo_problem_builder_set_obj( builder, 0, 1.0 );
      libpapilo_problem_builder_set_obj( builder, 1, 1.0 );
      libpapilo_problem_builder_set_col_lb( builder, 0, 0.0 );
      libpapilo_problem_builder_set_col_lb( builder, 1, 0.0 );
      libpapilo_problem_builder_set_col_ub( builder, 0, 10.0 );
      libpapilo_problem_builder_set_col_ub( builder, 1, 10.0 );
      libpapilo_problem_builder_set_row_lhs( builder, 0, -INFINITY );
      libpapilo_problem_builder_set_row_rhs( builder, 0, 1.0 );
      // Only add first column (makes it singleton)
      libpapilo_problem_builder_add_entry( builder, 0, 0, 1.0 );

      libpapilo_problem_t* problem = libpapilo_problem_builder_build( builder );
      REQUIRE( problem != nullptr );

      // Create all dependencies
      libpapilo_num_t* num = libpapilo_num_create();
      double time = 0.0;
      libpapilo_timer_t* timer = libpapilo_timer_create( &time );
      libpapilo_message_t* message = libpapilo_message_create();
      libpapilo_presolve_options_t* options =
          libpapilo_presolve_options_create();
      libpapilo_statistics_t* statistics = libpapilo_statistics_create();
      libpapilo_postsolve_storage_t* postsolve =
          libpapilo_postsolve_storage_create( problem, num, options );
      libpapilo_problem_update_t* update = libpapilo_problem_update_create(
          problem, postsolve, statistics, options, num, message );

      // Force calculation of singleton rows (mimic
      // forceCalculationOfSingletonRows)
      libpapilo_problem_recompute_locks( problem );
      libpapilo_problem_update_trivial_column_presolve( update );
      libpapilo_problem_recompute_activities( problem );

      // Create presolver and reductions
      libpapilo_singleton_cols_t* presolver = libpapilo_singleton_cols_create();
      libpapilo_reductions_t* reductions = libpapilo_reductions_create();

      // Execute presolver
      int cause = -1;
      libpapilo_presolve_status_t status = libpapilo_singleton_cols_execute(
          presolver, problem, update, num, reductions, timer, &cause );

      // Should return some status (not testing specific behavior here, just
      // that it doesn't crash)
      REQUIRE( ( status == LIBPAPILO_PRESOLVE_STATUS_UNCHANGED ||
                 status == LIBPAPILO_PRESOLVE_STATUS_REDUCED ||
                 status == LIBPAPILO_PRESOLVE_STATUS_UNBOUNDED ||
                 status == LIBPAPILO_PRESOLVE_STATUS_UNBOUNDED_OR_INFEASIBLE ||
                 status == LIBPAPILO_PRESOLVE_STATUS_INFEASIBLE ) );

      // Reductions size should be non-negative
      int reductions_size = libpapilo_reductions_get_size( reductions );
      REQUIRE( reductions_size >= 0 );

      // If there are reductions, test accessing them
      for( int i = 0; i < reductions_size; ++i )
      {
         libpapilo_reduction_info_t info =
             libpapilo_reductions_get_info( reductions, i );
         // Just verify the structure is populated (values can be anything
         // valid) No specific assertions on values since this is a unit test,
         // not behavior test
         (void)info; // Suppress unused variable warning
      }

      // Clean up
      libpapilo_reductions_free( reductions );
      libpapilo_singleton_cols_free( presolver );
      libpapilo_problem_update_free( update );
      libpapilo_postsolve_storage_free( postsolve );
      libpapilo_statistics_free( statistics );
      libpapilo_presolve_options_free( options );
      libpapilo_message_free( message );
      libpapilo_timer_free( timer );
      libpapilo_num_free( num );
      libpapilo_problem_free( problem );
      libpapilo_problem_builder_free( builder );
   }

   SECTION( "infinity bounds api" )
   {
      // Test Purpose: Verify the infinity bounds APIs work correctly

      libpapilo_problem_builder_t* builder = libpapilo_problem_builder_create();
      libpapilo_problem_builder_set_num_cols( builder, 3 );
      libpapilo_problem_builder_set_num_rows( builder, 2 );

      // Test column infinity bounds
      uint8_t col_lb_inf[] = { 1, 0, 1 }; // cols 0,2 have -inf lower bounds
      uint8_t col_ub_inf[] = { 0, 1, 1 }; // cols 1,2 have +inf upper bounds
      libpapilo_problem_builder_set_col_lb_inf_all( builder, col_lb_inf );
      libpapilo_problem_builder_set_col_ub_inf_all( builder, col_ub_inf );

      // Test row infinity bounds
      uint8_t row_lhs_inf[] = { 1, 0 }; // row 0 has -inf LHS
      uint8_t row_rhs_inf[] = { 0, 1 }; // row 1 has +inf RHS
      libpapilo_problem_builder_set_row_lhs_inf_all( builder, row_lhs_inf );
      libpapilo_problem_builder_set_row_rhs_inf_all( builder, row_rhs_inf );

      // Set some finite bounds to avoid issues
      double col_lbs[] = { -INFINITY, 0.0, -INFINITY };
      double col_ubs[] = { 10.0, INFINITY, INFINITY };
      double row_lhs[] = { -INFINITY, 1.0 };
      double row_rhs[] = { 5.0, INFINITY };

      libpapilo_problem_builder_set_col_lb_all( builder, col_lbs );
      libpapilo_problem_builder_set_col_ub_all( builder, col_ubs );
      libpapilo_problem_builder_set_row_lhs_all( builder, row_lhs );
      libpapilo_problem_builder_set_row_rhs_all( builder, row_rhs );

      // Build and verify it doesn't crash
      libpapilo_problem_t* problem = libpapilo_problem_builder_build( builder );
      REQUIRE( problem != nullptr );

      // Clean up
      libpapilo_problem_free( problem );
      libpapilo_problem_builder_free( builder );
   }

   SECTION( "name arrays api" )
   {
      // Test Purpose: Verify the name array APIs work correctly

      libpapilo_problem_builder_t* builder = libpapilo_problem_builder_create();
      libpapilo_problem_builder_set_num_cols( builder, 3 );
      libpapilo_problem_builder_set_num_rows( builder, 2 );

      // Test column names
      const char* col_names[] = { "x1", "x2", "x3" };
      libpapilo_problem_builder_set_col_name_all( builder, col_names );

      // Test row names
      const char* row_names[] = { "c1", "c2" };
      libpapilo_problem_builder_set_row_name_all( builder, row_names );

      // Set minimal problem data
      double obj[] = { 1.0, 1.0, 1.0 };
      libpapilo_problem_builder_set_obj_all( builder, obj );

      // Build and verify it doesn't crash
      libpapilo_problem_t* problem = libpapilo_problem_builder_build( builder );
      REQUIRE( problem != nullptr );

      // Verify names can be retrieved
      REQUIRE( std::string( libpapilo_problem_get_variable_name(
                   problem, 0 ) ) == "x1" );
      REQUIRE( std::string( libpapilo_problem_get_variable_name(
                   problem, 1 ) ) == "x2" );
      REQUIRE( std::string( libpapilo_problem_get_variable_name(
                   problem, 2 ) ) == "x3" );
      REQUIRE( std::string( libpapilo_problem_get_constraint_name(
                   problem, 0 ) ) == "c1" );
      REQUIRE( std::string( libpapilo_problem_get_constraint_name(
                   problem, 1 ) ) == "c2" );

      // Clean up
      libpapilo_problem_free( problem );
      libpapilo_problem_builder_free( builder );
   }
}
