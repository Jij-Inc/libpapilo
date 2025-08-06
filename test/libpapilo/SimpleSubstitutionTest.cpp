/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                           */
/*               This file is part of the program and library                */
/*    PaPILO --- Parallel Presolve for Integer and Linear Optimization       */
/*                                                                           */
/* Copyright (C) 2020-2025 Zuse Institute Berlin (ZIB)                       */
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

// Forward declarations for problem setup functions
libpapilo_problem_t* setupProblemWithSimpleSubstitution( uint8_t is_x_integer, uint8_t is_y_integer, double a_y );
libpapilo_problem_t* setupSimpleEquations( double obj_x, double obj_y, double rhs, double coef1,
                                            double coef2, double lb1, double ub1, double lb2, double ub2 );
libpapilo_problem_t* setupProblemWithSimpleSubstitutionInfeasibleGcd();
libpapilo_problem_t* setupProblemWithSimpleSubstitutionFeasibleGcd();
libpapilo_presolve_status_t check_gcd_result_with_expectation( double obj_x, double obj_y, double rhs, double coef1,
                                                               double coef2, double lb1, double ub1, double lb2, double ub2 );

TEST_CASE( "simple-substitution-happy-path-for-2-int", "[libpapilo]" )
{
   libpapilo_num_t* num = libpapilo_num_create();
   double time = 0.0;
   int cause = -1;
   libpapilo_timer_t* timer = libpapilo_timer_create( &time );
   libpapilo_message_t* msg = libpapilo_message_create();
   libpapilo_problem_t* problem = setupProblemWithSimpleSubstitution( 1, 1, 1.0 );
   libpapilo_statistics_t* statistics = libpapilo_statistics_create();
   libpapilo_presolve_options_t* presolveOptions = libpapilo_presolve_options_create();
   libpapilo_presolve_options_set_dualreds( presolveOptions, 0 );
   libpapilo_postsolve_storage_t* postsolve =
       libpapilo_postsolve_storage_create( problem, num, presolveOptions );
   libpapilo_problem_update_t* problemUpdate = libpapilo_problem_update_create(
       problem, postsolve, statistics, presolveOptions, num, msg );
   libpapilo_simple_substitution_t* presolvingMethod = libpapilo_simple_substitution_create();
   libpapilo_reductions_t* reductions = libpapilo_reductions_create();
   libpapilo_problem_recompute_all_activities( problem );

   libpapilo_presolve_status_t presolveStatus =
       libpapilo_simple_substitution_execute( presolvingMethod, problem,
                                              problemUpdate, num, reductions, timer,
                                              &cause );

   // Reduction => x = 2 - y/2 -> 0.5 (1 for int) <= x <= 2
   REQUIRE( presolveStatus == LIBPAPILO_PRESOLVE_STATUS_REDUCED );
   REQUIRE( libpapilo_reductions_get_size( reductions ) == 5 );

   libpapilo_reduction_info_t reduction0 = libpapilo_reductions_get_info( reductions, 0 );
   REQUIRE( reduction0.col == LIBPAPILO_ROW_REDUCTION_LOCKED );
   REQUIRE( reduction0.row == 0 );
   REQUIRE( reduction0.newval == 0 );

   libpapilo_reduction_info_t reduction1 = libpapilo_reductions_get_info( reductions, 1 );
   REQUIRE( reduction1.row == LIBPAPILO_COL_REDUCTION_BOUNDS_LOCKED );
   REQUIRE( reduction1.col == 1 );
   REQUIRE( reduction1.newval == 0 );

   libpapilo_reduction_info_t reduction2 = libpapilo_reductions_get_info( reductions, 2 );
   REQUIRE( reduction2.col == 0 );
   REQUIRE( reduction2.row == LIBPAPILO_COL_REDUCTION_UPPER_BOUND );
   REQUIRE( reduction2.newval == 2 );

   libpapilo_reduction_info_t reduction3 = libpapilo_reductions_get_info( reductions, 3 );
   REQUIRE( reduction3.col == 0 );
   REQUIRE( reduction3.row == LIBPAPILO_COL_REDUCTION_LOWER_BOUND );
   REQUIRE( reduction3.newval == 0.5 );

   libpapilo_reduction_info_t reduction4 = libpapilo_reductions_get_info( reductions, 4 );
   REQUIRE( reduction4.col == 1 );
   REQUIRE( reduction4.row == LIBPAPILO_COL_REDUCTION_SUBSTITUTE );
   REQUIRE( reduction4.newval == 0 );

   // Clean up
   libpapilo_reductions_free( reductions );
   libpapilo_simple_substitution_free( presolvingMethod );
   libpapilo_problem_update_free( problemUpdate );
   libpapilo_postsolve_storage_free( postsolve );
   libpapilo_presolve_options_free( presolveOptions );
   libpapilo_statistics_free( statistics );
   libpapilo_problem_free( problem );
   libpapilo_message_free( msg );
   libpapilo_timer_free( timer );
   libpapilo_num_free( num );
}

