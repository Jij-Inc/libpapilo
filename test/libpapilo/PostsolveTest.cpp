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
