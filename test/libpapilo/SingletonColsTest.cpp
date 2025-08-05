#include "catch_amalgamated.hpp"
#include "libpapilo.h"
#include <cmath>

// Forward declarations for problem setup functions
libpapilo_problem_t*
setupProblemWithOnlyOneEntryIn1stRowAndColumn();
libpapilo_problem_t*
setupProblemWithSingletonColumn();
libpapilo_problem_t*
setupProblemWithSingletonColumnInEquationWithNoImpliedBounds(
    double coefficient, double upper_bound, double lower_bound );
libpapilo_problem_t*
setupProblemWithSingletonColumnInEquationWithInfinityBounds();

void
forceCalculationOfSingletonRows( libpapilo_problem_t* problem,
                                 libpapilo_problem_update_t* problemUpdate )
{
   libpapilo_problem_recompute_locks( problem );
   // Note: trivialColumnPresolve currently causes segfault, so we skip it
   // libpapilo_problem_update_trivial_column_presolve(problemUpdate);
   libpapilo_problem_recompute_activities( problem );
}

TEST_CASE( "happy-path-singleton-column", "[libpapilo]" )
{
   libpapilo_num_t* num = libpapilo_num_create();
   double time = 0.0;
   int cause = -1;
   libpapilo_timer_t* timer = libpapilo_timer_create( &time );
   libpapilo_message_t* msg = libpapilo_message_create();
   libpapilo_problem_t* problem = setupProblemWithSingletonColumn();
   libpapilo_statistics_t* statistics = libpapilo_statistics_create();
   libpapilo_presolve_options_t* presolveOptions =
       libpapilo_presolve_options_create();
   libpapilo_postsolve_storage_t* postsolve =
       libpapilo_postsolve_storage_create( problem, num, presolveOptions );
   libpapilo_problem_update_t* problemUpdate = libpapilo_problem_update_create(
       problem, postsolve, statistics, presolveOptions, num, msg );
   forceCalculationOfSingletonRows( problem, problemUpdate );
   libpapilo_singleton_cols_t* presolvingMethod =
       libpapilo_singleton_cols_create();
   libpapilo_reductions_t* reductions = libpapilo_reductions_create();

   libpapilo_presolve_status_t presolveStatus =
       libpapilo_singleton_cols_execute( presolvingMethod, problem,
                                         problemUpdate, num, reductions, timer,
                                         &cause );

   REQUIRE( presolveStatus == LIBPAPILO_PRESOLVE_STATUS_REDUCED );
   REQUIRE( libpapilo_reductions_get_size( reductions ) == 5 );

   libpapilo_reduction_info_t reduction0 =
       libpapilo_reductions_get_info( reductions, 0 );
   REQUIRE( reduction0.col == 0 );
   REQUIRE( reduction0.row == LIBPAPILO_COL_REDUCTION_BOUNDS_LOCKED );
   REQUIRE( reduction0.newval == 0 );

   libpapilo_reduction_info_t reduction1 =
       libpapilo_reductions_get_info( reductions, 1 );
   REQUIRE( reduction1.row == 0 );
   REQUIRE( reduction1.col == LIBPAPILO_ROW_REDUCTION_LOCKED );
   REQUIRE( reduction1.newval == 0 );

   libpapilo_reduction_info_t reduction2 =
       libpapilo_reductions_get_info( reductions, 2 );
   REQUIRE( reduction2.col == 0 );
   REQUIRE( reduction2.newval == 0 );
   REQUIRE( reduction2.row == LIBPAPILO_COL_REDUCTION_SUBSTITUTE_OBJ );

   // in matrix entry (0,0) new value 0
   libpapilo_reduction_info_t reduction3 =
       libpapilo_reductions_get_info( reductions, 3 );
   REQUIRE( reduction3.col == 0 );
   REQUIRE( reduction3.newval == 0 );
   REQUIRE( reduction3.row == 0 );

   libpapilo_reduction_info_t reduction4 =
       libpapilo_reductions_get_info( reductions, 4 );
   REQUIRE( reduction4.col == LIBPAPILO_ROW_REDUCTION_LHS_INF );
   REQUIRE( reduction4.newval == 0 );
   REQUIRE( reduction4.row == 0 );

   // Clean up
   libpapilo_reductions_free( reductions );
   libpapilo_singleton_cols_free( presolvingMethod );
   libpapilo_problem_update_free( problemUpdate );
   libpapilo_postsolve_storage_free( postsolve );
   libpapilo_presolve_options_free( presolveOptions );
   libpapilo_statistics_free( statistics );
   libpapilo_problem_free( problem );
   libpapilo_message_free( msg );
   libpapilo_timer_free( timer );
   libpapilo_num_free( num );
}

