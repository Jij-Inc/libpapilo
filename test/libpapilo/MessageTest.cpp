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

struct Buffer {
   std::vector<std::string> lines;
   static void cb( int, const char* data, size_t size, void* usr )
   {
      auto* self = reinterpret_cast<Buffer*>( usr );
      self->lines.emplace_back( data, data + size );
   }
};

TEST_CASE( "message-set-get-verbosity", "[libpapilo]" )
{
   libpapilo_message_t* msg = libpapilo_message_create();

   libpapilo_message_set_verbosity( msg, -1 );
   REQUIRE( libpapilo_message_get_verbosity( msg ) == 0 );

   libpapilo_message_set_verbosity( msg, 5 );
   REQUIRE( libpapilo_message_get_verbosity( msg ) == 4 );

   libpapilo_message_free( msg );
}

#ifdef LIBPAPILO_ENABLE_TEST_HOOKS
TEST_CASE( "message-callback-prefix-timestamp", "[libpapilo]" )
{
   libpapilo_message_t* msg = libpapilo_message_create();

   Buffer buf;
   libpapilo_message_set_trace_callback( msg, &Buffer::cb, &buf );
   libpapilo_message_set_prefix( msg, "[T] " );
   libpapilo_message_enable_timestamps( msg, 1, "%Y" );

   libpapilo_message_emit( msg, 3 /* info */, "hello" );

   REQUIRE( buf.lines.size() == 1 );
   REQUIRE( buf.lines[0].find( "[T] " ) == 0 );
   REQUIRE( buf.lines[0].find( "hello" ) != std::string::npos );

   libpapilo_message_free( msg );
}

TEST_CASE( "message-file-and-stderr-routing", "[libpapilo]" )
{
   libpapilo_message_t* msg = libpapilo_message_create();

   // File sink
   std::string path = std::string( LIBPAPILO_BUILD_DIR ) + "/msg_test.log";
   REQUIRE( libpapilo_message_set_trace_file( msg, path.c_str(), 0 ) == 0 );
   libpapilo_message_emit( msg, 3, "file-only" );

   // Now route errors to stderr: error must not go to file
   libpapilo_message_route_errors_to_stderr( msg, 1 );
   libpapilo_message_emit( msg, 1, "to-stderr" );

   // Read file and check content
   FILE* f = std::fopen( path.c_str(), "rb" );
   REQUIRE( f != nullptr );
   std::string content;
   char buf[256];
   size_t n;
   while( ( n = std::fread( buf, 1, sizeof( buf ), f ) ) > 0 )
      content.append( buf, buf + n );
   std::fclose( f );

   REQUIRE( content.find( "file-only" ) != std::string::npos );
   REQUIRE( content.find( "to-stderr" ) == std::string::npos );

   libpapilo_message_free( msg );
}
#endif