TEST_CASE( "simple-substitution-happy-path-for-int-continuous-coeff",
           "[libpapilo]" )
{
   libpapilo_message_t* msg = libpapilo_message_create();
   double time = 0.0;
   int cause = -1;
   libpapilo_timer_t* timer = libpapilo_timer_create( &time );
   libpapilo_num_t* num = libpapilo_num_create();
   libpapilo_problem_t* problem = setupProblemWithSimpleSubstitution( 1, 1, 2.2 );
   libpapilo_statistics_t* statistics = libpapilo_statistics_create();
   libpapilo_presolve_options_t* presolveOptions = libpapilo_presolve_options_create();
   libpapilo_presolve_options_set_dualreds( presolveOptions, 0 );
   libpapilo_postsolve_storage_t* postsolve =
       libpapilo_postsolve_storage_create( problem, num, presolveOptions );
   libpapilo_problem_update_t* problemUpdate = libpapilo_problem_update_create(
       problem, postsolve, statistics, presolveOptions, num, msg );
   libpapilo_simple_substitution_t* presolvingMethod = libpapilo_simple_substitution_create();
   libpapilo_reductions_t* reductions = libpapilo_reductions_create();
   libpapilo_problem_recompute_all_activities( problem );

   libpapilo_presolve_status_t presolveStatus =
       libpapilo_simple_substitution_execute( presolvingMethod, problem,
                                              problemUpdate, num, reductions, timer,
                                              &cause );
   REQUIRE( presolveStatus == LIBPAPILO_PRESOLVE_STATUS_UNCHANGED );

   // Clean up
   libpapilo_reductions_free( reductions );
   libpapilo_simple_substitution_free( presolvingMethod );
   libpapilo_problem_update_free( problemUpdate );
   libpapilo_postsolve_storage_free( postsolve );
   libpapilo_presolve_options_free( presolveOptions );
   libpapilo_statistics_free( statistics );
   libpapilo_problem_free( problem );
   libpapilo_message_free( msg );
   libpapilo_timer_free( timer );
   libpapilo_num_free( num );
}