TEST_CASE( "happy-path-singleton-column-equation", "[libpapilo]" )
{
   libpapilo_num_t* num = libpapilo_num_create();
   double time = 0.0;
   int cause = -1;
   libpapilo_timer_t* timer = libpapilo_timer_create( &time );
   libpapilo_message_t* msg = libpapilo_message_create();
   libpapilo_problem_t* problem =
       setupProblemWithOnlyOneEntryIn1stRowAndColumn();
   libpapilo_statistics_t* statistics = libpapilo_statistics_create();
   libpapilo_presolve_options_t* presolveOptions =
       libpapilo_presolve_options_create();
   libpapilo_postsolve_storage_t* postsolve =
       libpapilo_postsolve_storage_create( problem, num, presolveOptions );
   libpapilo_problem_update_t* problemUpdate = libpapilo_problem_update_create(
       problem, postsolve, statistics, presolveOptions, num, msg );
   forceCalculationOfSingletonRows( problem, problemUpdate );
   libpapilo_singleton_cols_t* presolvingMethod =
       libpapilo_singleton_cols_create();
   libpapilo_reductions_t* reductions = libpapilo_reductions_create();

   libpapilo_presolve_status_t presolveStatus =
       libpapilo_singleton_cols_execute( presolvingMethod, problem,
                                         problemUpdate, num, reductions, timer,
                                         &cause );

   REQUIRE( presolveStatus == LIBPAPILO_PRESOLVE_STATUS_REDUCED );
   REQUIRE( libpapilo_reductions_get_size( reductions ) == 8 );

   libpapilo_reduction_info_t reduction0 =
       libpapilo_reductions_get_info( reductions, 0 );
   REQUIRE( reduction0.col == 2 );
   REQUIRE( reduction0.row == LIBPAPILO_COL_REDUCTION_BOUNDS_LOCKED );
   REQUIRE( reduction0.newval == 0 );

   libpapilo_reduction_info_t reduction1 =
       libpapilo_reductions_get_info( reductions, 1 );
   REQUIRE( reduction1.col == LIBPAPILO_ROW_REDUCTION_LOCKED );
   REQUIRE( reduction1.row == 1 );
   REQUIRE( reduction1.newval == 0 );

   libpapilo_reduction_info_t reduction2 =
       libpapilo_reductions_get_info( reductions, 2 );
   REQUIRE( reduction2.col == 2 );
   REQUIRE( reduction2.row == LIBPAPILO_COL_REDUCTION_SUBSTITUTE_OBJ );
   REQUIRE( reduction2.newval == 1 );

   libpapilo_reduction_info_t reduction3 =
       libpapilo_reductions_get_info( reductions, 3 );
   REQUIRE( reduction3.col == 2 );
   REQUIRE( reduction3.row == 1 );
   REQUIRE( reduction3.newval == 0 );

   libpapilo_reduction_info_t reduction4 =
       libpapilo_reductions_get_info( reductions, 4 );
   REQUIRE( reduction4.col == LIBPAPILO_ROW_REDUCTION_LHS_INF );
   REQUIRE( reduction4.row == 1 );
   REQUIRE( reduction4.newval == 0 );

   libpapilo_reduction_info_t reduction5 =
       libpapilo_reductions_get_info( reductions, 5 );
   REQUIRE( reduction5.col == LIBPAPILO_ROW_REDUCTION_RHS );
   REQUIRE( reduction5.row == 1 );
   REQUIRE( reduction5.newval == 2.5 );

   libpapilo_reduction_info_t reduction6 =
       libpapilo_reductions_get_info( reductions, 6 );
   REQUIRE( reduction6.col == 0 );
   REQUIRE( reduction6.row == 1 );
   REQUIRE( reduction6.newval == 0.75 );

   libpapilo_reduction_info_t reduction7 =
       libpapilo_reductions_get_info( reductions, 7 );
   REQUIRE( reduction7.col == 1 );
   REQUIRE( reduction7.row == 1 );
   REQUIRE( reduction7.newval == 0.75 );

   // Clean up
   libpapilo_reductions_free( reductions );
   libpapilo_singleton_cols_free( presolvingMethod );
   libpapilo_problem_update_free( problemUpdate );
   libpapilo_postsolve_storage_free( postsolve );
   libpapilo_presolve_options_free( presolveOptions );
   libpapilo_statistics_free( statistics );
   libpapilo_problem_free( problem );
   libpapilo_message_free( msg );
   libpapilo_timer_free( timer );
   libpapilo_num_free( num );
}

