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

#include "papilo/CMakeConfig.hpp"
#include "papilo/core/Presolve.hpp"
#include "papilo/core/PresolveOptions.hpp"
#include "papilo/core/Problem.hpp"
#include "papilo/core/ProblemBuilder.hpp"
#include "papilo/core/ProblemUpdate.hpp"
#include "papilo/core/Reductions.hpp"
#include "papilo/core/Solution.hpp"
#include "papilo/core/Statistics.hpp"
#include "papilo/core/postsolve/Postsolve.hpp"
#include "papilo/core/postsolve/PostsolveStatus.hpp"
#include "papilo/core/postsolve/PostsolveStorage.hpp"
#include "papilo/io/Message.hpp"
#include "papilo/misc/Num.hpp"
#include "papilo/misc/Timer.hpp"
#include "papilo/misc/Vec.hpp"
#include "papilo/presolvers/SimpleSubstitution.hpp"
#include "papilo/presolvers/SingletonCols.hpp"
#include <boost/archive/binary_iarchive.hpp>
#include <fstream>

#include <cstring>
#include <iostream>
#include <limits>
#include <memory>

using namespace papilo;

/** Magic number to check the pointer passed from user is ours */
const uint64_t LIBPAPILO_MAGIC_NUMBER = 0x506150494C4F; // 'PaPILO'

struct libpapilo_problem_builder_t
{
   uint64_t magic_number = LIBPAPILO_MAGIC_NUMBER;
   ProblemBuilder<double> builder;
};

struct libpapilo_problem_t
{
   uint64_t magic_number = LIBPAPILO_MAGIC_NUMBER;
   Problem<double> problem;
};

struct libpapilo_presolve_options_t
{
   uint64_t magic_number = LIBPAPILO_MAGIC_NUMBER;
   PresolveOptions options;
};

struct libpapilo_statistics_t
{
   uint64_t magic_number = LIBPAPILO_MAGIC_NUMBER;
   Statistics statistics;
};

struct libpapilo_postsolve_storage_t
{
   uint64_t magic_number = LIBPAPILO_MAGIC_NUMBER;
   PostsolveStorage<double> postsolve;

   // Default constructor
   libpapilo_postsolve_storage_t() = default;
   // Constructor to properly initialize postsolve
   libpapilo_postsolve_storage_t( PostsolveStorage<double>&& ps )
       : postsolve( std::move( ps ) )
   {
   }
};

struct libpapilo_problem_update_t
{
   uint64_t magic_number = LIBPAPILO_MAGIC_NUMBER;
   ProblemUpdate<double> update;

   // Constructor to properly initialize ProblemUpdate with references
   libpapilo_problem_update_t( Problem<double>& problem,
                               PostsolveStorage<double>& postsolve,
                               Statistics& stats,
                               const PresolveOptions& options,
                               const Num<double>& num, const Message& msg )
       : update( problem, postsolve, stats, options, num, msg )
   {
   }
};

struct libpapilo_reductions_t
{
   uint64_t magic_number = LIBPAPILO_MAGIC_NUMBER;
   Reductions<double> reductions;
   std::unique_ptr<TransactionGuard<double>> transaction_guard;
};

struct libpapilo_singleton_cols_t
{
   uint64_t magic_number = LIBPAPILO_MAGIC_NUMBER;
   SingletonCols<double> presolver;
};

struct libpapilo_simple_substitution_t
{
   uint64_t magic_number = LIBPAPILO_MAGIC_NUMBER;
   SimpleSubstitution<double> presolver;
};

struct libpapilo_num_t
{
   uint64_t magic_number = LIBPAPILO_MAGIC_NUMBER;
   Num<double> num;
};

struct libpapilo_timer_t
{
   uint64_t magic_number = LIBPAPILO_MAGIC_NUMBER;
   Timer timer;

   libpapilo_timer_t( double& time ) : timer( time ) {}
};

struct libpapilo_message_t
{
   uint64_t magic_number = LIBPAPILO_MAGIC_NUMBER;
   Message message;
};

struct libpapilo_presolve_t
{
   uint64_t magic_number = LIBPAPILO_MAGIC_NUMBER;
   Presolve<double> presolve;
};

struct libpapilo_solution_t
{
   uint64_t magic_number = LIBPAPILO_MAGIC_NUMBER;
   Solution<double> solution;
};

struct libpapilo_postsolve_t
{
   uint64_t magic_number = LIBPAPILO_MAGIC_NUMBER;
   Postsolve<double> postsolve;

   libpapilo_postsolve_t( const Message& msg, const Num<double>& num )
       : postsolve( msg, num )
   {
   }
};

/** Custom assert also working on release build */
void
custom_assert( bool condition, const char* message )
{
   if( !condition )
   {
      std::cerr << "libpapilo error: " << message << std::endl;
      std::terminate();
   }
}

/** Check the pointer passed from user code is valid. */
void
check_problem_builder_ptr( const libpapilo_problem_builder_t* builder )
{
   custom_assert( builder != nullptr,
                  "libpapilo_problem_builder_t pointer is null" );
   custom_assert(
       builder->magic_number == LIBPAPILO_MAGIC_NUMBER,
       "Invalid libpapilo_problem_builder_t pointer (magic number mismatch)" );
}

/** Check the pointer passed from user code is valid. */
void
check_problem_ptr( const libpapilo_problem_t* problem )
{
   custom_assert( problem != nullptr, "libpapilo_problem_t pointer is null" );
   custom_assert(
       problem->magic_number == LIBPAPILO_MAGIC_NUMBER,
       "Invalid libpapilo_problem_t pointer (magic number mismatch)" );
}

/** Check the pointer passed from user code is valid. */
void
check_presolve_options_ptr( const libpapilo_presolve_options_t* options )
{
   custom_assert( options != nullptr,
                  "libpapilo_presolve_options_t pointer is null" );
   custom_assert(
       options->magic_number == LIBPAPILO_MAGIC_NUMBER,
       "Invalid libpapilo_presolve_options_t pointer (magic number mismatch)" );
}

/** Check the pointer passed from user code is valid. */
void
check_statistics_ptr( const libpapilo_statistics_t* statistics )
{
   custom_assert( statistics != nullptr,
                  "libpapilo_statistics_t pointer is null" );
   custom_assert(
       statistics->magic_number == LIBPAPILO_MAGIC_NUMBER,
       "Invalid libpapilo_statistics_t pointer (magic number mismatch)" );
}

/** Check the pointer passed from user code is valid. */
void
check_postsolve_storage_ptr( const libpapilo_postsolve_storage_t* postsolve )
{
   custom_assert( postsolve != nullptr,
                  "libpapilo_postsolve_storage_t pointer is null" );
   custom_assert( postsolve->magic_number == LIBPAPILO_MAGIC_NUMBER,
                  "Invalid libpapilo_postsolve_storage_t pointer (magic "
                  "number mismatch)" );
}

/** Check the pointer passed from user code is valid. */
void
check_reductions_ptr( const libpapilo_reductions_t* reductions )
{
   custom_assert( reductions != nullptr,
                  "libpapilo_reductions_t pointer is null" );
   custom_assert(
       reductions->magic_number == LIBPAPILO_MAGIC_NUMBER,
       "Invalid libpapilo_reductions_t pointer (magic number mismatch)" );
}

