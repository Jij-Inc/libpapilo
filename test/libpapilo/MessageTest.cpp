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
#include <string>
#include <vector>

struct Buffer
{
   std::vector<std::string> lines;
   static void
   cb( int, const char* data, size_t size, void* usr )
   {
      auto* self = reinterpret_cast<Buffer*>( usr );
      self->lines.emplace_back( data, data + size );
   }
};

TEST_CASE( "message-set-get-verbosity", "[libpapilo]" )
{
   libpapilo_message_t* msg = libpapilo_message_create();

   libpapilo_message_set_verbosity_level( msg, -1 );
   REQUIRE( libpapilo_message_get_verbosity_level( msg ) == 0 );

   libpapilo_message_set_verbosity_level( msg, 5 );
   REQUIRE( libpapilo_message_get_verbosity_level( msg ) == 4 );

   libpapilo_message_free( msg );
}

TEST_CASE( "message-callback-simple", "[libpapilo]" )
{
   libpapilo_message_t* msg = libpapilo_message_create();

   Buffer buf;
   libpapilo_message_set_output_callback( msg, &Buffer::cb, &buf );
   libpapilo_message_print( msg, 3 /* info */, "hello" );

   REQUIRE( buf.lines.size() == 1 );
   REQUIRE( buf.lines[0].find( "hello" ) != std::string::npos );

   libpapilo_message_free( msg );
}