TEST_CASE(
    "happy-path-singleton-column-implied-bounds-negative-coeff-pos-bounds",
    "[libpapilo]" )
{
   libpapilo_message_t* msg = libpapilo_message_create();
   double time = 0.0;
   int cause = -1;
   libpapilo_timer_t* timer = libpapilo_timer_create( &time );
   libpapilo_num_t* num = libpapilo_num_create();
   libpapilo_problem_t* problem =
       setupProblemWithSingletonColumnInEquationWithNoImpliedBounds( -1.0, 10.0,
                                                                     3.0 );
   libpapilo_statistics_t* statistics = libpapilo_statistics_create();
   libpapilo_presolve_options_t* presolveOptions =
       libpapilo_presolve_options_create();
   libpapilo_postsolve_storage_t* postsolve =
       libpapilo_postsolve_storage_create( problem, num, presolveOptions );
   libpapilo_problem_update_t* problemUpdate = libpapilo_problem_update_create(
       problem, postsolve, statistics, presolveOptions, num, msg );
   forceCalculationOfSingletonRows( problem, problemUpdate );
   libpapilo_singleton_cols_t* presolvingMethod =
       libpapilo_singleton_cols_create();
   libpapilo_reductions_t* reductions = libpapilo_reductions_create();

   libpapilo_presolve_status_t presolveStatus =
       libpapilo_singleton_cols_execute( presolvingMethod, problem,
                                         problemUpdate, num, reductions, timer,
                                         &cause );

   REQUIRE( presolveStatus == LIBPAPILO_PRESOLVE_STATUS_REDUCED );
   REQUIRE( libpapilo_reductions_get_size( reductions ) == 6 );

   libpapilo_reduction_info_t reduction0 =
       libpapilo_reductions_get_info( reductions, 0 );
   REQUIRE( reduction0.col == 0 );
   REQUIRE( reduction0.row == LIBPAPILO_COL_REDUCTION_BOUNDS_LOCKED );
   REQUIRE( reduction0.newval == 0 );

   libpapilo_reduction_info_t reduction1 =
       libpapilo_reductions_get_info( reductions, 1 );
   REQUIRE( reduction1.col == LIBPAPILO_ROW_REDUCTION_LOCKED );
   REQUIRE( reduction1.row == 0 );
   REQUIRE( reduction1.newval == 0 );

   libpapilo_reduction_info_t reduction2 =
       libpapilo_reductions_get_info( reductions, 2 );
   REQUIRE( reduction2.col == 0 );
   REQUIRE( reduction2.row == LIBPAPILO_COL_REDUCTION_SUBSTITUTE_OBJ );
   REQUIRE( reduction2.newval == 0 );

   libpapilo_reduction_info_t reduction3 =
       libpapilo_reductions_get_info( reductions, 3 );
   REQUIRE( reduction3.col == 0 );
   REQUIRE( reduction3.row == 0 );
   REQUIRE( reduction3.newval == 0 );

   libpapilo_reduction_info_t reduction4 =
       libpapilo_reductions_get_info( reductions, 4 );
   REQUIRE( reduction4.col == LIBPAPILO_ROW_REDUCTION_RHS );
   REQUIRE( reduction4.row == 0 );
   REQUIRE( reduction4.newval == 11 );

   libpapilo_reduction_info_t reduction5 =
       libpapilo_reductions_get_info( reductions, 5 );
   REQUIRE( reduction5.col == LIBPAPILO_ROW_REDUCTION_LHS );
   REQUIRE( reduction5.row == 0 );
   REQUIRE( reduction5.newval == 4 );

   // Clean up
   libpapilo_reductions_free( reductions );
   libpapilo_singleton_cols_free( presolvingMethod );
   libpapilo_problem_update_free( problemUpdate );
   libpapilo_postsolve_storage_free( postsolve );
   libpapilo_presolve_options_free( presolveOptions );
   libpapilo_statistics_free( statistics );
   libpapilo_problem_free( problem );
   libpapilo_message_free( msg );
   libpapilo_timer_free( timer );
   libpapilo_num_free( num );
}