/** Check the pointer passed from user code is valid. */
void
check_singleton_cols_ptr( const libpapilo_singleton_cols_t* presolver )
{
   custom_assert( presolver != nullptr,
                  "libpapilo_singleton_cols_t pointer is null" );
   custom_assert(
       presolver->magic_number == LIBPAPILO_MAGIC_NUMBER,
       "Invalid libpapilo_singleton_cols_t pointer (magic number mismatch)" );
}

/** Check the pointer passed from user code is valid. */
void
check_simple_substitution_ptr(
    const libpapilo_simple_substitution_t* presolver )
{
   custom_assert( presolver != nullptr,
                  "libpapilo_simple_substitution_t pointer is null" );
   custom_assert( presolver->magic_number == LIBPAPILO_MAGIC_NUMBER,
                  "Invalid libpapilo_simple_substitution_t pointer (magic "
                  "number mismatch)" );
}

/** Check the pointer passed from user code is valid. */
void
check_num_ptr( const libpapilo_num_t* num )
{
   custom_assert( num != nullptr, "libpapilo_num_t pointer is null" );
   custom_assert( num->magic_number == LIBPAPILO_MAGIC_NUMBER,
                  "Invalid libpapilo_num_t pointer (magic number mismatch)" );
}

/** Check the pointer passed from user code is valid. */
void
check_timer_ptr( const libpapilo_timer_t* timer )
{
   custom_assert( timer != nullptr, "libpapilo_timer_t pointer is null" );
   custom_assert( timer->magic_number == LIBPAPILO_MAGIC_NUMBER,
                  "Invalid libpapilo_timer_t pointer (magic number mismatch)" );
}

/** Check the pointer passed from user code is valid. */
void
check_message_ptr( const libpapilo_message_t* message )
{
   custom_assert( message != nullptr, "libpapilo_message_t pointer is null" );
   custom_assert(
       message->magic_number == LIBPAPILO_MAGIC_NUMBER,
       "Invalid libpapilo_message_t pointer (magic number mismatch)" );
}

/** Check the pointer passed from user code is valid. */
void
check_problem_update_ptr( const libpapilo_problem_update_t* update )
{
   custom_assert( update != nullptr,
                  "libpapilo_problem_update_t pointer is null" );
   custom_assert(
       update->magic_number == LIBPAPILO_MAGIC_NUMBER,
       "Invalid libpapilo_problem_update_t pointer (magic number mismatch)" );
}

/** Check the pointer passed from user code is valid. */
void
check_presolve_ptr( const libpapilo_presolve_t* presolve )
{
   custom_assert( presolve != nullptr, "libpapilo_presolve_t pointer is null" );
   custom_assert(
       presolve->magic_number == LIBPAPILO_MAGIC_NUMBER,
       "Invalid libpapilo_presolve_t pointer (magic number mismatch)" );
}

static void
check_solution_ptr( const libpapilo_solution_t* solution )
{
   custom_assert( solution != nullptr, "libpapilo_solution_t pointer is null" );
   custom_assert(
       solution->magic_number == LIBPAPILO_MAGIC_NUMBER,
       "Invalid libpapilo_solution_t pointer (magic number mismatch)" );
}

static void
check_postsolve_ptr( const libpapilo_postsolve_t* postsolve )
{
   custom_assert( postsolve != nullptr,
                  "libpapilo_postsolve_t pointer is null" );
   custom_assert(
       postsolve->magic_number == LIBPAPILO_MAGIC_NUMBER,
       "Invalid libpapilo_postsolve_t pointer (magic number mismatch)" );
}

template <typename Func>
auto
check_run( Func func, const char* message )
{
   try
   {
      return func();
   }
   catch( const std::exception& e )
   {
      std::cerr << "libpapilo error: " << message << ": " << e.what()
                << std::endl;
   }
   catch( ... )
   {
      std::cerr << "libpapilo error: " << message << ": Unknown exception"
                << std::endl;
   }
   // For now, we just terminate the process on exception.
   // This strict strategy would be not ideal, but this keeps the API simple.
   std::terminate();
}

// Helper function to convert PresolveStatus to libpapilo_presolve_status_t
static libpapilo_presolve_status_t
convert_presolve_status( PresolveStatus status )
{
   switch( status )
   {
   case PresolveStatus::kUnchanged:
      return LIBPAPILO_PRESOLVE_STATUS_UNCHANGED;
   case PresolveStatus::kReduced:
      return LIBPAPILO_PRESOLVE_STATUS_REDUCED;
   case PresolveStatus::kUnbounded:
      return LIBPAPILO_PRESOLVE_STATUS_UNBOUNDED;
   case PresolveStatus::kUnbndOrInfeas:
      return LIBPAPILO_PRESOLVE_STATUS_UNBOUNDED_OR_INFEASIBLE;
   case PresolveStatus::kInfeasible:
      return LIBPAPILO_PRESOLVE_STATUS_INFEASIBLE;
   default:
      custom_assert( false, "Unknown presolve status" );
      return LIBPAPILO_PRESOLVE_STATUS_UNCHANGED;
   }
}

static libpapilo_postsolve_status_t
convert_postsolve_status( PostsolveStatus status )
{
   switch( status )
   {
   case PostsolveStatus::kOk:
      return LIBPAPILO_POSTSOLVE_STATUS_OK;
   default:
      return LIBPAPILO_POSTSOLVE_STATUS_ERROR;
   }
}