TEST_CASE( "simple-substitution-happy-path-for-2-continuous", "[libpapilo]" )
{
   libpapilo_num_t* num = libpapilo_num_create();
   double time = 0.0;
   int cause = -1;
   libpapilo_timer_t* timer = libpapilo_timer_create( &time );
   libpapilo_message_t* msg = libpapilo_message_create();
   libpapilo_problem_t* problem = setupProblemWithSimpleSubstitution( 0, 0, 1.0 );
   libpapilo_statistics_t* statistics = libpapilo_statistics_create();
   libpapilo_presolve_options_t* presolveOptions = libpapilo_presolve_options_create();
   libpapilo_presolve_options_set_dualreds( presolveOptions, 0 );
   libpapilo_postsolve_storage_t* postsolve =
       libpapilo_postsolve_storage_create( problem, num, presolveOptions );
   libpapilo_problem_update_t* problemUpdate = libpapilo_problem_update_create(
       problem, postsolve, statistics, presolveOptions, num, msg );
   libpapilo_simple_substitution_t* presolvingMethod = libpapilo_simple_substitution_create();
   libpapilo_reductions_t* reductions = libpapilo_reductions_create();
   libpapilo_problem_recompute_all_activities( problem );

   libpapilo_presolve_status_t presolveStatus =
       libpapilo_simple_substitution_execute( presolvingMethod, problem,
                                              problemUpdate, num, reductions, timer,
                                              &cause );

   // Reduction => x = 4 - 2y -> 0 <= x <= 4 (no further bound relaxation)
   REQUIRE( presolveStatus == LIBPAPILO_PRESOLVE_STATUS_REDUCED );
   REQUIRE( libpapilo_reductions_get_size( reductions ) == 3 );

   libpapilo_reduction_info_t reduction0 = libpapilo_reductions_get_info( reductions, 0 );
   REQUIRE( reduction0.col == LIBPAPILO_ROW_REDUCTION_LOCKED );
   REQUIRE( reduction0.row == 0 );
   REQUIRE( reduction0.newval == 0 );

   libpapilo_reduction_info_t reduction1 = libpapilo_reductions_get_info( reductions, 1 );
   REQUIRE( reduction1.row == LIBPAPILO_COL_REDUCTION_BOUNDS_LOCKED );
   REQUIRE( reduction1.col == 0 );
   REQUIRE( reduction1.newval == 0 );

   libpapilo_reduction_info_t reduction2 = libpapilo_reductions_get_info( reductions, 2 );
   REQUIRE( reduction2.col == 0 );
   REQUIRE( reduction2.row == LIBPAPILO_COL_REDUCTION_SUBSTITUTE );
   REQUIRE( reduction2.newval == 0 );

   // Clean up
   libpapilo_reductions_free( reductions );
   libpapilo_simple_substitution_free( presolvingMethod );
   libpapilo_problem_update_free( problemUpdate );
   libpapilo_postsolve_storage_free( postsolve );
   libpapilo_presolve_options_free( presolveOptions );
   libpapilo_statistics_free( statistics );
   libpapilo_problem_free( problem );
   libpapilo_message_free( msg );
   libpapilo_timer_free( timer );
   libpapilo_num_free( num );
}

TEST_CASE( "simple-substitution-happy-path-for-continuous-and-integer",
           "[libpapilo]" )
{
   libpapilo_num_t* num = libpapilo_num_create();
   double time = 0.0;
   int cause = -1;
   libpapilo_timer_t* timer = libpapilo_timer_create( &time );
   libpapilo_message_t* msg = libpapilo_message_create();
   libpapilo_problem_t* problem = setupProblemWithSimpleSubstitution( 0, 1, 1.0 );
   libpapilo_statistics_t* statistics = libpapilo_statistics_create();
   libpapilo_presolve_options_t* presolveOptions = libpapilo_presolve_options_create();
   libpapilo_presolve_options_set_dualreds( presolveOptions, 0 );
   libpapilo_postsolve_storage_t* postsolve =
       libpapilo_postsolve_storage_create( problem, num, presolveOptions );
   libpapilo_problem_update_t* problemUpdate = libpapilo_problem_update_create(
       problem, postsolve, statistics, presolveOptions, num, msg );
   libpapilo_simple_substitution_t* presolvingMethod = libpapilo_simple_substitution_create();
   libpapilo_reductions_t* reductions = libpapilo_reductions_create();
   libpapilo_problem_recompute_all_activities( problem );

   libpapilo_presolve_status_t presolveStatus =
       libpapilo_simple_substitution_execute( presolvingMethod, problem,
                                              problemUpdate, num, reductions, timer,
                                              &cause );

   // Reduction => x = 4 - 2y -> 0 <= x <= 4 (no further bound relaxation)
   REQUIRE( presolveStatus == LIBPAPILO_PRESOLVE_STATUS_REDUCED );
   REQUIRE( libpapilo_reductions_get_size( reductions ) == 3 );

   libpapilo_reduction_info_t reduction0 = libpapilo_reductions_get_info( reductions, 0 );
   REQUIRE( reduction0.col == LIBPAPILO_ROW_REDUCTION_LOCKED );
   REQUIRE( reduction0.row == 0 );
   REQUIRE( reduction0.newval == 0 );

   libpapilo_reduction_info_t reduction1 = libpapilo_reductions_get_info( reductions, 1 );
   REQUIRE( reduction1.row == LIBPAPILO_COL_REDUCTION_BOUNDS_LOCKED );
   REQUIRE( reduction1.col == 0 );
   REQUIRE( reduction1.newval == 0 );

   libpapilo_reduction_info_t reduction2 = libpapilo_reductions_get_info( reductions, 2 );
   REQUIRE( reduction2.col == 0 );
   REQUIRE( reduction2.row == LIBPAPILO_COL_REDUCTION_SUBSTITUTE );
   REQUIRE( reduction2.newval == 0 );

   // Clean up
   libpapilo_reductions_free( reductions );
   libpapilo_simple_substitution_free( presolvingMethod );
   libpapilo_problem_update_free( problemUpdate );
   libpapilo_postsolve_storage_free( postsolve );
   libpapilo_presolve_options_free( presolveOptions );
   libpapilo_statistics_free( statistics );
   libpapilo_problem_free( problem );
   libpapilo_message_free( msg );
   libpapilo_timer_free( timer );
   libpapilo_num_free( num );
}