TEST_CASE(
    "happy-path-singleton-column-implied-bounds-negative-coeff-neg-bounds",
    "[libpapilo]" )
{
   libpapilo_message_t* msg = libpapilo_message_create();
   double time = 0.0;
   int cause = -1;
   libpapilo_timer_t* timer = libpapilo_timer_create( &time );
   libpapilo_num_t* num = libpapilo_num_create();
   libpapilo_problem_t* problem =
       setupProblemWithSingletonColumnInEquationWithNoImpliedBounds( -1.0, -3.0,
                                                                     -10.0 );
   libpapilo_statistics_t* statistics = libpapilo_statistics_create();
   libpapilo_presolve_options_t* presolveOptions =
       libpapilo_presolve_options_create();
   libpapilo_postsolve_storage_t* postsolve =
       libpapilo_postsolve_storage_create( problem, num, presolveOptions );
   libpapilo_problem_update_t* problemUpdate = libpapilo_problem_update_create(
       problem, postsolve, statistics, presolveOptions, num, msg );
   forceCalculationOfSingletonRows( problem, problemUpdate );
   libpapilo_singleton_cols_t* presolvingMethod =
       libpapilo_singleton_cols_create();
   libpapilo_reductions_t* reductions = libpapilo_reductions_create();

   libpapilo_presolve_status_t presolveStatus =
       libpapilo_singleton_cols_execute( presolvingMethod, problem,
                                         problemUpdate, num, reductions, timer,
                                         &cause );

   REQUIRE( presolveStatus == LIBPAPILO_PRESOLVE_STATUS_REDUCED );
   REQUIRE( libpapilo_reductions_get_size( reductions ) == 6 );

   libpapilo_reduction_info_t reduction0 =
       libpapilo_reductions_get_info( reductions, 0 );
   REQUIRE( reduction0.col == 0 );
   REQUIRE( reduction0.row == LIBPAPILO_COL_REDUCTION_BOUNDS_LOCKED );
   REQUIRE( reduction0.newval == 0 );

   libpapilo_reduction_info_t reduction1 =
       libpapilo_reductions_get_info( reductions, 1 );
   REQUIRE( reduction1.col == LIBPAPILO_ROW_REDUCTION_LOCKED );
   REQUIRE( reduction1.row == 0 );
   REQUIRE( reduction1.newval == 0 );

   libpapilo_reduction_info_t reduction2 =
       libpapilo_reductions_get_info( reductions, 2 );
   REQUIRE( reduction2.col == 0 );
   REQUIRE( reduction2.row == LIBPAPILO_COL_REDUCTION_SUBSTITUTE_OBJ );
   REQUIRE( reduction2.newval == 0 );

   libpapilo_reduction_info_t reduction3 =
       libpapilo_reductions_get_info( reductions, 3 );
   REQUIRE( reduction3.col == 0 );
   REQUIRE( reduction3.row == 0 );
   REQUIRE( reduction3.newval == 0 );

   libpapilo_reduction_info_t reduction4 =
       libpapilo_reductions_get_info( reductions, 4 );
   REQUIRE( reduction4.col == LIBPAPILO_ROW_REDUCTION_LHS );
   REQUIRE( reduction4.row == 0 );
   REQUIRE( reduction4.newval == -9 );

   libpapilo_reduction_info_t reduction5 =
       libpapilo_reductions_get_info( reductions, 5 );
   REQUIRE( reduction5.col == LIBPAPILO_ROW_REDUCTION_RHS );
   REQUIRE( reduction5.row == 0 );
   REQUIRE( reduction5.newval == -2 );

   // Clean up
   libpapilo_reductions_free( reductions );
   libpapilo_singleton_cols_free( presolvingMethod );
   libpapilo_problem_update_free( problemUpdate );
   libpapilo_postsolve_storage_free( postsolve );
   libpapilo_presolve_options_free( presolveOptions );
   libpapilo_statistics_free( statistics );
   libpapilo_problem_free( problem );
   libpapilo_message_free( msg );
   libpapilo_timer_free( timer );
   libpapilo_num_free( num );
}

TEST_CASE(
    "happy-path-singleton-column-implied-bounds-positive-coeff-pos-bounds",
    "[libpapilo]" )
{
   libpapilo_message_t* msg = libpapilo_message_create();
   double time = 0.0;
   int cause = -1;
   libpapilo_timer_t* timer = libpapilo_timer_create( &time );
   libpapilo_num_t* num = libpapilo_num_create();
   libpapilo_problem_t* problem =
       setupProblemWithSingletonColumnInEquationWithNoImpliedBounds( 1.0, 10.0,
                                                                     3.0 );
   libpapilo_statistics_t* statistics = libpapilo_statistics_create();
   libpapilo_presolve_options_t* presolveOptions =
       libpapilo_presolve_options_create();
   libpapilo_postsolve_storage_t* postsolve =
       libpapilo_postsolve_storage_create( problem, num, presolveOptions );
   libpapilo_problem_update_t* problemUpdate = libpapilo_problem_update_create(
       problem, postsolve, statistics, presolveOptions, num, msg );
   forceCalculationOfSingletonRows( problem, problemUpdate );
   libpapilo_singleton_cols_t* presolvingMethod =
       libpapilo_singleton_cols_create();
   libpapilo_reductions_t* reductions = libpapilo_reductions_create();

   libpapilo_presolve_status_t presolveStatus =
       libpapilo_singleton_cols_execute( presolvingMethod, problem,
                                         problemUpdate, num, reductions, timer,
                                         &cause );

   REQUIRE( presolveStatus == LIBPAPILO_PRESOLVE_STATUS_REDUCED );
   REQUIRE( libpapilo_reductions_get_size( reductions ) == 6 );

   libpapilo_reduction_info_t reduction0 =
       libpapilo_reductions_get_info( reductions, 0 );
   REQUIRE( reduction0.col == 0 );
   REQUIRE( reduction0.row == LIBPAPILO_COL_REDUCTION_BOUNDS_LOCKED );
   REQUIRE( reduction0.newval == 0 );

   libpapilo_reduction_info_t reduction1 =
       libpapilo_reductions_get_info( reductions, 1 );
   REQUIRE( reduction1.col == LIBPAPILO_ROW_REDUCTION_LOCKED );
   REQUIRE( reduction1.row == 0 );
   REQUIRE( reduction1.newval == 0 );

   libpapilo_reduction_info_t reduction2 =
       libpapilo_reductions_get_info( reductions, 2 );
   REQUIRE( reduction2.col == 0 );
   REQUIRE( reduction2.row == LIBPAPILO_COL_REDUCTION_SUBSTITUTE_OBJ );
   REQUIRE( reduction2.newval == 0 );

   libpapilo_reduction_info_t reduction3 =
       libpapilo_reductions_get_info( reductions, 3 );
   REQUIRE( reduction3.col == 0 );
   REQUIRE( reduction3.row == 0 );
   REQUIRE( reduction3.newval == 0 );

   libpapilo_reduction_info_t reduction4 =
       libpapilo_reductions_get_info( reductions, 4 );
   REQUIRE( reduction4.col == LIBPAPILO_ROW_REDUCTION_LHS );
   REQUIRE( reduction4.row == 0 );
   REQUIRE( reduction4.newval == -9 );

   libpapilo_reduction_info_t reduction5 =
       libpapilo_reductions_get_info( reductions, 5 );
   REQUIRE( reduction5.col == LIBPAPILO_ROW_REDUCTION_RHS );
   REQUIRE( reduction5.row == 0 );
   REQUIRE( reduction5.newval == -2 );

   // Clean up
   libpapilo_reductions_free( reductions );
   libpapilo_singleton_cols_free( presolvingMethod );
   libpapilo_problem_update_free( problemUpdate );
   libpapilo_postsolve_storage_free( postsolve );
   libpapilo_presolve_options_free( presolveOptions );
   libpapilo_statistics_free( statistics );
   libpapilo_problem_free( problem );
   libpapilo_message_free( msg );
   libpapilo_timer_free( timer );
   libpapilo_num_free( num );
}

