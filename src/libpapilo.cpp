#include "libpapilo.h"

#include "papilo/core/Presolve.hpp"
#include "papilo/core/PresolveOptions.hpp"
#include "papilo/core/Problem.hpp"
#include "papilo/core/ProblemBuilder.hpp"
#include "papilo/core/ProblemUpdate.hpp"
#include "papilo/core/Reductions.hpp"
#include "papilo/core/Statistics.hpp"
#include "papilo/core/postsolve/PostsolveStorage.hpp"
#include "papilo/io/Message.hpp"
#include "papilo/misc/Num.hpp"
#include "papilo/misc/Timer.hpp"
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
                               const Num<double>& num, Message& msg )
       : update( problem, postsolve, stats, options, num, msg )
   {
   }
};

struct libpapilo_reductions_t
{
   uint64_t magic_number = LIBPAPILO_MAGIC_NUMBER;
   Reductions<double> reductions;
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

extern "C"
{

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
             // Create required objects
             Num<double> num{};
             Message msg{};
             double time = 0.0;
             Timer timer{ time };

             // Create statistics
             auto* stats = new libpapilo_statistics_t();

             // Create presolve instance
             Presolve<double> presolve;
             presolve.addDefaultPresolvers();
             presolve.getPresolveOptions() = options->options;

             // Execute presolve
             PresolveResult<double> result = presolve.apply( problem->problem );

             // Create postsolve storage using constructor
             auto* postsolve_storage = new libpapilo_postsolve_storage_t(
                 std::move( result.postsolve ) );

             // Create reductions - we need to extract them from the postsolve
             // storage
             auto* reductions = new libpapilo_reductions_t();
             // Note: Reductions are stored inside PostsolveStorage, not
             // directly accessible For now, we'll leave it empty - this needs
             // further investigation

             // Set output parameters
             *reductions_out = reductions;
             *postsolve_out = postsolve_storage;
             *statistics_out = stats;

             // Convert PresolveStatus to C enum
             switch( result.status )
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
          },
          "Failed to apply presolve" );
   }

   int
   libpapilo_reductions_get_size( const libpapilo_reductions_t* reductions )
   {
      check_reductions_ptr( reductions );
      // Cast away const as size() is not const in the C++ API
      auto* mutable_reductions =
          const_cast<libpapilo_reductions_t*>( reductions );
      return static_cast<int>( mutable_reductions->reductions.size() );
   }

   libpapilo_reduction_info_t
   libpapilo_reductions_get_info( const libpapilo_reductions_t* reductions,
                                  int index )
   {
      check_reductions_ptr( reductions );
      // Cast away const as size() and getReduction() are not const in the C++
      // API
      auto* mutable_reductions =
          const_cast<libpapilo_reductions_t*>( reductions );
      custom_assert( index >= 0 &&
                         index < (int)mutable_reductions->reductions.size(),
                     "Reduction index out of bounds" );

      const auto& reduction =
          mutable_reductions->reductions.getReduction( index );
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

   void
   libpapilo_postsolve_storage_free( libpapilo_postsolve_storage_t* postsolve )
   {
      check_postsolve_storage_ptr( postsolve );
      delete postsolve;
   }

   void
   libpapilo_statistics_free( libpapilo_statistics_t* statistics )
   {
      check_statistics_ptr( statistics );
      delete statistics;
   }

} // extern "C"
