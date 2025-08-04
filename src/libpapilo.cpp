#include "libpapilo.h"

#include "papilo/core/Presolve.hpp"
#include "papilo/core/Problem.hpp"
#include "papilo/core/ProblemBuilder.hpp"
#include "papilo/misc/Vec.hpp"

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

void
check_problem_builder_ptr( const libpapilo_problem_builder_t* builder )
{
   if( builder == nullptr )
   {
      std::cerr
          << "libpapilo error: libpapilo_problem_builder_t pointer is null"
          << std::endl;
      std::terminate();
   }
   if( builder->magic_number != LIBPAPILO_MAGIC_NUMBER )
   {
      std::cerr << "libpapilo error: Invalid libpapilo_problem_builder_t "
                   "pointer (magic number mismatch)"
                << std::endl;
      std::terminate();
   }
}

void
check_problem_ptr( const libpapilo_problem_t* problem )
{
   if( problem == nullptr )
   {
      std::cerr << "libpapilo error: libpapilo_problem_t pointer is null"
                << std::endl;
      std::terminate();
   }
   if( problem->magic_number != LIBPAPILO_MAGIC_NUMBER )
   {
      std::cerr << "libpapilo error: Invalid libpapilo_problem_t pointer "
                   "(magic number mismatch)"
                << std::endl;
      std::terminate();
   }
}

extern "C"
{

   libpapilo_problem_builder_t*
   libpapilo_problem_builder_create()
   {
      try
      {
         return new libpapilo_problem_builder_t();
      }
      catch( const std::exception& e )
      {
         std::cerr << "libpapilo error: Failed to create problem builder: "
                   << e.what() << std::endl;
         return nullptr;
      }
      catch( ... )
      {
         std::cerr << "libpapilo error: Failed to create problem builder: "
                      "Unknown exception"
                   << std::endl;
         return nullptr;
      }
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
      if( values )
      {
         int ncols = builder->builder.getNumCols();
         Vec<double> vals( values, values + ncols );
         builder->builder.setObjAll( std::move( vals ) );
      }
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
      if( lbs )
      {
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
   }

   void
   libpapilo_problem_builder_set_col_ub( libpapilo_problem_builder_t* builder,
                                         int col, double ub )
   {
      if( builder && builder->magic_number == LIBPAPILO_MAGIC_NUMBER )
      {
         builder->builder.setColUb( col, ub );
         if( std::isinf( ub ) && ub > 0 )
            builder->builder.setColUbInf( col, true );
         else
            builder->builder.setColUbInf( col, false );
      }
   }

   void
   libpapilo_problem_builder_set_col_ub_all(
       libpapilo_problem_builder_t* builder, const double* ubs )
   {
      check_problem_builder_ptr( builder );
      if( ubs )
      {
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
      if( is_integral )
      {
         int ncols = builder->builder.getNumCols();
         Vec<uint8_t> vals( is_integral, is_integral + ncols );
         builder->builder.setColIntegralAll( std::move( vals ) );
      }
   }

   void
   libpapilo_problem_builder_set_row_lhs( libpapilo_problem_builder_t* builder,
                                          int row, double lhs )
   {
      if( builder && builder->magic_number == LIBPAPILO_MAGIC_NUMBER )
      {
         builder->builder.setRowLhs( row, lhs );
         if( std::isinf( lhs ) && lhs < 0 )
            builder->builder.setRowLhsInf( row, true );
         else
            builder->builder.setRowLhsInf( row, false );
      }
   }

   void
   libpapilo_problem_builder_set_row_lhs_all(
       libpapilo_problem_builder_t* builder, const double* lhs_vals )
   {
      check_problem_builder_ptr( builder );
      if( lhs_vals )
      {
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
   }

   void
   libpapilo_problem_builder_set_row_rhs( libpapilo_problem_builder_t* builder,
                                          int row, double rhs )
   {
      if( builder && builder->magic_number == LIBPAPILO_MAGIC_NUMBER )
      {
         builder->builder.setRowRhs( row, rhs );
         if( std::isinf( rhs ) && rhs > 0 )
            builder->builder.setRowRhsInf( row, true );
         else
            builder->builder.setRowRhsInf( row, false );
      }
   }

   void
   libpapilo_problem_builder_set_row_rhs_all(
       libpapilo_problem_builder_t* builder, const double* rhs_vals )
   {
      check_problem_builder_ptr( builder );
      if( rhs_vals )
      {
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
      if( rows && cols && vals && count > 0 )
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
      if( cols && vals && len > 0 )
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
      if( rows && vals && len > 0 )
      {
         builder->builder.addColEntries( col, len, rows, vals );
      }
   }

   void
   libpapilo_problem_builder_set_problem_name(
       libpapilo_problem_builder_t* builder, const char* name )
   {
      check_problem_builder_ptr( builder );
      if( name )
      {
         builder->builder.setProblemName( name );
      }
   }

   void
   libpapilo_problem_builder_set_col_name( libpapilo_problem_builder_t* builder,
                                           int col, const char* name )
   {
      check_problem_builder_ptr( builder );
      if( name )
      {
         builder->builder.setColName( col, name );
      }
   }

   void
   libpapilo_problem_builder_set_row_name( libpapilo_problem_builder_t* builder,
                                           int row, const char* name )
   {
      check_problem_builder_ptr( builder );
      if( name )
      {
         builder->builder.setRowName( row, name );
      }
   }

   libpapilo_problem_t*
   libpapilo_problem_builder_build( libpapilo_problem_builder_t* builder )
   {
      check_problem_builder_ptr( builder );

      try
      {
         auto* problem = new libpapilo_problem_t();
         problem->problem = builder->builder.build();
         return problem;
      }
      catch( const std::exception& e )
      {
         std::cerr << "libpapilo error: Failed to build problem: " << e.what()
                   << std::endl;
         return nullptr;
      }
      catch( ... )
      {
         std::cerr
             << "libpapilo error: Failed to build problem: Unknown exception"
             << std::endl;
         return nullptr;
      }
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

} // extern "C"