TEST_CASE( "simple-substitution-simple-substitution-for-2-int", "[libpapilo]" )
{
   libpapilo_num_t* num = libpapilo_num_create();
   double time = 0.0;
   int cause = -1;
   libpapilo_timer_t* timer = libpapilo_timer_create( &time );
   libpapilo_message_t* msg = libpapilo_message_create();
   libpapilo_problem_t* problem = setupProblemWithSimpleSubstitution( 1, 1, 3.0 );
   libpapilo_statistics_t* statistics = libpapilo_statistics_create();
   libpapilo_presolve_options_t* presolveOptions = libpapilo_presolve_options_create();
   libpapilo_presolve_options_set_dualreds( presolveOptions, 0 );
   libpapilo_postsolve_storage_t* postsolve =
       libpapilo_postsolve_storage_create( problem, num, presolveOptions );
   libpapilo_problem_update_t* problemUpdate = libpapilo_problem_update_create(
       problem, postsolve, statistics, presolveOptions, num, msg );
   libpapilo_simple_substitution_t* presolvingMethod = libpapilo_simple_substitution_create();
   libpapilo_reductions_t* reductions = libpapilo_reductions_create();
   libpapilo_problem_recompute_all_activities( problem );

   libpapilo_presolve_status_t presolveStatus =
       libpapilo_simple_substitution_execute( presolvingMethod, problem,
                                              problemUpdate, num, reductions, timer,
                                              &cause );

   REQUIRE( presolveStatus == LIBPAPILO_PRESOLVE_STATUS_UNCHANGED );

   // Clean up
   libpapilo_reductions_free( reductions );
   libpapilo_simple_substitution_free( presolvingMethod );
   libpapilo_problem_update_free( problemUpdate );
   libpapilo_postsolve_storage_free( postsolve );
   libpapilo_presolve_options_free( presolveOptions );
   libpapilo_statistics_free( statistics );
   libpapilo_problem_free( problem );
   libpapilo_message_free( msg );
   libpapilo_timer_free( timer );
   libpapilo_num_free( num );
}

TEST_CASE( "simple-substitution-2-negative-integer", "[libpapilo]" )
{
   // 2x - 2y = 4 with x,y in [0,3]
   REQUIRE( check_gcd_result_with_expectation(
                1.0, 1.0, 4.0, 2.0, 2.0, 0.0, 3.0, 0.0, 3.0 ) == LIBPAPILO_PRESOLVE_STATUS_REDUCED );
}