TEST_CASE(
    "happy-path-singleton-column-implied-bounds-positive-coeff-neg-bounds",
    "[libpapilo]" )
{
   libpapilo_message_t* msg = libpapilo_message_create();
   double time = 0.0;
   int cause = -1;
   libpapilo_timer_t* timer = libpapilo_timer_create( &time );
   libpapilo_num_t* num = libpapilo_num_create();
   libpapilo_problem_t* problem =
       setupProblemWithSingletonColumnInEquationWithNoImpliedBounds( 1.0, -3.0,
                                                                     -10.0 );
   libpapilo_statistics_t* statistics = libpapilo_statistics_create();
   libpapilo_presolve_options_t* presolveOptions =
       libpapilo_presolve_options_create();
   libpapilo_postsolve_storage_t* postsolve =
       libpapilo_postsolve_storage_create( problem, num, presolveOptions );
   libpapilo_problem_update_t* problemUpdate = libpapilo_problem_update_create(
       problem, postsolve, statistics, presolveOptions, num, msg );
   forceCalculationOfSingletonRows( problem, problemUpdate );
   libpapilo_singleton_cols_t* presolvingMethod =
       libpapilo_singleton_cols_create();
   libpapilo_reductions_t* reductions = libpapilo_reductions_create();

   libpapilo_presolve_status_t presolveStatus =
       libpapilo_singleton_cols_execute( presolvingMethod, problem,
                                         problemUpdate, num, reductions, timer,
                                         &cause );

   REQUIRE( presolveStatus == LIBPAPILO_PRESOLVE_STATUS_REDUCED );
   REQUIRE( libpapilo_reductions_get_size( reductions ) == 6 );

   libpapilo_reduction_info_t reduction0 =
       libpapilo_reductions_get_info( reductions, 0 );
   REQUIRE( reduction0.col == 0 );
   REQUIRE( reduction0.row == LIBPAPILO_COL_REDUCTION_BOUNDS_LOCKED );
   REQUIRE( reduction0.newval == 0 );

   libpapilo_reduction_info_t reduction1 =
       libpapilo_reductions_get_info( reductions, 1 );
   REQUIRE( reduction1.col == LIBPAPILO_ROW_REDUCTION_LOCKED );
   REQUIRE( reduction1.row == 0 );
   REQUIRE( reduction1.newval == 0 );

   libpapilo_reduction_info_t reduction2 =
       libpapilo_reductions_get_info( reductions, 2 );
   REQUIRE( reduction2.col == 0 );
   REQUIRE( reduction2.row == LIBPAPILO_COL_REDUCTION_SUBSTITUTE_OBJ );
   REQUIRE( reduction2.newval == 0 );

   libpapilo_reduction_info_t reduction3 =
       libpapilo_reductions_get_info( reductions, 3 );
   REQUIRE( reduction3.col == 0 );
   REQUIRE( reduction3.row == 0 );
   REQUIRE( reduction3.newval == 0 );

   libpapilo_reduction_info_t reduction4 =
       libpapilo_reductions_get_info( reductions, 4 );
   REQUIRE( reduction4.col == LIBPAPILO_ROW_REDUCTION_RHS );
   REQUIRE( reduction4.row == 0 );
   REQUIRE( reduction4.newval == 11 );

   libpapilo_reduction_info_t reduction5 =
       libpapilo_reductions_get_info( reductions, 5 );
   REQUIRE( reduction5.col == LIBPAPILO_ROW_REDUCTION_LHS );
   REQUIRE( reduction5.row == 0 );
   REQUIRE( reduction5.newval == 4 );

   // Clean up
   libpapilo_reductions_free( reductions );
   libpapilo_singleton_cols_free( presolvingMethod );
   libpapilo_problem_update_free( problemUpdate );
   libpapilo_postsolve_storage_free( postsolve );
   libpapilo_presolve_options_free( presolveOptions );
   libpapilo_statistics_free( statistics );
   libpapilo_problem_free( problem );
   libpapilo_message_free( msg );
   libpapilo_timer_free( timer );
   libpapilo_num_free( num );
}

