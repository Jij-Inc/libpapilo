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

struct papilo_t
{
   std::unique_ptr<ProblemBuilder<double>> builder;
   std::unique_ptr<Problem<double>> problem;
   PresolveOptions options;
   int nrows;
   int ncols;
   int nnz;
   bool problem_built;

   papilo_t() : nrows( 0 ), ncols( 0 ), nnz( 0 ), problem_built( false )
   {
      options = PresolveOptions();
   }
};

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

extern "C"
{

   libpapilo_problem_builder_t*
   libpapilo_problem_builder_create()
   {
      try
      {
         return new libpapilo_problem_builder_t();
      }
      catch( ... )
      {
         return nullptr;
      }
   }

   void
   libpapilo_problem_builder_free( libpapilo_problem_builder_t* builder )
   {
      if( builder && builder->magic_number == LIBPAPILO_MAGIC_NUMBER )
      {
         delete builder;
      }
   }

   papilo_t*
   papilo_create( void )
   {
      try
      {
         return new papilo_t();
      }
      catch( ... )
      {
         return nullptr;
      }
   }

   void
   papilo_free( papilo_t* papilo )
   {
      delete papilo;
   }

   int
   papilo_set_problem_dimensions( papilo_t* papilo, int nrows, int ncols,
                                  int nnz )
   {
      if( !papilo || nrows < 0 || ncols < 0 || nnz < 0 )
      {
         return PAPILO_ERROR_INVALID_PARAMETER;
      }

      if( papilo->problem_built )
      {
         return PAPILO_ERROR_INVALID_STATE;
      }

      try
      {
         papilo->nrows = nrows;
         papilo->ncols = ncols;
         papilo->nnz = nnz;
         papilo->builder = std::make_unique<ProblemBuilder<double>>();
         papilo->builder->reserve( nnz, nrows, ncols );

         // Set the number of rows and columns
         papilo->builder->setNumRows( nrows );
         papilo->builder->setNumCols( ncols );

         // Initialize objective coefficients to zero
         for( int i = 0; i < ncols; ++i )
         {
            papilo->builder->setObj( i, 0.0 );
         }

         // Initialize bounds to default values
         for( int i = 0; i < ncols; ++i )
         {
            papilo->builder->setColLb( i, 0.0 );
            papilo->builder->setColUb(
                i, std::numeric_limits<double>::infinity() );
         }

         for( int i = 0; i < nrows; ++i )
         {
            papilo->builder->setRowLhs(
                i, -std::numeric_limits<double>::infinity() );
            papilo->builder->setRowRhs(
                i, std::numeric_limits<double>::infinity() );
         }

         return PAPILO_OK;
      }
      catch( ... )
      {
         return PAPILO_ERROR_OUT_OF_MEMORY;
      }
   }

   int
   papilo_set_objective( papilo_t* papilo, const double* coefficients,
                         double offset )
   {
      if( !papilo || !coefficients )
      {
         return PAPILO_ERROR_INVALID_PARAMETER;
      }

      if( papilo->problem_built )
      {
         return PAPILO_ERROR_INVALID_STATE;
      }

      if( !papilo->builder )
      {
         return PAPILO_ERROR_INVALID_PARAMETER;
      }

      try
      {
         // Set individual coefficients instead of using setObjAll
         for( int i = 0; i < papilo->ncols; ++i )
         {
            papilo->builder->setObj( i, coefficients[i] );
         }
         papilo->builder->setObjOffset( offset );
         return PAPILO_OK;
      }
      catch( ... )
      {
         return PAPILO_ERROR_OUT_OF_MEMORY;
      }
   }

   int
   papilo_set_col_bounds( papilo_t* papilo, int col, double lb, double ub )
   {
      if( !papilo )
      {
         return PAPILO_ERROR_INVALID_PARAMETER;
      }

      if( papilo->problem_built )
      {
         return PAPILO_ERROR_INVALID_STATE;
      }

      if( !papilo->builder || col < 0 || col >= papilo->ncols )
      {
         return PAPILO_ERROR_INVALID_PARAMETER;
      }

      try
      {
         papilo->builder->setColLb( col, lb );
         papilo->builder->setColUb( col, ub );
         return PAPILO_OK;
      }
      catch( ... )
      {
         return PAPILO_ERROR_OUT_OF_MEMORY;
      }
   }

   int
   papilo_set_col_bounds_all( papilo_t* papilo, const double* lb,
                              const double* ub )
   {
      if( !papilo || !papilo->builder || !lb || !ub )
      {
         return PAPILO_ERROR_INVALID_PARAMETER;
      }

      if( papilo->problem_built )
      {
         return PAPILO_ERROR_INVALID_STATE;
      }

      try
      {
         for( int i = 0; i < papilo->ncols; ++i )
         {
            papilo->builder->setColLb( i, lb[i] );
            papilo->builder->setColUb( i, ub[i] );
         }
         return PAPILO_OK;
      }
      catch( ... )
      {
         return PAPILO_ERROR_OUT_OF_MEMORY;
      }
   }

   int
   papilo_set_row_bounds( papilo_t* papilo, int row, double lhs, double rhs )
   {
      if( !papilo )
      {
         return PAPILO_ERROR_INVALID_PARAMETER;
      }

      if( papilo->problem_built )
      {
         return PAPILO_ERROR_INVALID_STATE;
      }

      if( !papilo->builder || row < 0 || row >= papilo->nrows )
      {
         return PAPILO_ERROR_INVALID_PARAMETER;
      }

      try
      {
         papilo->builder->setRowLhs( row, lhs );
         papilo->builder->setRowRhs( row, rhs );
         return PAPILO_OK;
      }
      catch( ... )
      {
         return PAPILO_ERROR_OUT_OF_MEMORY;
      }
   }

   int
   papilo_set_row_bounds_all( papilo_t* papilo, const double* lhs,
                              const double* rhs )
   {
      if( !papilo || !papilo->builder || !lhs || !rhs )
      {
         return PAPILO_ERROR_INVALID_PARAMETER;
      }

      if( papilo->problem_built )
      {
         return PAPILO_ERROR_INVALID_STATE;
      }

      try
      {
         for( int i = 0; i < papilo->nrows; ++i )
         {
            papilo->builder->setRowLhs( i, lhs[i] );
            papilo->builder->setRowRhs( i, rhs[i] );
         }
         return PAPILO_OK;
      }
      catch( ... )
      {
         return PAPILO_ERROR_OUT_OF_MEMORY;
      }
   }

   int
   papilo_add_entry( papilo_t* papilo, int row, int col, double value )
   {
      if( !papilo || row < 0 || row >= papilo->nrows || col < 0 ||
          col >= papilo->ncols )
      {
         return PAPILO_ERROR_INVALID_PARAMETER;
      }

      if( papilo->problem_built )
      {
         return PAPILO_ERROR_INVALID_STATE;
      }

      if( !papilo->builder )
      {
         return PAPILO_ERROR_INVALID_PARAMETER;
      }

      try
      {
         papilo->builder->addEntry( row, col, value );
         return PAPILO_OK;
      }
      catch( ... )
      {
         return PAPILO_ERROR_OUT_OF_MEMORY;
      }
   }

   int
   papilo_add_entries( papilo_t* papilo, int count, const int* rows,
                       const int* cols, const double* values )
   {
      if( !papilo || !papilo->builder || count < 0 || !rows || !cols ||
          !values )
      {
         return PAPILO_ERROR_INVALID_PARAMETER;
      }

      if( papilo->problem_built )
      {
         return PAPILO_ERROR_INVALID_STATE;
      }

      try
      {
         Vec<Triplet<double>> triplets;
         triplets.reserve( count );
         for( int i = 0; i < count; ++i )
         {
            if( rows[i] < 0 || rows[i] >= papilo->nrows || cols[i] < 0 ||
                cols[i] >= papilo->ncols )
            {
               return PAPILO_ERROR_INVALID_PARAMETER;
            }
            triplets.emplace_back( rows[i], cols[i], values[i] );
         }
         papilo->builder->addEntryAll( triplets );
         return PAPILO_OK;
      }
      catch( ... )
      {
         return PAPILO_ERROR_OUT_OF_MEMORY;
      }
   }

   int
   papilo_build_problem( papilo_t* papilo )
   {
      if( !papilo || !papilo->builder )
      {
         return PAPILO_ERROR_INVALID_PARAMETER;
      }

      if( papilo->problem_built )
      {
         return PAPILO_ERROR_INVALID_STATE;
      }

      try
      {
         papilo->problem =
             std::make_unique<Problem<double>>( papilo->builder->build() );
         papilo->problem_built = true;

         // Debug: verify problem dimensions after build (comment out for clean
         // output) std::cerr << "After build - rows: " <<
         // papilo->problem->getNRows()
         //           << ", cols: " << papilo->problem->getNCols()
         //           << ", nnz: " <<
         //           papilo->problem->getConstraintMatrix().getNnz() <<
         //           std::endl;

         papilo->builder.reset(); // Free builder memory
         return PAPILO_OK;
      }
      catch( ... )
      {
         return PAPILO_ERROR_OUT_OF_MEMORY;
      }
   }

   int
   papilo_get_nrows( const papilo_t* papilo )
   {
      if( !papilo || !papilo->problem || !papilo->problem_built )
      {
         return -1;
      }
      return papilo->problem->getNRows();
   }

   int
   papilo_get_ncols( const papilo_t* papilo )
   {
      if( !papilo || !papilo->problem || !papilo->problem_built )
      {
         return -1;
      }
      return papilo->problem->getNCols();
   }

   int
   papilo_get_nnz( const papilo_t* papilo )
   {
      if( !papilo || !papilo->problem || !papilo->problem_built )
      {
         return -1;
      }
      return papilo->problem->getConstraintMatrix().getNnz();
   }

   int
   papilo_get_objective( const papilo_t* papilo, double* coefficients,
                         double* offset )
   {
      if( !papilo || !papilo->problem || !papilo->problem_built )
      {
         return PAPILO_ERROR_INVALID_PARAMETER;
      }

      const auto& obj = papilo->problem->getObjective();
      if( coefficients )
      {
         const auto& coeffs = obj.coefficients;
         std::copy( coeffs.begin(), coeffs.end(), coefficients );
      }

      if( offset )
      {
         *offset = obj.offset;
      }

      return PAPILO_OK;
   }

   int
   papilo_get_col_bounds( const papilo_t* papilo, int col, double* lb,
                          double* ub )
   {
      if( !papilo || !papilo->problem || !papilo->problem_built || col < 0 ||
          col >= papilo->problem->getNCols() )
      {
         return PAPILO_ERROR_INVALID_PARAMETER;
      }

      const auto& lower = papilo->problem->getLowerBounds();
      const auto& upper = papilo->problem->getUpperBounds();

      if( lb )
      {
         *lb = lower[col];
      }

      if( ub )
      {
         *ub = upper[col];
      }

      return PAPILO_OK;
   }

   int
   papilo_get_col_bounds_all( const papilo_t* papilo, double* lb, double* ub )
   {
      if( !papilo || !papilo->problem || !papilo->problem_built ||
          ( !lb && !ub ) )
      {
         return PAPILO_ERROR_INVALID_PARAMETER;
      }

      const auto& lower = papilo->problem->getLowerBounds();
      const auto& upper = papilo->problem->getUpperBounds();

      if( lb )
      {
         std::copy( lower.begin(), lower.end(), lb );
      }

      if( ub )
      {
         std::copy( upper.begin(), upper.end(), ub );
      }

      return PAPILO_OK;
   }

   int
   papilo_get_row_bounds( const papilo_t* papilo, int row, double* lhs,
                          double* rhs )
   {
      if( !papilo || !papilo->problem || !papilo->problem_built || row < 0 ||
          row >= papilo->problem->getNRows() )
      {
         return PAPILO_ERROR_INVALID_PARAMETER;
      }

      const auto& lhs_vec =
          papilo->problem->getConstraintMatrix().getLeftHandSides();
      const auto& rhs_vec =
          papilo->problem->getConstraintMatrix().getRightHandSides();

      if( lhs )
      {
         *lhs = lhs_vec[row];
      }

      if( rhs )
      {
         *rhs = rhs_vec[row];
      }

      return PAPILO_OK;
   }

   int
   papilo_get_row_bounds_all( const papilo_t* papilo, double* lhs, double* rhs )
   {
      if( !papilo || !papilo->problem || !papilo->problem_built ||
          ( !lhs && !rhs ) )
      {
         return PAPILO_ERROR_INVALID_PARAMETER;
      }

      const auto& lhs_vec =
          papilo->problem->getConstraintMatrix().getLeftHandSides();
      const auto& rhs_vec =
          papilo->problem->getConstraintMatrix().getRightHandSides();

      if( lhs )
      {
         std::copy( lhs_vec.begin(), lhs_vec.end(), lhs );
      }

      if( rhs )
      {
         std::copy( rhs_vec.begin(), rhs_vec.end(), rhs );
      }

      return PAPILO_OK;
   }

   int
   papilo_get_matrix( const papilo_t* papilo, int* rows, int* cols,
                      double* values )
   {
      if( !papilo || !papilo->problem || !papilo->problem_built ||
          ( !rows && !cols && !values ) )
      {
         return PAPILO_ERROR_INVALID_PARAMETER;
      }

      const auto& matrix = papilo->problem->getConstraintMatrix();

      // Iterate through the sparse matrix structure
      int idx = 0;
      for( int col = 0; col < matrix.getNCols(); ++col )
      {
         auto col_range = matrix.getColumnCoefficients( col );
         for( int i = 0; i < col_range.getLength(); ++i )
         {
            if( rows )
               rows[idx] = col_range.getIndices()[i];
            if( cols )
               cols[idx] = col;
            if( values )
               values[idx] = col_range.getValues()[i];
            ++idx;
         }
      }

      return PAPILO_OK;
   }

} // extern "C"
