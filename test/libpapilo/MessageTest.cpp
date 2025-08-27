/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                           */
/* This file is part of the library libpapilo, a fork of PaPILO from ZIB     */
/*                                                                           */
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