TEST_CASE( "happy-path-singleton-column-infinity-bounds-equation",
           "[libpapilo]" )
{
   libpapilo_message_t* msg = libpapilo_message_create();
   double time = 0.0;
   int cause = -1;
   libpapilo_timer_t* timer = libpapilo_timer_create( &time );
   libpapilo_num_t* num = libpapilo_num_create();
   libpapilo_problem_t* problem =
       setupProblemWithSingletonColumnInEquationWithInfinityBounds();
   libpapilo_statistics_t* statistics = libpapilo_statistics_create();
   libpapilo_presolve_options_t* presolveOptions =
       libpapilo_presolve_options_create();
   libpapilo_postsolve_storage_t* postsolve =
       libpapilo_postsolve_storage_create( problem, num, presolveOptions );
   libpapilo_problem_update_t* problemUpdate = libpapilo_problem_update_create(
       problem, postsolve, statistics, presolveOptions, num, msg );
   forceCalculationOfSingletonRows( problem, problemUpdate );
   libpapilo_singleton_cols_t* presolvingMethod =
       libpapilo_singleton_cols_create();
   libpapilo_reductions_t* reductions = libpapilo_reductions_create();

   libpapilo_presolve_status_t presolveStatus =
       libpapilo_singleton_cols_execute( presolvingMethod, problem,
                                         problemUpdate, num, reductions, timer,
                                         &cause );

   REQUIRE( presolveStatus == LIBPAPILO_PRESOLVE_STATUS_REDUCED );
   REQUIRE( libpapilo_reductions_get_size( reductions ) == 5 );

   libpapilo_reduction_info_t reduction0 =
       libpapilo_reductions_get_info( reductions, 0 );
   REQUIRE( reduction0.col == 0 );
   REQUIRE( reduction0.row == LIBPAPILO_COL_REDUCTION_BOUNDS_LOCKED );
   REQUIRE( reduction0.newval == 0 );

   libpapilo_reduction_info_t reduction1 =
       libpapilo_reductions_get_info( reductions, 1 );
   REQUIRE( reduction1.col == LIBPAPILO_ROW_REDUCTION_LOCKED );
   REQUIRE( reduction1.row == 0 );
   REQUIRE( reduction1.newval == 0 );

   libpapilo_reduction_info_t reduction2 =
       libpapilo_reductions_get_info( reductions, 2 );
   REQUIRE( reduction2.col == 0 );
   REQUIRE( reduction2.row == LIBPAPILO_COL_REDUCTION_SUBSTITUTE_OBJ );
   REQUIRE( reduction2.newval == 0 );

   libpapilo_reduction_info_t reduction3 =
       libpapilo_reductions_get_info( reductions, 3 );
   REQUIRE( reduction3.col == 0 );
   REQUIRE( reduction3.row == 0 );
   REQUIRE( reduction3.newval == 0 );

   libpapilo_reduction_info_t reduction4 =
       libpapilo_reductions_get_info( reductions, 4 );
   REQUIRE( reduction4.col == LIBPAPILO_ROW_REDUCTION_LHS_INF );
   REQUIRE( reduction4.row == 0 );
   REQUIRE( reduction4.newval == 0 );

   // Clean up
   libpapilo_reductions_free( reductions );
   libpapilo_singleton_cols_free( presolvingMethod );
   libpapilo_problem_update_free( problemUpdate );
   libpapilo_postsolve_storage_free( postsolve );
   libpapilo_presolve_options_free( presolveOptions );
   libpapilo_statistics_free( statistics );
   libpapilo_problem_free( problem );
   libpapilo_message_free( msg );
   libpapilo_timer_free( timer );
   libpapilo_num_free( num );
}

// Problem setup functions implementation

