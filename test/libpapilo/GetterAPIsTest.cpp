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
#include <vector>

TEST_CASE( "Statistics getters", "[libpapilo]" )
{
   SECTION( "Test statistics getters" )
   {
      // Create a statistics object
      libpapilo_statistics_t* stats = libpapilo_statistics_create();
      REQUIRE( stats != nullptr );

      // Test all getter functions - initially should return 0
      REQUIRE( libpapilo_statistics_get_presolvetime( stats ) == 0.0 );
      REQUIRE( libpapilo_statistics_get_ntsxapplied( stats ) == 0 );
      REQUIRE( libpapilo_statistics_get_ntsxconflicts( stats ) == 0 );
      REQUIRE( libpapilo_statistics_get_nboundchgs( stats ) == 0 );
      REQUIRE( libpapilo_statistics_get_nsidechgs( stats ) == 0 );
      REQUIRE( libpapilo_statistics_get_ncoefchgs( stats ) == 0 );
      REQUIRE( libpapilo_statistics_get_nrounds( stats ) == 0 );
      REQUIRE( libpapilo_statistics_get_ndeletedcols( stats ) == 0 );
      REQUIRE( libpapilo_statistics_get_ndeletedrows( stats ) == 0 );
      REQUIRE( libpapilo_statistics_get_consecutive_rounds_of_only_boundchanges(
                   stats ) == 0 );
      REQUIRE( libpapilo_statistics_get_single_matrix_coefficient_changes(
                   stats ) == 0 );

      libpapilo_statistics_free( stats );
   }
}

TEST_CASE( "Reductions getters", "[libpapilo]" )
{
   SECTION( "Test reductions getters" )
   {
      // Create a reductions object
      libpapilo_reductions_t* reductions = libpapilo_reductions_create();
      REQUIRE( reductions != nullptr );

      // Initially should have 0 size and 0 transactions
      REQUIRE( libpapilo_reductions_get_size( reductions ) == 0 );
      REQUIRE( libpapilo_reductions_get_num_transactions( reductions ) == 0 );

      // Begin and end a transaction
      libpapilo_reductions_begin_transaction( reductions );
      libpapilo_reductions_end_transaction( reductions );

      // Now should have 1 transaction
      REQUIRE( libpapilo_reductions_get_num_transactions( reductions ) == 1 );

      // Test transaction getters
      int transaction_start =
          libpapilo_reductions_get_transaction_start( reductions, 0 );
      int transaction_end =
          libpapilo_reductions_get_transaction_end( reductions, 0 );
      int nlocks = libpapilo_reductions_get_transaction_nlocks( reductions, 0 );
      int naddcoeffs =
          libpapilo_reductions_get_transaction_naddcoeffs( reductions, 0 );

      REQUIRE( transaction_start == 0 );
      REQUIRE( transaction_end == 0 );
      REQUIRE( nlocks == 0 );
      REQUIRE( naddcoeffs == 0 );

      libpapilo_reductions_free( reductions );
   }
}

TEST_CASE( "PostsolveStorage getters", "[libpapilo]" )
{
   SECTION( "Test postsolve storage getters" )
   {
      // Create a simple problem for testing
      libpapilo_problem_builder_t* builder = libpapilo_problem_builder_create();
      REQUIRE( builder != nullptr );

      // Set dimensions
      libpapilo_problem_builder_set_num_cols( builder, 2 );
      libpapilo_problem_builder_set_num_rows( builder, 1 );

      // Set variable bounds and objective
      libpapilo_problem_builder_set_col_lb( builder, 0, 0.0 );
      libpapilo_problem_builder_set_col_ub( builder, 0, 10.0 );
      libpapilo_problem_builder_set_col_lb( builder, 1, 0.0 );
      libpapilo_problem_builder_set_col_ub( builder, 1, 10.0 );

      libpapilo_problem_builder_set_obj( builder, 0, 1.0 );
      libpapilo_problem_builder_set_obj( builder, 1, 2.0 );

      // Set constraint bounds
      libpapilo_problem_builder_set_row_lhs( builder, 0, 0.0 );
      libpapilo_problem_builder_set_row_rhs( builder, 0, 10.0 );

      // Add matrix entries: x + y <= 10
      libpapilo_problem_builder_add_entry( builder, 0, 0, 1.0 );
      libpapilo_problem_builder_add_entry( builder, 0, 1, 1.0 );

      libpapilo_problem_t* problem = libpapilo_problem_builder_build( builder );
      REQUIRE( problem != nullptr );

      // Create num and options
      libpapilo_num_t* num = libpapilo_num_create();
      libpapilo_presolve_options_t* options =
          libpapilo_presolve_options_create();

      // Create postsolve storage
      libpapilo_postsolve_storage_t* postsolve =
          libpapilo_postsolve_storage_create( problem, num, options );
      REQUIRE( postsolve != nullptr );

      // Test getters
      REQUIRE( libpapilo_postsolve_storage_get_n_cols_original( postsolve ) ==
               2 );
      REQUIRE( libpapilo_postsolve_storage_get_n_rows_original( postsolve ) ==
               1 );

      // Test mapping getters
      int col_size = 0;
      const int* col_mapping = libpapilo_postsolve_storage_get_orig_col_mapping(
          postsolve, &col_size );
      REQUIRE( col_mapping != nullptr );
      REQUIRE( col_size == 2 );
      REQUIRE( col_mapping[0] == 0 );
      REQUIRE( col_mapping[1] == 1 );

      int row_size = 0;
      const int* row_mapping = libpapilo_postsolve_storage_get_orig_row_mapping(
          postsolve, &row_size );
      REQUIRE( row_mapping != nullptr );
      REQUIRE( row_size == 1 );
      REQUIRE( row_mapping[0] == 0 );

      // Test postsolve type
      libpapilo_postsolve_type_t type =
          libpapilo_postsolve_storage_get_postsolve_type( postsolve );
      REQUIRE( type == LIBPAPILO_POSTSOLVE_TYPE_PRIMAL );

      // Test arrays sizes (initially should be 0 or small)
      REQUIRE( libpapilo_postsolve_storage_get_num_types( postsolve ) >= 0 );
      REQUIRE( libpapilo_postsolve_storage_get_num_indices( postsolve ) >= 0 );
      REQUIRE( libpapilo_postsolve_storage_get_num_values( postsolve ) >= 0 );

      // Test original problem getter
      const libpapilo_problem_t* orig_problem =
          libpapilo_postsolve_storage_get_original_problem( postsolve );
      REQUIRE( orig_problem != nullptr );
      REQUIRE( libpapilo_problem_get_ncols( orig_problem ) == 2 );
      REQUIRE( libpapilo_problem_get_nrows( orig_problem ) == 1 );

      // Clean up
      libpapilo_postsolve_storage_free( postsolve );
      libpapilo_presolve_options_free( options );
      libpapilo_num_free( num );
      libpapilo_problem_free( problem );
      libpapilo_problem_builder_free( builder );
   }
}