TEST_CASE( "simple-substitution-feasible-gcd", "[libpapilo]" )
{
   // 3x + 8y = 37 with x in {0,7} y in {0,5} -> solution x = 7, y = 2
   REQUIRE( check_gcd_result_with_expectation(
                8.0, 3.0, 37.0, 3.0, 8.0, 0.0, 7.0, 0.0, 5.0 ) == LIBPAPILO_PRESOLVE_STATUS_UNCHANGED );
   // -3x -8y = 37 with x in {-7,0} y in {-5,0} -> solution x = -7, y = -2
   REQUIRE( check_gcd_result_with_expectation(
                8.0, 3.0, 37.0, -3.0, -8.0, -7.0, 0.0, -5.0, 0.0 ) == LIBPAPILO_PRESOLVE_STATUS_UNCHANGED );
   // -3x -8y = -37 with x in {0,7} y in {0,5} -> solution x = 7, y = 2
   REQUIRE( check_gcd_result_with_expectation(
                8.0, 3.0, -37.0, -3.0, -8.0, 0.0, 7.0, 0.0, 5.0 ) == LIBPAPILO_PRESOLVE_STATUS_UNCHANGED );
   // -3x + 8y = 37 with x in {-7,0} y in {0,5} -> solution x = -7, y = 2
   REQUIRE( check_gcd_result_with_expectation(
                8.0, 3.0, 37.0, -3.0, 8.0, -7.0, 0.0, 0.0, 5.0 ) == LIBPAPILO_PRESOLVE_STATUS_UNCHANGED );
   // 3x - 8y = 37 with x in {0,7} y in {-5,0} -> solution x = 7, y = -2
   REQUIRE( check_gcd_result_with_expectation(
                8.0, 3.0, 37.0, 3.0, -8.0, 0.0, 7.0, -5.0, 0.0 ) == LIBPAPILO_PRESOLVE_STATUS_UNCHANGED );
}

TEST_CASE( "simple-substitution-non-coprime", "[libpapilo]" )
{
   // -128x - 1000 y = -2000 with x in {0,1} y in {0,1,2} -> solution x = 0, y = 2
   REQUIRE( check_gcd_result_with_expectation(
                0.0, 0.0, -2000.0, -128.0, -1000.0, 0.0, 1.0, 0.0, 2.0 ) == LIBPAPILO_PRESOLVE_STATUS_UNCHANGED );
   REQUIRE( check_gcd_result_with_expectation(
                0.0, 0.0, +2000.0, +128.0, +1000.0, 0.0, 1.0, 0.0, 2.0 ) == LIBPAPILO_PRESOLVE_STATUS_UNCHANGED );
}

TEST_CASE( "simple-substitution-violated-gcd", "[libpapilo]" )
{
   // -3x - 8y = 37 with x,y in {-5,0}
   REQUIRE( check_gcd_result_with_expectation(
                8.0, 3.0, 37.0, -3.0, 8.0, -5.0, 0.0, -5.0, 0.0 ) == LIBPAPILO_PRESOLVE_STATUS_INFEASIBLE );
   // -3x - 8y = -37 with x,y in {0,5}
   REQUIRE( check_gcd_result_with_expectation(
                8.0, 3.0, -37.0, -3.0, -8.0, 0.0, 5.0, 0.0, 5.0 ) == LIBPAPILO_PRESOLVE_STATUS_INFEASIBLE );
}

TEST_CASE( "example_10_1_in_constraint_integer_programming", "[libpapilo]" )
{
   // 3x + 8y = 37 with x,y in {0,5}
   REQUIRE( check_gcd_result_with_expectation(
                8.0, 3.0, 37.0, 3.0, 8.0, 0.0, 5.0, 0.0, 5.0 ) == LIBPAPILO_PRESOLVE_STATUS_INFEASIBLE );
}