extern "C"
{

   const char*
   libpapilo_version()
   {
      return LIBPAPILO_VERSION;
   }

   libpapilo_problem_builder_t*
   libpapilo_problem_builder_create()
   {
      return check_run( []() { return new libpapilo_problem_builder_t(); },
                        "Failed to create problem builder" );
   }

   void
   libpapilo_problem_builder_free( libpapilo_problem_builder_t* builder )
   {
      check_problem_builder_ptr( builder );
      delete builder;
   }

   void
   libpapilo_problem_builder_reserve( libpapilo_problem_builder_t* builder,
                                      int nnz, int nrows, int ncols )
   {
      check_problem_builder_ptr( builder );
      builder->builder.reserve( nnz, nrows, ncols );
   }

   void
   libpapilo_problem_builder_set_num_cols( libpapilo_problem_builder_t* builder,
                                           int ncols )
   {
      check_problem_builder_ptr( builder );
      builder->builder.setNumCols( ncols );
   }

   void
   libpapilo_problem_builder_set_num_rows( libpapilo_problem_builder_t* builder,
                                           int nrows )
   {
      check_problem_builder_ptr( builder );
      builder->builder.setNumRows( nrows );
   }

   int
   libpapilo_problem_builder_get_num_cols(
       const libpapilo_problem_builder_t* builder )
   {
      check_problem_builder_ptr( builder );
      return builder->builder.getNumCols();
   }

   int
   libpapilo_problem_builder_get_num_rows(
       const libpapilo_problem_builder_t* builder )
   {
      check_problem_builder_ptr( builder );
      return builder->builder.getNumRows();
   }

   void
   libpapilo_problem_builder_set_obj( libpapilo_problem_builder_t* builder,
                                      int col, double val )
   {
      check_problem_builder_ptr( builder );
      builder->builder.setObj( col, val );
   }

   void
   libpapilo_problem_builder_set_obj_all( libpapilo_problem_builder_t* builder,
                                          const double* values )
   {
      check_problem_builder_ptr( builder );
      custom_assert(
          values != nullptr,
          "libpapilo_problem_builder_set_obj_all: values pointer is null" );
      int ncols = builder->builder.getNumCols();
      Vec<double> vals( values, values + ncols );
      builder->builder.setObjAll( std::move( vals ) );
   }

   void
   libpapilo_problem_builder_set_obj_offset(
       libpapilo_problem_builder_t* builder, double val )
   {
      check_problem_builder_ptr( builder );
      builder->builder.setObjOffset( val );
   }

   void
   libpapilo_problem_builder_set_col_lb( libpapilo_problem_builder_t* builder,
                                         int col, double lb )
   {
      check_problem_builder_ptr( builder );
      builder->builder.setColLb( col, lb );
      if( std::isinf( lb ) && lb < 0 )
         builder->builder.setColLbInf( col, true );
      else
         builder->builder.setColLbInf( col, false );
   }

   void
   libpapilo_problem_builder_set_col_lb_all(
       libpapilo_problem_builder_t* builder, const double* lbs )
   {
      check_problem_builder_ptr( builder );
      custom_assert(
          lbs != nullptr,
          "libpapilo_problem_builder_set_col_lb_all: lbs pointer is null" );
      int ncols = builder->builder.getNumCols();
      Vec<double> vals( lbs, lbs + ncols );
      builder->builder.setColLbAll( std::move( vals ) );

      // Set infinity flags
      for( int i = 0; i < ncols; ++i )
      {
         if( std::isinf( lbs[i] ) && lbs[i] < 0 )
            builder->builder.setColLbInf( i, true );
         else
            builder->builder.setColLbInf( i, false );
      }
   }

   void
   libpapilo_problem_builder_set_col_ub( libpapilo_problem_builder_t* builder,
                                         int col, double ub )
   {
      check_problem_builder_ptr( builder );
      builder->builder.setColUb( col, ub );
      if( std::isinf( ub ) && ub > 0 )
         builder->builder.setColUbInf( col, true );
      else
         builder->builder.setColUbInf( col, false );
   }

   void
   libpapilo_problem_builder_set_col_ub_all(
       libpapilo_problem_builder_t* builder, const double* ubs )
   {
      check_problem_builder_ptr( builder );
      custom_assert(
          ubs != nullptr,
          "libpapilo_problem_builder_set_col_ub_all: ubs pointer is null" );
      int ncols = builder->builder.getNumCols();
      Vec<double> vals( ubs, ubs + ncols );
      builder->builder.setColUbAll( std::move( vals ) );

      // Set infinity flags
      for( int i = 0; i < ncols; ++i )
      {
         if( std::isinf( ubs[i] ) && ubs[i] > 0 )
            builder->builder.setColUbInf( i, true );
         else
            builder->builder.setColUbInf( i, false );
      }
   }

   /* Column infinity bounds implementations */

   void
   libpapilo_problem_builder_set_col_lb_inf_all(
       libpapilo_problem_builder_t* builder, const uint8_t* is_inf )
   {
      check_problem_builder_ptr( builder );
      custom_assert( is_inf != nullptr, "libpapilo_problem_builder_set_col_lb_"
                                        "inf_all: is_inf pointer is null" );
      int ncols = builder->builder.getNumCols();
      Vec<uint8_t> vals( is_inf, is_inf + ncols );
      builder->builder.setColLbInfAll( std::move( vals ) );
   }

   void
   libpapilo_problem_builder_set_col_ub_inf_all(
       libpapilo_problem_builder_t* builder, const uint8_t* is_inf )
   {
      check_problem_builder_ptr( builder );
      custom_assert( is_inf != nullptr, "libpapilo_problem_builder_set_col_ub_"
                                        "inf_all: is_inf pointer is null" );
      int ncols = builder->builder.getNumCols();
      Vec<uint8_t> vals( is_inf, is_inf + ncols );
      builder->builder.setColUbInfAll( std::move( vals ) );
   }

   void
   libpapilo_problem_builder_set_col_integral(
       libpapilo_problem_builder_t* builder, int col, int is_integral )
   {
      check_problem_builder_ptr( builder );
      builder->builder.setColIntegral( col, is_integral != 0 );
   }

   void
   libpapilo_problem_builder_set_col_integral_all(
       libpapilo_problem_builder_t* builder, const uint8_t* is_integral )
   {
      check_problem_builder_ptr( builder );
      custom_assert( is_integral != nullptr,
                     "libpapilo_problem_builder_set_col_integral_all: "
                     "is_integral pointer is null" );
      int ncols = builder->builder.getNumCols();
      Vec<uint8_t> vals( is_integral, is_integral + ncols );
      builder->builder.setColIntegralAll( std::move( vals ) );
   }

   void
   libpapilo_problem_builder_set_row_lhs( libpapilo_problem_builder_t* builder,
                                          int row, double lhs )
   {
      check_problem_builder_ptr( builder );
      builder->builder.setRowLhs( row, lhs );
      if( std::isinf( lhs ) && lhs < 0 )
         builder->builder.setRowLhsInf( row, true );
      else
         builder->builder.setRowLhsInf( row, false );
   }

   void
   libpapilo_problem_builder_set_row_lhs_all(
       libpapilo_problem_builder_t* builder, const double* lhs_vals )
   {
      check_problem_builder_ptr( builder );
      custom_assert( lhs_vals != nullptr, "libpapilo_problem_builder_set_row_"
                                          "lhs_all: lhs_vals pointer is null" );
      int nrows = builder->builder.getNumRows();
      Vec<double> vals( lhs_vals, lhs_vals + nrows );
      builder->builder.setRowLhsAll( std::move( vals ) );

      // Set infinity flags
      for( int i = 0; i < nrows; ++i )
      {
         if( std::isinf( lhs_vals[i] ) && lhs_vals[i] < 0 )
            builder->builder.setRowLhsInf( i, true );
         else
            builder->builder.setRowLhsInf( i, false );
      }
   }

   void
   libpapilo_problem_builder_set_row_rhs( libpapilo_problem_builder_t* builder,
                                          int row, double rhs )
   {
      check_problem_builder_ptr( builder );
      builder->builder.setRowRhs( row, rhs );
      if( std::isinf( rhs ) && rhs > 0 )
         builder->builder.setRowRhsInf( row, true );
      else
         builder->builder.setRowRhsInf( row, false );
   }

   void
   libpapilo_problem_builder_set_row_rhs_all(
       libpapilo_problem_builder_t* builder, const double* rhs_vals )
   {
      check_problem_builder_ptr( builder );
      custom_assert( rhs_vals != nullptr, "libpapilo_problem_builder_set_row_"
                                          "rhs_all: rhs_vals pointer is null" );
      int nrows = builder->builder.getNumRows();
      Vec<double> vals( rhs_vals, rhs_vals + nrows );
      builder->builder.setRowRhsAll( std::move( vals ) );

      // Set infinity flags
      for( int i = 0; i < nrows; ++i )
      {
         if( std::isinf( rhs_vals[i] ) && rhs_vals[i] > 0 )
            builder->builder.setRowRhsInf( i, true );
         else
            builder->builder.setRowRhsInf( i, false );
      }
   }

   /* Row infinity bounds implementations */

   void
   libpapilo_problem_builder_set_row_lhs_inf_all(
       libpapilo_problem_builder_t* builder, const uint8_t* is_inf )
   {
      check_problem_builder_ptr( builder );
      custom_assert( is_inf != nullptr, "libpapilo_problem_builder_set_row_lhs_"
                                        "inf_all: is_inf pointer is null" );
      int nrows = builder->builder.getNumRows();
      Vec<uint8_t> vals( is_inf, is_inf + nrows );
      builder->builder.setRowLhsInfAll( std::move( vals ) );
   }

   void
   libpapilo_problem_builder_set_row_rhs_inf_all(
       libpapilo_problem_builder_t* builder, const uint8_t* is_inf )
   {
      check_problem_builder_ptr( builder );
      custom_assert( is_inf != nullptr, "libpapilo_problem_builder_set_row_rhs_"
                                        "inf_all: is_inf pointer is null" );
      int nrows = builder->builder.getNumRows();
      Vec<uint8_t> vals( is_inf, is_inf + nrows );
      builder->builder.setRowRhsInfAll( std::move( vals ) );
   }

   void
   libpapilo_problem_builder_add_entry( libpapilo_problem_builder_t* builder,
                                        int row, int col, double val )
   {
      check_problem_builder_ptr( builder );
      if( val != 0.0 )
      {
         builder->builder.addEntry( row, col, val );
      }
   }

   void
   libpapilo_problem_builder_add_entry_all(
       libpapilo_problem_builder_t* builder, int count, const int* rows,
       const int* cols, const double* vals )
   {
      check_problem_builder_ptr( builder );
      custom_assert(
          rows != nullptr,
          "libpapilo_problem_builder_add_entry_all: rows pointer is null" );
      custom_assert(
          cols != nullptr,
          "libpapilo_problem_builder_add_entry_all: cols pointer is null" );
      custom_assert(
          vals != nullptr,
          "libpapilo_problem_builder_add_entry_all: vals pointer is null" );
      if( count > 0 )
      {
         Vec<std::tuple<int, int, double>> entries;
         entries.reserve( count );
         for( int i = 0; i < count; ++i )
         {
            if( vals[i] != 0.0 )
               entries.emplace_back( rows[i], cols[i], vals[i] );
         }
         builder->builder.addEntryAll( std::move( entries ) );
      }
   }

   void
   libpapilo_problem_builder_add_row_entries(
       libpapilo_problem_builder_t* builder, int row, int len, const int* cols,
       const double* vals )
   {
      check_problem_builder_ptr( builder );
      custom_assert(
          cols != nullptr,
          "libpapilo_problem_builder_add_row_entries: cols pointer is null" );
      custom_assert(
          vals != nullptr,
          "libpapilo_problem_builder_add_row_entries: vals pointer is null" );
      if( len > 0 )
      {
         builder->builder.addRowEntries( row, len, cols, vals );
      }
   }

   void
   libpapilo_problem_builder_add_col_entries(
       libpapilo_problem_builder_t* builder, int col, int len, const int* rows,
       const double* vals )
   {
      check_problem_builder_ptr( builder );
      custom_assert(
          rows != nullptr,
          "libpapilo_problem_builder_add_col_entries: rows pointer is null" );
      custom_assert(
          vals != nullptr,
          "libpapilo_problem_builder_add_col_entries: vals pointer is null" );
      if( len > 0 )
      {
         builder->builder.addColEntries( col, len, rows, vals );
      }
   }

   void
   libpapilo_problem_builder_set_problem_name(
       libpapilo_problem_builder_t* builder, const char* name )
   {
      check_problem_builder_ptr( builder );
      custom_assert(
          name != nullptr,
          "libpapilo_problem_builder_set_problem_name: name pointer is null" );
      builder->builder.setProblemName( name );
   }

   void
   libpapilo_problem_builder_set_col_name( libpapilo_problem_builder_t* builder,
                                           int col, const char* name )
   {
      check_problem_builder_ptr( builder );
      custom_assert(
          name != nullptr,
          "libpapilo_problem_builder_set_col_name: name pointer is null" );
      builder->builder.setColName( col, name );
   }

   void
   libpapilo_problem_builder_set_row_name( libpapilo_problem_builder_t* builder,
                                           int row, const char* name )
   {
      check_problem_builder_ptr( builder );
      custom_assert(
          name != nullptr,
          "libpapilo_problem_builder_set_row_name: name pointer is null" );
      builder->builder.setRowName( row, name );
   }

   /* Batch name setters implementations */

   void
   libpapilo_problem_builder_set_col_name_all(
       libpapilo_problem_builder_t* builder, const char* const* names )
   {
      check_problem_builder_ptr( builder );
      custom_assert(
          names != nullptr,
          "libpapilo_problem_builder_set_col_name_all: names pointer is null" );
      int ncols = builder->builder.getNumCols();
      Vec<std::string> name_vec;
      name_vec.reserve( ncols );
      for( int i = 0; i < ncols; ++i )
      {
         if( names[i] != nullptr )
            name_vec.emplace_back( names[i] );
         else
            name_vec.emplace_back( "" );
      }
      builder->builder.setColNameAll( std::move( name_vec ) );
   }

   void
   libpapilo_problem_builder_set_row_name_all(
       libpapilo_problem_builder_t* builder, const char* const* names )
   {
      check_problem_builder_ptr( builder );
      custom_assert(
          names != nullptr,
          "libpapilo_problem_builder_set_row_name_all: names pointer is null" );
      int nrows = builder->builder.getNumRows();
      Vec<std::string> name_vec;
      name_vec.reserve( nrows );
      for( int i = 0; i < nrows; ++i )
      {
         if( names[i] != nullptr )
            name_vec.emplace_back( names[i] );
         else
            name_vec.emplace_back( "" );
      }
      builder->builder.setRowNameAll( std::move( name_vec ) );
   }

   libpapilo_problem_t*
   libpapilo_problem_builder_build( libpapilo_problem_builder_t* builder )
   {
      check_problem_builder_ptr( builder );

      return check_run(
          [builder]()
          {
             auto* problem = new libpapilo_problem_t();
             problem->problem = builder->builder.build();
             return problem;
          },
          "Failed to build problem" );
   }

   void
   libpapilo_problem_free( libpapilo_problem_t* problem )
   {
      check_problem_ptr( problem );
      delete problem;
   }

   int
   libpapilo_problem_get_nrows( const libpapilo_problem_t* problem )
   {
      check_problem_ptr( problem );
      return problem->problem.getNRows();
   }

   int
   libpapilo_problem_get_ncols( const libpapilo_problem_t* problem )
   {
      check_problem_ptr( problem );
      return problem->problem.getNCols();
   }

   int
   libpapilo_problem_get_nnz( const libpapilo_problem_t* problem )
   {
      check_problem_ptr( problem );
      return problem->problem.getConstraintMatrix().getNnz();
   }

   int
   libpapilo_problem_get_num_integral_cols( const libpapilo_problem_t* problem )
   {
      check_problem_ptr( problem );
      return problem->problem.getNumIntegralCols();
   }

   int
   libpapilo_problem_get_num_continuous_cols(
       const libpapilo_problem_t* problem )
   {
      check_problem_ptr( problem );
      return problem->problem.getNumContinuousCols();
   }

   const double*
   libpapilo_problem_get_objective_coefficients(
       const libpapilo_problem_t* problem, int* size )
   {
      check_problem_ptr( problem );
      const auto& obj = problem->problem.getObjective();
      if( size != nullptr )
         *size = obj.coefficients.size();
      return obj.coefficients.data();
   }

   double
   libpapilo_problem_get_objective_offset( const libpapilo_problem_t* problem )
   {
      check_problem_ptr( problem );
      return problem->problem.getObjective().offset;
   }

   const double*
   libpapilo_problem_get_lower_bounds( const libpapilo_problem_t* problem,
                                       int* size )
   {
      check_problem_ptr( problem );
      const auto& bounds = problem->problem.getLowerBounds();
      if( size != nullptr )
         *size = bounds.size();
      return bounds.data();
   }

   const double*
   libpapilo_problem_get_upper_bounds( const libpapilo_problem_t* problem,
                                       int* size )
   {
      check_problem_ptr( problem );
      const auto& bounds = problem->problem.getUpperBounds();
      if( size != nullptr )
         *size = bounds.size();
      return bounds.data();
   }

   const double*
   libpapilo_problem_get_row_lhs( const libpapilo_problem_t* problem,
                                  int* size )
   {
      check_problem_ptr( problem );
      const auto& lhs =
          problem->problem.getConstraintMatrix().getLeftHandSides();
      if( size != nullptr )
         *size = lhs.size();
      return lhs.data();
   }

   const double*
   libpapilo_problem_get_row_rhs( const libpapilo_problem_t* problem,
                                  int* size )
   {
      check_problem_ptr( problem );
      const auto& rhs =
          problem->problem.getConstraintMatrix().getRightHandSides();
      if( size != nullptr )
         *size = rhs.size();
      return rhs.data();
   }

   const int*
   libpapilo_problem_get_row_sizes( const libpapilo_problem_t* problem,
                                    int* size )
   {
      check_problem_ptr( problem );
      const auto& sizes = problem->problem.getRowSizes();
      if( size != nullptr )
         *size = sizes.size();
      return sizes.data();
   }

   const int*
   libpapilo_problem_get_col_sizes( const libpapilo_problem_t* problem,
                                    int* size )
   {
      check_problem_ptr( problem );
      const auto& sizes = problem->problem.getColSizes();
      if( size != nullptr )
         *size = sizes.size();
      return sizes.data();
   }

   const char*
   libpapilo_problem_get_name( const libpapilo_problem_t* problem )
   {
      check_problem_ptr( problem );
      const auto& name = problem->problem.getName();
      return name.c_str();
   }

   const char*
   libpapilo_problem_get_variable_name( const libpapilo_problem_t* problem,
                                        int col )
   {
      check_problem_ptr( problem );
      const auto& names = problem->problem.getVariableNames();
      if( col < 0 || col >= (int)names.size() )
         return nullptr;
      return names[col].c_str();
   }

   const char*
   libpapilo_problem_get_constraint_name( const libpapilo_problem_t* problem,
                                          int row )
   {
      check_problem_ptr( problem );
      const auto& names = problem->problem.getConstraintNames();
      if( row < 0 || row >= (int)names.size() )
         return nullptr;
      return names[row].c_str();
   }

   uint8_t
   libpapilo_problem_get_col_flags( const libpapilo_problem_t* problem,
                                    int col )
   {
      check_problem_ptr( problem );
      const auto& flags = problem->problem.getColFlags();
      if( col < 0 || col >= (int)flags.size() )
         return 0;

      uint8_t c_flags = 0;
      const auto& papilo_flags = flags[col];

      if( papilo_flags.test( papilo::ColFlag::kLbInf ) )
         c_flags |= LIBPAPILO_COLFLAG_LB_INF;
      if( papilo_flags.test( papilo::ColFlag::kUbInf ) )
         c_flags |= LIBPAPILO_COLFLAG_UB_INF;
      if( papilo_flags.test( papilo::ColFlag::kIntegral ) )
         c_flags |= LIBPAPILO_COLFLAG_INTEGRAL;
      if( papilo_flags.test( papilo::ColFlag::kImplInt ) )
         c_flags |= LIBPAPILO_COLFLAG_IMPLIED_INTEGRAL;
      if( papilo_flags.test( papilo::ColFlag::kFixed ) )
         c_flags |= LIBPAPILO_COLFLAG_FIXED;

      return c_flags;
   }

   uint8_t
   libpapilo_problem_get_row_flags( const libpapilo_problem_t* problem,
                                    int row )
   {
      check_problem_ptr( problem );
      const auto& flags = problem->problem.getRowFlags();
      if( row < 0 || row >= (int)flags.size() )
         return 0;

      uint8_t c_flags = 0;
      const auto& papilo_flags = flags[row];

      if( papilo_flags.test( papilo::RowFlag::kLhsInf ) )
         c_flags |= LIBPAPILO_ROWFLAG_LHS_INF;
      if( papilo_flags.test( papilo::RowFlag::kRhsInf ) )
         c_flags |= LIBPAPILO_ROWFLAG_RHS_INF;
      if( papilo_flags.test( papilo::RowFlag::kRedundant ) )
         c_flags |= LIBPAPILO_ROWFLAG_REDUNDANT;
      if( papilo_flags.test( papilo::RowFlag::kEquation ) )
         c_flags |= LIBPAPILO_ROWFLAG_EQUATION;

      return c_flags;
   }

   int
   libpapilo_problem_is_row_redundant( const libpapilo_problem_t* problem,
                                       int row )
   {
      check_problem_ptr( problem );
      const auto& flags = problem->problem.getRowFlags();
      if( row < 0 || row >= (int)flags.size() )
         return 0;

      return flags[row].test( papilo::RowFlag::kRedundant ) ? 1 : 0;
   }

   int
   libpapilo_problem_is_col_substituted( const libpapilo_problem_t* problem,
                                         int col )
   {
      check_problem_ptr( problem );
      const auto& flags = problem->problem.getColFlags();
      if( col < 0 || col >= (int)flags.size() )
         return 0;
      return flags[col].test( papilo::ColFlag::kSubstituted ) ? 1 : 0;
   }

   double*
   libpapilo_problem_get_objective_coefficients_mutable(
       libpapilo_problem_t* problem, int* size )
   {
      check_problem_ptr( problem );
      custom_assert( size != nullptr, "size pointer is null" );
      auto& coeffs = problem->problem.getObjective().coefficients;
      *size = static_cast<int>( coeffs.size() );
      return coeffs.data();
   }

   const double*
   libpapilo_problem_get_row_left_hand_sides(
       const libpapilo_problem_t* problem, int* size )
   {
      check_problem_ptr( problem );
      custom_assert( size != nullptr, "size pointer is null" );
      const auto& lhs =
          problem->problem.getConstraintMatrix().getLeftHandSides();
      *size = static_cast<int>( lhs.size() );
      return lhs.data();
   }

   const double*
   libpapilo_problem_get_row_right_hand_sides(
       const libpapilo_problem_t* problem, int* size )
   {
      check_problem_ptr( problem );
      custom_assert( size != nullptr, "size pointer is null" );
      const auto& rhs =
          problem->problem.getConstraintMatrix().getRightHandSides();
      *size = static_cast<int>( rhs.size() );
      return rhs.data();
   }

   int
   libpapilo_problem_get_row_entries( const libpapilo_problem_t* problem,
                                      int row, const int** cols,
                                      const double** vals )
   {
      check_problem_ptr( problem );
      const auto& matrix = problem->problem.getConstraintMatrix();

      if( row < 0 || row >= matrix.getNRows() )
         return -1;

      auto rowvec = matrix.getRowCoefficients( row );

      if( cols != nullptr )
         *cols = rowvec.getIndices();
      if( vals != nullptr )
         *vals = rowvec.getValues();

      return rowvec.getLength();
   }

   int
   libpapilo_problem_get_col_entries( const libpapilo_problem_t* problem,
                                      int col, const int** rows,
                                      const double** vals )
   {
      check_problem_ptr( problem );
      const auto& matrix = problem->problem.getConstraintMatrix();

      if( col < 0 || col >= matrix.getNCols() )
         return -1;

      auto colvec = matrix.getColumnCoefficients( col );

      if( rows != nullptr )
         *rows = colvec.getIndices();
      if( vals != nullptr )
         *vals = colvec.getValues();

      return colvec.getLength();
   }

   /* Phase 2: Presolve API Implementation */

   libpapilo_presolve_options_t*
   libpapilo_presolve_options_create()
   {
      return check_run( []() { return new libpapilo_presolve_options_t(); },
                        "Failed to create presolve options" );
   }

   void
   libpapilo_presolve_options_free( libpapilo_presolve_options_t* options )
   {
      check_presolve_options_ptr( options );
      delete options;
   }

   void
   libpapilo_presolve_options_set_dualreds(
       libpapilo_presolve_options_t* options, int dualreds )
   {
      check_presolve_options_ptr( options );
      options->options.dualreds = dualreds;
   }

   /* Core Presolve API Implementation */

   libpapilo_presolve_t*
   libpapilo_presolve_create()
   {
      return check_run( []() { return new libpapilo_presolve_t(); },
                        "Failed to create presolve object" );
   }

   void
   libpapilo_presolve_free( libpapilo_presolve_t* presolve )
   {
      check_presolve_ptr( presolve );
      delete presolve;
   }

   void
   libpapilo_presolve_add_default_presolvers( libpapilo_presolve_t* presolve )
   {
      check_presolve_ptr( presolve );
      presolve->presolve.addDefaultPresolvers();
   }

   void
   libpapilo_presolve_set_options( libpapilo_presolve_t* presolve,
                                   libpapilo_presolve_options_t* options )
   {
      check_presolve_ptr( presolve );
      check_presolve_options_ptr( options );
      presolve->presolve.getPresolveOptions() = options->options;
   }

   libpapilo_presolve_status_t
   libpapilo_presolve_apply_simple( libpapilo_presolve_t* presolve,
                                    libpapilo_problem_t* problem )
   {
      check_presolve_ptr( presolve );
      check_problem_ptr( problem );

      return check_run(
          [&]()
          {
             PresolveResult<double> result =
                 presolve->presolve.apply( problem->problem );
             return convert_presolve_status( result.status );
          },
          "Failed to apply presolve" );
   }

   void
   libpapilo_presolve_apply_reductions( libpapilo_presolve_t* presolve,
                                        int round,
                                        libpapilo_reductions_t* reductions,
                                        libpapilo_problem_update_t* update,
                                        int* num_rounds, int* num_changes )
   {
      check_presolve_ptr( presolve );
      check_reductions_ptr( reductions );
      check_problem_update_ptr( update );
      custom_assert( num_rounds != nullptr, "num_rounds pointer is null" );
      custom_assert( num_changes != nullptr, "num_changes pointer is null" );

      check_run(
          [&]()
          {
             std::pair<int, int> result = presolve->presolve.applyReductions(
                 round, reductions->reductions, update->update );
             *num_rounds = result.first;
             *num_changes = result.second;
          },
          "Failed to apply reductions" );
   }

   /* High-level presolve function for backward compatibility */
   libpapilo_presolve_status_t
   libpapilo_presolve_apply( libpapilo_problem_t* problem,
                             libpapilo_presolve_options_t* options,
                             libpapilo_reductions_t** reductions_out,
                             libpapilo_postsolve_storage_t** postsolve_out,
                             libpapilo_statistics_t** statistics_out )
   {
      check_problem_ptr( problem );
      check_presolve_options_ptr( options );
      custom_assert( reductions_out != nullptr,
                     "reductions_out pointer is null" );
      custom_assert( postsolve_out != nullptr,
                     "postsolve_out pointer is null" );
      custom_assert( statistics_out != nullptr,
                     "statistics_out pointer is null" );

      return check_run(
          [&]()
          {
             // Create presolve object
             auto* presolve = libpapilo_presolve_create();
             libpapilo_presolve_add_default_presolvers( presolve );
             libpapilo_presolve_set_options( presolve, options );

             // Execute presolve
             PresolveResult<double> result =
                 presolve->presolve.apply( problem->problem );

             // Create output objects
             auto* postsolve_storage = new libpapilo_postsolve_storage_t(
                 std::move( result.postsolve ) );
             auto* reductions = new libpapilo_reductions_t();
             auto* stats = new libpapilo_statistics_t();

             // Set output parameters
             *reductions_out = reductions;
             *postsolve_out = postsolve_storage;
             *statistics_out = stats;

             // Clean up presolve object
             libpapilo_presolve_free( presolve );

             // Convert status
             return convert_presolve_status( result.status );
          },
          "Failed to apply presolve" );
   }

   libpapilo_reductions_t*
   libpapilo_reductions_create()
   {
      return check_run( []() { return new libpapilo_reductions_t(); },
                        "Failed to create reductions object" );
   }

   int
   libpapilo_reductions_get_size( libpapilo_reductions_t* reductions )
   {
      check_reductions_ptr( reductions );
      return static_cast<int>( reductions->reductions.size() );
   }

   libpapilo_reduction_info_t
   libpapilo_reductions_get_info( libpapilo_reductions_t* reductions,
                                  int index )
   {
      check_reductions_ptr( reductions );
      custom_assert( index >= 0 && index < (int)reductions->reductions.size(),
                     "Reduction index out of bounds" );

      const auto& reduction = reductions->reductions.getReduction( index );
      libpapilo_reduction_info_t info;
      info.row = reduction.row;
      info.col = reduction.col;
      info.newval = reduction.newval;
      return info;
   }

   void
   libpapilo_reductions_free( libpapilo_reductions_t* reductions )
   {
      check_reductions_ptr( reductions );
      delete reductions;
   }

   /* Reductions manipulation API Implementation */

   void
   libpapilo_reductions_replace_col( libpapilo_reductions_t* reductions,
                                     int col, int replace_col, double scale,
                                     double offset )
   {
      check_reductions_ptr( reductions );
      check_run(
          // clang-format off
          [&]() {
             reductions->reductions.replaceCol( col, replace_col, scale,
                                                offset );
          },
          // clang-format on
          "Failed to replace column in reductions" );
   }

   void
   libpapilo_reductions_lock_col_bounds( libpapilo_reductions_t* reductions,
                                         int col )
   {
      check_reductions_ptr( reductions );
      check_run( [&]() { reductions->reductions.lockColBounds( col ); },
                 "Failed to lock column bounds in reductions" );
   }

   void
   libpapilo_reductions_lock_row( libpapilo_reductions_t* reductions, int row )
   {
      check_reductions_ptr( reductions );
      check_run( [&]() { reductions->reductions.lockRow( row ); },
                 "Failed to lock row in reductions" );
   }

   void
   libpapilo_reductions_substitute_col_in_objective(
       libpapilo_reductions_t* reductions, int col, int row )
   {
      check_reductions_ptr( reductions );
      check_run(
          [&]()
          { reductions->reductions.substituteColInObjective( col, row ); },
          "Failed to substitute column in objective" );
   }

   void
   libpapilo_reductions_mark_row_redundant( libpapilo_reductions_t* reductions,
                                            int row )
   {
      check_reductions_ptr( reductions );
      check_run( [&]() { reductions->reductions.markRowRedundant( row ); },
                 "Failed to mark row redundant in reductions" );
   }

   void
   libpapilo_reductions_aggregate_free_col( libpapilo_reductions_t* reductions,
                                            int col, int row )
   {
      check_reductions_ptr( reductions );
      check_run( [&]() { reductions->reductions.aggregateFreeCol( col, row ); },
                 "Failed to aggregate free column in reductions" );
   }

   void
   libpapilo_reductions_begin_transaction( libpapilo_reductions_t* reductions )
   {
      check_reductions_ptr( reductions );
      check_run(
          [&]()
          {
             // Create a new transaction guard
             reductions->transaction_guard =
                 std::make_unique<TransactionGuard<double>>(
                     reductions->reductions );
          },
          "Failed to begin transaction" );
   }

   void
   libpapilo_reductions_end_transaction( libpapilo_reductions_t* reductions )
   {
      check_reductions_ptr( reductions );
      check_run(
          [&]()
          {
             // Release the transaction guard, which commits the transaction
             reductions->transaction_guard.reset();
          },
          "Failed to end transaction" );
   }

   /* PostsolveStorage management implementation */

   libpapilo_postsolve_storage_t*
   libpapilo_postsolve_storage_create( const libpapilo_problem_t* problem,
                                       const libpapilo_num_t* num,
                                       const libpapilo_presolve_options_t* options )
   {
      check_problem_ptr( problem );
      check_num_ptr( num );
      check_presolve_options_ptr( options );

      return check_run(
          [&]()
          {
             PostsolveStorage<double> postsolve( problem->problem, num->num,
                                                 options->options );
             return new libpapilo_postsolve_storage_t( std::move( postsolve ) );
          },
          "Failed to create postsolve storage" );
   }

   void
   libpapilo_postsolve_storage_free( libpapilo_postsolve_storage_t* postsolve )
   {
      check_postsolve_storage_ptr( postsolve );
      delete postsolve;
   }

   libpapilo_statistics_t*
   libpapilo_statistics_create()
   {
      return check_run( []() { return new libpapilo_statistics_t(); },
                        "Failed to create statistics object" );
   }

   void
   libpapilo_statistics_free( libpapilo_statistics_t* statistics )
   {
      check_statistics_ptr( statistics );
      delete statistics;
   }

   /* Problem Modification API Implementation */

   void
   libpapilo_problem_modify_row_lhs( libpapilo_problem_t* problem, int row,
                                     double lhs )
   {
      check_problem_ptr( problem );
      const Num<double> num{};
      problem->problem.getConstraintMatrix().modifyLeftHandSide( row, num,
                                                                 lhs );
   }

   void
   libpapilo_problem_recompute_locks( libpapilo_problem_t* problem )
   {
      check_problem_ptr( problem );
      problem->problem.recomputeLocks();
   }

   void
   libpapilo_problem_recompute_activities( libpapilo_problem_t* problem )
   {
      check_problem_ptr( problem );
      problem->problem.recomputeAllActivities();
   }

   void
   libpapilo_problem_recompute_all_activities( libpapilo_problem_t* problem )
   {
      check_problem_ptr( problem );
      problem->problem.recomputeAllActivities();
   }

   /* Utility Objects API Implementation */

   libpapilo_num_t*
   libpapilo_num_create()
   {
      return check_run( []() { return new libpapilo_num_t(); },
                        "Failed to create num object" );
   }

   void
   libpapilo_num_free( libpapilo_num_t* num )
   {
      check_num_ptr( num );
      delete num;
   }

   libpapilo_timer_t*
   libpapilo_timer_create( double* time_ref )
   {
      if( time_ref == nullptr )
      {
         std::cerr << "Error: time_ref cannot be NULL for timer creation"
                   << std::endl;
         std::terminate();
      }

      return check_run( [time_ref]()
                        { return new libpapilo_timer_t( *time_ref ); },
                        "Failed to create timer object" );
   }

   void
   libpapilo_timer_free( libpapilo_timer_t* timer )
   {
      check_timer_ptr( timer );
      delete timer;
   }

   libpapilo_message_t*
   libpapilo_message_create()
   {
      return check_run( []() { return new libpapilo_message_t(); },
                        "Failed to create message object" );
   }

   void
   libpapilo_message_free( libpapilo_message_t* message )
   {
      check_message_ptr( message );
      delete message;
   }

   /* ProblemUpdate Control API Implementation */

   libpapilo_problem_update_t*
   libpapilo_problem_update_create( libpapilo_problem_t* problem,
                                    libpapilo_postsolve_storage_t* postsolve,
                                    libpapilo_statistics_t* statistics,
                                    const libpapilo_presolve_options_t* options,
                                    const libpapilo_num_t* num,
                                    const libpapilo_message_t* message )
   {
      check_problem_ptr( problem );
      check_postsolve_storage_ptr( postsolve );
      check_statistics_ptr( statistics );
      check_presolve_options_ptr( options );
      check_num_ptr( num );
      check_message_ptr( message );

      return check_run(
          [&]()
          {
             return new libpapilo_problem_update_t(
                 problem->problem, postsolve->postsolve, statistics->statistics,
                 options->options, num->num, message->message );
          },
          "Failed to create problem update" );
   }

   void
   libpapilo_problem_update_free( libpapilo_problem_update_t* update )
   {
      check_problem_update_ptr( update );
      delete update;
   }

   libpapilo_presolve_status_t
   libpapilo_problem_update_trivial_column_presolve(
       libpapilo_problem_update_t* update )
   {
      check_problem_update_ptr( update );

      return check_run(
          [&]()
          {
             PresolveStatus status = update->update.trivialColumnPresolve();
             return convert_presolve_status( status );
          },
          "Failed to execute trivial column presolve" );
   }

   libpapilo_presolve_status_t
   libpapilo_problem_update_trivial_presolve(
       libpapilo_problem_update_t* update )
   {
      check_problem_update_ptr( update );

      return check_run(
          // clang-format off
          [&]() {
             PresolveStatus status = update->update.trivialPresolve();
             return convert_presolve_status( status );
          },
          // clang-format on
          "Failed to execute trivial presolve" );
   }

   int
   libpapilo_problem_update_get_singleton_cols_count(
       libpapilo_problem_update_t* update )
   {
      check_problem_update_ptr( update );

      return check_run(
          // clang-format off
          [&]() {
             return static_cast<int>(
                 update->update.getSingletonCols().size() );
          },
          // clang-format on
          "Failed to get singleton columns count" );
   }

   libpapilo_reductions_t*
   libpapilo_problem_update_get_reductions( libpapilo_problem_update_t* update )
   {
      check_problem_update_ptr( update );
      // Note: ProblemUpdate doesn't store reductions directly.
      // Reductions are accumulated by presolver.execute() calls.
      // This function creates an empty reductions object for now.
      return check_run(
          [&]()
          {
             auto* reductions = new libpapilo_reductions_t();
             // reductions->reductions remains empty
             return reductions;
          },
          "Failed to create reductions object" );
   }

   void
   libpapilo_problem_update_set_postpone_substitutions(
       libpapilo_problem_update_t* update, int postpone )
   {
      check_problem_update_ptr( update );
      check_run( [&]() { update->update.setPostponeSubstitutions( postpone ); },
                 "Failed to set postpone substitutions" );
   }

   /* Individual Presolver API Implementation */

   libpapilo_singleton_cols_t*
   libpapilo_singleton_cols_create()
   {
      return check_run( []() { return new libpapilo_singleton_cols_t(); },
                        "Failed to create singleton cols presolver" );
   }

   void
   libpapilo_singleton_cols_free( libpapilo_singleton_cols_t* presolver )
   {
      check_singleton_cols_ptr( presolver );
      delete presolver;
   }

   libpapilo_presolve_status_t
   libpapilo_singleton_cols_execute( libpapilo_singleton_cols_t* presolver,
                                     const libpapilo_problem_t* problem,
                                     const libpapilo_problem_update_t* update,
                                     const libpapilo_num_t* num,
                                     libpapilo_reductions_t* reductions,
                                     const libpapilo_timer_t* timer, int* cause )
   {
      check_singleton_cols_ptr( presolver );
      check_problem_ptr( problem );
      check_problem_update_ptr( update );
      check_num_ptr( num );
      check_reductions_ptr( reductions );
      check_timer_ptr( timer );
      custom_assert( cause != nullptr, "cause pointer is null" );

      return check_run(
          [&]()
          {
             PresolveStatus status = presolver->presolver.execute(
                 problem->problem, update->update, num->num,
                 reductions->reductions, timer->timer, *cause );
             return convert_presolve_status( status );
          },
          "Failed to execute singleton cols presolver" );
   }

   /* SimpleSubstitution Presolver API Implementation */

   libpapilo_simple_substitution_t*
   libpapilo_simple_substitution_create()
   {
      return check_run( []() { return new libpapilo_simple_substitution_t(); },
                        "Failed to create simple substitution presolver" );
   }

   void
   libpapilo_simple_substitution_free(
       libpapilo_simple_substitution_t* presolver )
   {
      check_simple_substitution_ptr( presolver );
      delete presolver;
   }

   libpapilo_presolve_status_t
   libpapilo_simple_substitution_execute(
       libpapilo_simple_substitution_t* presolver, const libpapilo_problem_t* problem,
       const libpapilo_problem_update_t* update, const libpapilo_num_t* num,
       libpapilo_reductions_t* reductions, const libpapilo_timer_t* timer,
       int* cause )
   {
      check_simple_substitution_ptr( presolver );
      check_problem_ptr( problem );
      check_problem_update_ptr( update );
      check_num_ptr( num );
      check_reductions_ptr( reductions );
      check_timer_ptr( timer );
      custom_assert( cause != nullptr, "cause pointer is null" );

      return check_run(
          [&]()
          {
             PresolveStatus status = presolver->presolver.execute(
                 problem->problem, update->update, num->num,
                 reductions->reductions, timer->timer, *cause );
             return convert_presolve_status( status );
          },
          "Failed to execute simple substitution presolver" );
   }

   /* Solution Management API Implementation */

   libpapilo_solution_t*
   libpapilo_solution_create()
   {
      return check_run( []() { return new libpapilo_solution_t(); },
                        "Failed to create solution object" );
   }

   void
   libpapilo_solution_free( libpapilo_solution_t* solution )
   {
      check_solution_ptr( solution );
      delete solution;
   }

   const double*
   libpapilo_solution_get_primal( const libpapilo_solution_t* solution,
                                  int* size )
   {
      check_solution_ptr( solution );
      custom_assert( size != nullptr, "size pointer is null" );

      *size = static_cast<int>( solution->solution.primal.size() );
      return solution->solution.primal.data();
   }

   void
   libpapilo_solution_set_primal( libpapilo_solution_t* solution,
                                  const double* values, int size )
   {
      check_solution_ptr( solution );
      custom_assert( values != nullptr || size == 0,
                     "values pointer is null for non-zero size" );
      custom_assert( size >= 0, "size cannot be negative" );

      solution->solution.primal.clear();
      solution->solution.primal.reserve( size );
      for( int i = 0; i < size; ++i )
      {
         solution->solution.primal.push_back( values[i] );
      }
   }

   /* Postsolve Engine API Implementation */

   libpapilo_postsolve_t*
   libpapilo_postsolve_create( const libpapilo_message_t* message,
                               const libpapilo_num_t* num )
   {
      check_message_ptr( message );
      check_num_ptr( num );

      return check_run(
          [&]()
          { return new libpapilo_postsolve_t( message->message, num->num ); },
          "Failed to create postsolve object" );
   }

   void
   libpapilo_postsolve_free( libpapilo_postsolve_t* postsolve )
   {
      check_postsolve_ptr( postsolve );
      delete postsolve;
   }

   libpapilo_postsolve_status_t
   libpapilo_postsolve_undo( libpapilo_postsolve_t* postsolve,
                             const libpapilo_solution_t* reduced_solution,
                             libpapilo_solution_t* original_solution,
                             const libpapilo_postsolve_storage_t* storage )
   {
      check_postsolve_ptr( postsolve );
      check_solution_ptr( reduced_solution );
      check_solution_ptr( original_solution );
      check_postsolve_storage_ptr( storage );

      return check_run(
          [&]()
          {
             PostsolveStatus status = postsolve->postsolve.undo(
                 reduced_solution->solution, original_solution->solution,
                 storage->postsolve );
             return convert_postsolve_status( status );
          },
          "Failed to perform postsolve operation" );
   }

   /* PostsolveStorage File I/O API Implementation */

   libpapilo_postsolve_storage_t*
   libpapilo_postsolve_storage_load_from_file( const char* filename )
   {
      custom_assert( filename != nullptr, "filename pointer is null" );

      return check_run(
          [&]()
          {
             libpapilo_postsolve_storage_t* storage =
                 new libpapilo_postsolve_storage_t();
             std::ifstream inArchiveFile( filename, std::ios_base::binary );
             if( !inArchiveFile.is_open() )
             {
                delete storage;
                custom_assert( false, "Failed to open file for reading" );
             }
             boost::archive::binary_iarchive inputArchive( inArchiveFile );
             inputArchive >> storage->postsolve;
             inArchiveFile.close();
             return storage;
          },
          "Failed to load PostsolveStorage from file" );
   }

} // extern "C"