libpapilo_problem_t*
setupProblemWithOnlyOneEntryIn1stRowAndColumn()
{
   // Equivalent to the C++ version but using C API
   double coefficients[] = { 1.0, 1.0, 1.0 };
   double upperBounds[] = { 10.0, 10.0, 10.0 };
   double lowerBounds[] = { 0.0, 0.0, 0.0 };
   uint8_t isIntegral[] = { 0, 0, 0 };

   uint8_t isLefthandsideInfinity[] = { 1, 1 };
   uint8_t isRighthandsideInfinity[] = { 0, 0 };
   double rhs[] = { 3.0, 10.0 };
   // Only used for array size in original C++ test
   // const char* rowNames[] = { "A1", "A2" };
   const char* columnNames[] = { "x", "y", "z" };

   libpapilo_problem_builder_t* pb = libpapilo_problem_builder_create();
   libpapilo_problem_builder_reserve( pb, 5, 2, 3 );
   libpapilo_problem_builder_set_num_rows( pb, 2 );
   libpapilo_problem_builder_set_num_cols( pb, 3 );
   libpapilo_problem_builder_set_col_ub_all( pb, upperBounds );
   libpapilo_problem_builder_set_col_lb_all( pb, lowerBounds );
   libpapilo_problem_builder_set_obj_all( pb, coefficients );
   libpapilo_problem_builder_set_obj_offset( pb, 0.0 );
   libpapilo_problem_builder_set_col_integral_all( pb, isIntegral );
   libpapilo_problem_builder_set_row_rhs_all( pb, rhs );
   libpapilo_problem_builder_set_row_lhs_inf_all( pb, isLefthandsideInfinity );
   libpapilo_problem_builder_set_row_rhs_inf_all( pb, isRighthandsideInfinity );

   // Add entries
   libpapilo_problem_builder_add_entry( pb, 0, 0, 1.0 );
   libpapilo_problem_builder_add_entry( pb, 0, 1, 2.0 );
   libpapilo_problem_builder_add_entry( pb, 1, 0, 3.0 );
   libpapilo_problem_builder_add_entry( pb, 1, 1, 3.0 );
   libpapilo_problem_builder_add_entry( pb, 1, 2, 4.0 );

   libpapilo_problem_builder_set_col_name_all( pb, columnNames );
   libpapilo_problem_builder_set_problem_name(
       pb, "singleton column & row matrix with equation" );

   libpapilo_problem_t* problem = libpapilo_problem_builder_build( pb );
   libpapilo_problem_modify_row_lhs( problem, 1, rhs[1] );

   libpapilo_problem_builder_free( pb );
   return problem;
}

libpapilo_problem_t*
setupProblemWithSingletonColumn()
{
   double coefficients[] = { 1.0, 1.0, 1.0 };
   double upperBounds[] = { 10.0, 10.0, 10.0 };
   double lowerBounds[] = { 0.0, 0.0, 0.0 };
   uint8_t isIntegral[] = { 1, 1, 1 };

   uint8_t isLefthandsideInfinity[] = { 0, 1, 1 };
   uint8_t isRighthandsideInfinity[] = { 0, 0, 0 };
   double rhs[] = { 1.0, 2.0, 3.0 };
   // Only used for array size in original C++ test
   // const char* rowNames[] = { "A1", "A2", "A3" };
   const char* columnNames[] = { "c1", "c2", "c3" };

   libpapilo_problem_builder_t* pb = libpapilo_problem_builder_create();
   libpapilo_problem_builder_reserve( pb, 6, 3, 3 );
   libpapilo_problem_builder_set_num_rows( pb, 3 );
   libpapilo_problem_builder_set_num_cols( pb, 3 );
   libpapilo_problem_builder_set_col_ub_all( pb, upperBounds );
   libpapilo_problem_builder_set_col_lb_all( pb, lowerBounds );
   libpapilo_problem_builder_set_obj_all( pb, coefficients );
   libpapilo_problem_builder_set_obj_offset( pb, 0.0 );
   libpapilo_problem_builder_set_col_integral_all( pb, isIntegral );
   libpapilo_problem_builder_set_row_rhs_all( pb, rhs );
   libpapilo_problem_builder_set_row_lhs_inf_all( pb, isLefthandsideInfinity );
   libpapilo_problem_builder_set_row_rhs_inf_all( pb, isRighthandsideInfinity );

   // Add entries
   libpapilo_problem_builder_add_entry( pb, 0, 0, 1.0 );
   libpapilo_problem_builder_add_entry( pb, 0, 1, 1.0 );
   libpapilo_problem_builder_add_entry( pb, 1, 1, 2.0 );
   libpapilo_problem_builder_add_entry( pb, 2, 2, 3.0 );
   libpapilo_problem_builder_add_entry( pb, 1, 2, 3.0 );
   libpapilo_problem_builder_add_entry( pb, 2, 1, 4.0 );

   libpapilo_problem_builder_set_col_name_all( pb, columnNames );
   libpapilo_problem_builder_set_problem_name( pb, "singleton column" );

   libpapilo_problem_t* problem = libpapilo_problem_builder_build( pb );
   libpapilo_problem_modify_row_lhs( problem, 0, 1 );

   libpapilo_problem_builder_free( pb );
   return problem;
}