TEST_CASE( "simple-substitution-should_return_feasible_if_gcd_of_coeff_is_in_rhs", "[libpapilo]" )
{
   libpapilo_message_t* msg = libpapilo_message_create();
   double time = 0.0;
   int cause = -1;
   libpapilo_timer_t* timer = libpapilo_timer_create( &time );
   libpapilo_num_t* num = libpapilo_num_create();
   libpapilo_problem_t* problem = setupProblemWithSimpleSubstitutionFeasibleGcd();
   libpapilo_statistics_t* statistics = libpapilo_statistics_create();
   libpapilo_presolve_options_t* presolveOptions = libpapilo_presolve_options_create();
   libpapilo_presolve_options_set_dualreds( presolveOptions, 0 );
   libpapilo_postsolve_storage_t* postsolve =
       libpapilo_postsolve_storage_create( problem, num, presolveOptions );
   libpapilo_problem_update_t* problemUpdate = libpapilo_problem_update_create(
       problem, postsolve, statistics, presolveOptions, num, msg );
   libpapilo_simple_substitution_t* presolvingMethod = libpapilo_simple_substitution_create();
   libpapilo_reductions_t* reductions = libpapilo_reductions_create();
   libpapilo_problem_recompute_all_activities( problem );

   libpapilo_presolve_status_t presolveStatus =
       libpapilo_simple_substitution_execute( presolvingMethod, problem,
                                              problemUpdate, num, reductions, timer,
                                              &cause );

   REQUIRE( presolveStatus == LIBPAPILO_PRESOLVE_STATUS_UNCHANGED );

   // Clean up
   libpapilo_reductions_free( reductions );
   libpapilo_simple_substitution_free( presolvingMethod );
   libpapilo_problem_update_free( problemUpdate );
   libpapilo_postsolve_storage_free( postsolve );
   libpapilo_presolve_options_free( presolveOptions );
   libpapilo_statistics_free( statistics );
   libpapilo_problem_free( problem );
   libpapilo_message_free( msg );
   libpapilo_timer_free( timer );
   libpapilo_num_free( num );
}

