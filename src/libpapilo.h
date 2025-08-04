#ifndef LIBPAPILO_H
#define LIBPAPILO_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stddef.h>
#include <stdint.h>

/* Export macros for shared library */
#ifdef _WIN32
#ifdef libpapilo_EXPORTS
#define LIBPAPILO_EXPORT __declspec( dllexport )
#else
#define LIBPAPILO_EXPORT __declspec( dllimport )
#endif
#else
#define LIBPAPILO_EXPORT __attribute__( ( visibility( "default" ) ) )
#endif

/* Error codes */
#define PAPILO_OK 0
#define PAPILO_ERROR_OUT_OF_MEMORY -1
#define PAPILO_ERROR_INVALID_PARAMETER -2
#define PAPILO_ERROR_INVALID_STATE -3
#define PAPILO_ERROR_SOLVER_FAILURE -4

   /* Presolve status codes */
   typedef enum
   {
      PAPILO_STATUS_UNCHANGED = 0,
      PAPILO_STATUS_REDUCED = 1,
      PAPILO_STATUS_INFEASIBLE = 2,
      PAPILO_STATUS_UNBOUNDED = 3,
      PAPILO_STATUS_UNBOUNDED_OR_INFEASIBLE = 4,
      PAPILO_STATUS_ERROR = -1
   } papilo_status_t;

   /* Opaque handle types */
   typedef struct papilo_t papilo_t;
   /** Opaque pointer for papilo::Problem<double> */
   typedef struct libpapilo_problem_t libpapilo_problem_t;
   /** Opaque pointer for papilo::ProblemBuilder<double> */
   typedef struct libpapilo_problem_builder_t libpapilo_problem_builder_t;

   LIBPAPILO_EXPORT
   libpapilo_problem_builder_t*
   libpapilo_problem_builder_create();

   LIBPAPILO_EXPORT
   void
   libpapilo_problem_builder_free( libpapilo_problem_builder_t* builder );

   void
   libpapilo_problem_builder_reserve( libpapilo_problem_builder_t* builder,
                                      int nrows, int ncols, int nnz );

   LIBPAPILO_EXPORT
   libpapilo_problem_t*
   libpapilo_problem_builder_build( libpapilo_problem_builder_t* builder );

   /* Problem construction API */
   LIBPAPILO_EXPORT papilo_t*
   papilo_create( void );
   LIBPAPILO_EXPORT void
   papilo_free( papilo_t* papilo );

   /* Set problem dimensions */
   LIBPAPILO_EXPORT int
   papilo_set_problem_dimensions( papilo_t* papilo, int nrows, int ncols,
                                  int nnz );

   /* Set objective function */
   LIBPAPILO_EXPORT int
   papilo_set_objective( papilo_t* papilo, const double* coefficients,
                         double offset );

   /* Set variable bounds */
   LIBPAPILO_EXPORT int
   papilo_set_col_bounds( papilo_t* papilo, int col, double lb, double ub );
   LIBPAPILO_EXPORT int
   papilo_set_col_bounds_all( papilo_t* papilo, const double* lb,
                              const double* ub );

   /* Set constraint bounds */
   LIBPAPILO_EXPORT int
   papilo_set_row_bounds( papilo_t* papilo, int row, double lhs, double rhs );
   LIBPAPILO_EXPORT int
   papilo_set_row_bounds_all( papilo_t* papilo, const double* lhs,
                              const double* rhs );

   /* Add matrix entries */
   LIBPAPILO_EXPORT int
   papilo_add_entry( papilo_t* papilo, int row, int col, double value );
   LIBPAPILO_EXPORT int
   papilo_add_entries( papilo_t* papilo, int count, const int* rows,
                       const int* cols, const double* values );

   /* Build the problem (finalize construction) */
   LIBPAPILO_EXPORT int
   papilo_build_problem( papilo_t* papilo );

   /* Get problem dimensions */
   LIBPAPILO_EXPORT int
   papilo_get_nrows( const papilo_t* papilo );
   LIBPAPILO_EXPORT int
   papilo_get_ncols( const papilo_t* papilo );
   LIBPAPILO_EXPORT int
   papilo_get_nnz( const papilo_t* papilo );

   /* Get objective function */
   LIBPAPILO_EXPORT int
   papilo_get_objective( const papilo_t* papilo, double* coefficients,
                         double* offset );

   /* Get variable bounds */
   LIBPAPILO_EXPORT int
   papilo_get_col_bounds( const papilo_t* papilo, int col, double* lb,
                          double* ub );
   LIBPAPILO_EXPORT int
   papilo_get_col_bounds_all( const papilo_t* papilo, double* lb, double* ub );

   /* Get constraint bounds */
   LIBPAPILO_EXPORT int
   papilo_get_row_bounds( const papilo_t* papilo, int row, double* lhs,
                          double* rhs );
   LIBPAPILO_EXPORT int
   papilo_get_row_bounds_all( const papilo_t* papilo, double* lhs,
                              double* rhs );

   /* Get matrix entries */
   LIBPAPILO_EXPORT int
   papilo_get_matrix( const papilo_t* papilo, int* rows, int* cols,
                      double* values );

#ifdef __cplusplus
}
#endif

#endif /* LIBPAPILO_H */
