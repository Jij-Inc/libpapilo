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

TEST_CASE( "finding-the-right-value-in-postsolve-for-a-column-fixed-neg-inf",
           "[libpapilo]" )
{
   // Create objects with C API
   libpapilo_num_t* num = libpapilo_num_create();
   libpapilo_message_t* message = libpapilo_message_create();
   const std::string gen_neg =
       std::string( LIBPAPILO_BUILD_DIR ) + "/dual_fix_neg_inf.postsolve";
   libpapilo_postsolve_storage_t* postsolveStorage =
       libpapilo_postsolve_storage_load_from_file( gen_neg.c_str() );

   libpapilo_solution_t* reduced_solution = libpapilo_solution_create();
   libpapilo_solution_t* original_solution = libpapilo_solution_create();
   libpapilo_postsolve_t* postsolve =
       libpapilo_postsolve_create( message, num );

   libpapilo_postsolve_status_t status = libpapilo_postsolve_undo(
       postsolve, reduced_solution, original_solution, postsolveStorage );

   REQUIRE( status == LIBPAPILO_POSTSOLVE_STATUS_OK );

   size_t size;
   const double* values =
       libpapilo_solution_get_primal( original_solution, &size );
   REQUIRE( size == 3 );
   REQUIRE( values[0] == -11.0 );
   REQUIRE( values[1] == -5.0 );
   REQUIRE( values[2] == -5.0 );

   // Clean up
   libpapilo_postsolve_storage_free( postsolveStorage );
   libpapilo_solution_free( reduced_solution );
   libpapilo_solution_free( original_solution );
   libpapilo_postsolve_free( postsolve );
   libpapilo_message_free( message );
   libpapilo_num_free( num );
}

TEST_CASE( "finding-the-right-value-in-postsolve-for-a-column-fixed-pos-inf",
           "[libpapilo]" )
{
   // Create objects with C API
   libpapilo_num_t* num = libpapilo_num_create();
   libpapilo_message_t* message = libpapilo_message_create();
   const std::string gen_pos =
       std::string( LIBPAPILO_BUILD_DIR ) + "/dual_fix_pos_inf.postsolve";
   libpapilo_postsolve_storage_t* postsolveStorage =
       libpapilo_postsolve_storage_load_from_file( gen_pos.c_str() );

   libpapilo_solution_t* reduced_solution = libpapilo_solution_create();
   libpapilo_solution_t* original_solution = libpapilo_solution_create();
   libpapilo_postsolve_t* postsolve =
       libpapilo_postsolve_create( message, num );

   libpapilo_postsolve_status_t status = libpapilo_postsolve_undo(
       postsolve, reduced_solution, original_solution, postsolveStorage );

   REQUIRE( status == LIBPAPILO_POSTSOLVE_STATUS_OK );

   size_t size;
   const double* values =
       libpapilo_solution_get_primal( original_solution, &size );
   REQUIRE( size == 4 );
   REQUIRE( values[0] == 13.0 );
   REQUIRE( values[1] == 9.0 );
   REQUIRE( values[2] == -5.0 );
   REQUIRE( values[3] == -2.5 );

   // Clean up
   libpapilo_postsolve_storage_free( postsolveStorage );
   libpapilo_solution_free( reduced_solution );
   libpapilo_solution_free( original_solution );
   libpapilo_postsolve_free( postsolve );
   libpapilo_message_free( message );
   libpapilo_num_free( num );
}