TEST_CASE( "simple-substitution-should_return_infeasible_if_gcd_of_coeff_is_in_rhs", "[libpapilo]" )
{
   libpapilo_message_t* msg = libpapilo_message_create();
   double time = 0.0;
   int cause = -1;
   libpapilo_timer_t* timer = libpapilo_timer_create( &time );
   libpapilo_num_t* num = libpapilo_num_create();
   libpapilo_problem_t* problem = setupProblemWithSimpleSubstitutionInfeasibleGcd();
   libpapilo_statistics_t* statistics = libpapilo_statistics_create();
   libpapilo_presolve_options_t* presolveOptions = libpapilo_presolve_options_create();
   libpapilo_presolve_options_set_dualreds( presolveOptions, 0 );
   libpapilo_postsolve_storage_t* postsolve =
       libpapilo_postsolve_storage_create( problem, num, presolveOptions );
   libpapilo_problem_update_t* problemUpdate = libpapilo_problem_update_create(
       problem, postsolve, statistics, presolveOptions, num, msg );
   libpapilo_simple_substitution_t* presolvingMethod = libpapilo_simple_substitution_create();
   libpapilo_reductions_t* reductions = libpapilo_reductions_create();
   libpapilo_problem_recompute_all_activities( problem );

   libpapilo_presolve_status_t presolveStatus =
       libpapilo_simple_substitution_execute( presolvingMethod, problem,
                                              problemUpdate, num, reductions, timer,
                                              &cause );

   REQUIRE( presolveStatus == LIBPAPILO_PRESOLVE_STATUS_INFEASIBLE );

   // Clean up
   libpapilo_reductions_free( reductions );
   libpapilo_simple_substitution_free( presolvingMethod );
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

libpapilo_presolve_status_t
check_gcd_result_with_expectation( double obj_x, double obj_y, double rhs, double coef1,
                                  double coef2, double lb1, double ub1,
                                  double lb2, double ub2 )
{
   libpapilo_message_t* msg = libpapilo_message_create();
   double time = 0.0;
   int cause = -1;
   libpapilo_timer_t* timer = libpapilo_timer_create( &time );
   libpapilo_num_t* num = libpapilo_num_create();
   libpapilo_problem_t* problem = setupSimpleEquations( obj_x, obj_y, rhs, coef1,
                                                        coef2, lb1, ub1, lb2, ub2 );
   libpapilo_statistics_t* statistics = libpapilo_statistics_create();
   libpapilo_presolve_options_t* presolveOptions = libpapilo_presolve_options_create();
   libpapilo_presolve_options_set_dualreds( presolveOptions, 0 );
   libpapilo_postsolve_storage_t* postsolve =
       libpapilo_postsolve_storage_create( problem, num, presolveOptions );
   libpapilo_problem_update_t* problemUpdate = libpapilo_problem_update_create(
       problem, postsolve, statistics, presolveOptions, num, msg );
   libpapilo_simple_substitution_t* presolvingMethod = libpapilo_simple_substitution_create();
   libpapilo_reductions_t* reductions = libpapilo_reductions_create();
   libpapilo_problem_recompute_all_activities( problem );

   libpapilo_presolve_status_t result =
       libpapilo_simple_substitution_execute( presolvingMethod, problem,
                                              problemUpdate, num, reductions, timer,
                                              &cause );

   // Clean up
   libpapilo_reductions_free( reductions );
   libpapilo_simple_substitution_free( presolvingMethod );
   libpapilo_problem_update_free( problemUpdate );
   libpapilo_postsolve_storage_free( postsolve );
   libpapilo_presolve_options_free( presolveOptions );
   libpapilo_statistics_free( statistics );
   libpapilo_problem_free( problem );
   libpapilo_message_free( msg );
   libpapilo_timer_free( timer );
   libpapilo_num_free( num );

   return result;
}

libpapilo_problem_t*
setupSimpleEquations( double obj_x, double obj_y, double rhs, double coef1,
                      double coef2, double lb1, double ub1,
                      double lb2, double ub2 )
{
   double coefficients[] = { obj_x, obj_y };
   double upperBounds[] = { ub1, ub2 };
   double lowerBounds[] = { lb1, lb2 };
   uint8_t isIntegral[] = { 1, 1 };

   double rhs_values[] = { rhs };
   const char* columnNames[] = { "c1", "c2" };

   libpapilo_problem_builder_t* pb = libpapilo_problem_builder_create();
   libpapilo_problem_builder_reserve( pb, 2, 1, 2 );
   libpapilo_problem_builder_set_num_rows( pb, 1 );
   libpapilo_problem_builder_set_num_cols( pb, 2 );
   libpapilo_problem_builder_set_col_ub_all( pb, upperBounds );
   libpapilo_problem_builder_set_col_lb_all( pb, lowerBounds );
   libpapilo_problem_builder_set_obj_all( pb, coefficients );
   libpapilo_problem_builder_set_obj_offset( pb, 0.0 );
   libpapilo_problem_builder_set_col_integral_all( pb, isIntegral );
   libpapilo_problem_builder_set_row_rhs_all( pb, rhs_values );
   libpapilo_problem_builder_add_entry( pb, 0, 0, coef1 );
   libpapilo_problem_builder_add_entry( pb, 0, 1, coef2 );
   libpapilo_problem_builder_set_col_name_all( pb, columnNames );
   libpapilo_problem_builder_set_problem_name( pb, "example 10.1 in Constraint Integer Programming" );
   
   libpapilo_problem_t* problem = libpapilo_problem_builder_build( pb );
   libpapilo_problem_modify_row_lhs( problem, 0, rhs );

   libpapilo_problem_builder_free( pb );
   return problem;
}

libpapilo_problem_t*
setupProblemWithSimpleSubstitution( uint8_t is_x_integer, uint8_t is_y_integer,
                                    double a_y )
{
   // 2x + y = 4
   // 0<= x,y y= 3
   double coefficients[] = { 3.0, 1.0 };
   double upperBounds[] = { 3.0, 3.0 };
   double lowerBounds[] = { 0.0, 0.0 };
   uint8_t isIntegral[] = { is_x_integer, is_y_integer };

   double rhs[] = { 4.0 };
   const char* columnNames[] = { "c1", "c2" };

   libpapilo_problem_builder_t* pb = libpapilo_problem_builder_create();
   libpapilo_problem_builder_reserve( pb, 2, 1, 2 );
   libpapilo_problem_builder_set_num_rows( pb, 1 );
   libpapilo_problem_builder_set_num_cols( pb, 2 );
   libpapilo_problem_builder_set_col_ub_all( pb, upperBounds );
   libpapilo_problem_builder_set_col_lb_all( pb, lowerBounds );
   libpapilo_problem_builder_set_obj_all( pb, coefficients );
   libpapilo_problem_builder_set_obj_offset( pb, 0.0 );
   libpapilo_problem_builder_set_col_integral_all( pb, isIntegral );
   libpapilo_problem_builder_set_row_rhs_all( pb, rhs );
   libpapilo_problem_builder_add_entry( pb, 0, 0, 2.0 );
   libpapilo_problem_builder_add_entry( pb, 0, 1, a_y );
   libpapilo_problem_builder_set_col_name_all( pb, columnNames );
   libpapilo_problem_builder_set_problem_name( pb, "matrix for testing simple probing" );
   
   libpapilo_problem_t* problem = libpapilo_problem_builder_build( pb );
   libpapilo_problem_modify_row_lhs( problem, 0, rhs[0] );

   libpapilo_problem_builder_free( pb );
   return problem;
}

libpapilo_problem_t*
setupProblemWithSimpleSubstitutionInfeasibleGcd()
{
   // 6x + 8y = 37
   // 0<= x,y y= 5
   double coefficients[] = { 3.0, 1.0 };
   double upperBounds[] = { 5.0, 5.0 };
   double lowerBounds[] = { 0.0, 0.0 };
   uint8_t isIntegral[] = { 1, 1 };

   double rhs[] = { 37.0 };
   const char* columnNames[] = { "c1", "c2" };

   libpapilo_problem_builder_t* pb = libpapilo_problem_builder_create();
   libpapilo_problem_builder_reserve( pb, 2, 1, 2 );
   libpapilo_problem_builder_set_num_rows( pb, 1 );
   libpapilo_problem_builder_set_num_cols( pb, 2 );
   libpapilo_problem_builder_set_col_ub_all( pb, upperBounds );
   libpapilo_problem_builder_set_col_lb_all( pb, lowerBounds );
   libpapilo_problem_builder_set_obj_all( pb, coefficients );
   libpapilo_problem_builder_set_obj_offset( pb, 0.0 );
   libpapilo_problem_builder_set_col_integral_all( pb, isIntegral );
   libpapilo_problem_builder_set_row_rhs_all( pb, rhs );
   libpapilo_problem_builder_add_entry( pb, 0, 0, 6.0 );
   libpapilo_problem_builder_add_entry( pb, 0, 1, 8.0 );
   libpapilo_problem_builder_set_col_name_all( pb, columnNames );
   libpapilo_problem_builder_set_problem_name( pb, "gcd(x,y) is not divisor of rhs" );
   
   libpapilo_problem_t* problem = libpapilo_problem_builder_build( pb );
   libpapilo_problem_modify_row_lhs( problem, 0, rhs[0] );

   libpapilo_problem_builder_free( pb );
   return problem;
}

libpapilo_problem_t*
setupProblemWithSimpleSubstitutionFeasibleGcd()
{
   // 6x + 9y = 15 with 15/6 and 9/6 no integer
   // 0<= x,y y= 5
   double coefficients[] = { 3.0, 1.0 };
   double upperBounds[] = { 5.0, 5.0 };
   double lowerBounds[] = { 0.0, 0.0 };
   uint8_t isIntegral[] = { 1, 1 };

   double rhs[] = { 15.0 };
   const char* columnNames[] = { "c1", "c2" };

   libpapilo_problem_builder_t* pb = libpapilo_problem_builder_create();
   libpapilo_problem_builder_reserve( pb, 2, 1, 2 );
   libpapilo_problem_builder_set_num_rows( pb, 1 );
   libpapilo_problem_builder_set_num_cols( pb, 2 );
   libpapilo_problem_builder_set_col_ub_all( pb, upperBounds );
   libpapilo_problem_builder_set_col_lb_all( pb, lowerBounds );
   libpapilo_problem_builder_set_obj_all( pb, coefficients );
   libpapilo_problem_builder_set_obj_offset( pb, 0.0 );
   libpapilo_problem_builder_set_col_integral_all( pb, isIntegral );
   libpapilo_problem_builder_set_row_rhs_all( pb, rhs );
   libpapilo_problem_builder_add_entry( pb, 0, 0, 6.0 );
   libpapilo_problem_builder_add_entry( pb, 0, 1, 9.0 );
   libpapilo_problem_builder_set_col_name_all( pb, columnNames );
   libpapilo_problem_builder_set_problem_name( pb, "gcd(x,y) is divisor of rhs" );
   
   libpapilo_problem_t* problem = libpapilo_problem_builder_build( pb );
   libpapilo_problem_modify_row_lhs( problem, 0, rhs[0] );

   libpapilo_problem_builder_free( pb );
   return problem;
}