libpapilo_problem_t*
setupProblemWithSingletonColumnInEquationWithNoImpliedBounds(
    double coefficient, double upper_bound, double lower_bound )
{
   double coefficients[] = { 0.0, 1.0, 1.0 };
   double upperBounds[] = { upper_bound, 10.0, 10.0 };
   double lowerBounds[] = { lower_bound, -10.0, -10.0 };
   uint8_t isIntegral[] = { 0, 0, 0 };

   uint8_t isLefthandsideInfinity[] = { 0, 1, 1 };
   uint8_t isRighthandsideInfinity[] = { 0, 0, 0 };
   double rhs[] = { 1.0, 2.0, 3.0 };
   // Only used for array size in original C++ test
   // const char* rowNames[] = { "A1", "A2", "A3" };
   const char* columnNames[] = { "c1", "c2", "c3" };

   libpapilo_problem_builder_t* pb = libpapilo_problem_builder_create();
   libpapilo_problem_builder_reserve( pb, 5, 3, 3 );
   libpapilo_problem_builder_set_num_rows( pb, 3 );
   libpapilo_problem_builder_set_num_cols( pb, 3 );
   libpapilo_problem_builder_set_col_ub_all( pb, upperBounds );
   libpapilo_problem_builder_set_col_lb_all( pb, lowerBounds );
   libpapilo_problem_builder_set_obj_all( pb, coefficients );
   libpapilo_problem_builder_set_obj_offset( pb, 0.0 );
   libpapilo_problem_builder_set_col_integral_all( pb, isIntegral );
   libpapilo_problem_builder_set_row_rhs_all( pb, rhs );
   libpapilo_problem_builder_set_row_lhs_inf_all( pb, isLefthandsideInfinity );
   libpapilo_problem_builder_set_row_rhs_inf_all( pb, isRighthandsideInfinity );

   // Add entries
   libpapilo_problem_builder_add_entry( pb, 0, 0, coefficient );
   libpapilo_problem_builder_add_entry( pb, 0, 1, 1.0 );
   libpapilo_problem_builder_add_entry( pb, 0, 2, 3.0 );
   libpapilo_problem_builder_add_entry( pb, 1, 1, 2.0 );
   libpapilo_problem_builder_add_entry( pb, 1, 2, 3.0 );

   libpapilo_problem_builder_set_col_name_all( pb, columnNames );
   libpapilo_problem_builder_set_problem_name( pb, "singleton column" );

   libpapilo_problem_t* problem = libpapilo_problem_builder_build( pb );
   libpapilo_problem_modify_row_lhs( problem, 0, 1 );

   libpapilo_problem_builder_free( pb );
   return problem;
}

libpapilo_problem_t*
setupProblemWithSingletonColumnInEquationWithInfinityBounds()
{
   double coefficients[] = { 0.0, 1.0, 1.0 };
   uint8_t isIntegral[] = { 0, 0, 0 };
   uint8_t upper_bound_infinity[] = { 1, 1, 1 };
   uint8_t lower_bound_infinity[] = { 0, 0, 0 };

   double rhs[] = { 1.0, 2.0, 3.0 };
   // Only used for array size in original C++ test
   // const char* rowNames[] = { "A1", "A2", "A3" };
   const char* columnNames[] = { "c1", "c2", "c3" };

   libpapilo_problem_builder_t* pb = libpapilo_problem_builder_create();
   libpapilo_problem_builder_reserve( pb, 7, 3, 3 );
   libpapilo_problem_builder_set_num_rows( pb, 3 );
   libpapilo_problem_builder_set_num_cols( pb, 3 );
   libpapilo_problem_builder_set_col_ub_inf_all( pb, upper_bound_infinity );
   libpapilo_problem_builder_set_col_lb_inf_all( pb, lower_bound_infinity );
   libpapilo_problem_builder_set_obj_all( pb, coefficients );
   libpapilo_problem_builder_set_obj_offset( pb, 0.0 );
   libpapilo_problem_builder_set_col_integral_all( pb, isIntegral );
   libpapilo_problem_builder_set_row_rhs_all( pb, rhs );

   // Add entries
   libpapilo_problem_builder_add_entry( pb, 0, 0, 1.0 );
   libpapilo_problem_builder_add_entry( pb, 0, 1, 1.0 );
   libpapilo_problem_builder_add_entry( pb, 0, 2, 1.0 );
   libpapilo_problem_builder_add_entry( pb, 1, 1, 2.0 );
   libpapilo_problem_builder_add_entry( pb, 1, 2, 3.0 );
   libpapilo_problem_builder_add_entry( pb, 2, 1, -4.0 );
   libpapilo_problem_builder_add_entry( pb, 2, 2, -5.0 );

   libpapilo_problem_builder_set_col_name_all( pb, columnNames );
   libpapilo_problem_builder_set_problem_name( pb, "singleton column" );

   libpapilo_problem_t* problem = libpapilo_problem_builder_build( pb );
   libpapilo_problem_modify_row_lhs( problem, 0, rhs[0] );

   libpapilo_problem_builder_free( pb );
   return problem;
}
