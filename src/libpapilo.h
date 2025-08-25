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

   /* Flag definitions for column and row properties */
   typedef enum
   {
      LIBPAPILO_COLFLAG_LB_INF = 1 << 0,
      LIBPAPILO_COLFLAG_UB_INF = 1 << 1,
      LIBPAPILO_COLFLAG_INTEGRAL = 1 << 2,
      LIBPAPILO_COLFLAG_IMPLIED_INTEGRAL = 1 << 3,
      LIBPAPILO_COLFLAG_FIXED = 1 << 4
   } libpapilo_col_flags_t;

   typedef enum
   {
      LIBPAPILO_ROWFLAG_LHS_INF = 1 << 0,
      LIBPAPILO_ROWFLAG_RHS_INF = 1 << 1,
      LIBPAPILO_ROWFLAG_REDUNDANT = 1 << 2,
      LIBPAPILO_ROWFLAG_EQUATION = 1 << 3
   } libpapilo_row_flags_t;

   /* Presolve status codes */
   typedef enum
   {
      LIBPAPILO_PRESOLVE_STATUS_UNCHANGED = 0,
      LIBPAPILO_PRESOLVE_STATUS_REDUCED = 1,
      LIBPAPILO_PRESOLVE_STATUS_UNBOUNDED = 2,
      LIBPAPILO_PRESOLVE_STATUS_UNBOUNDED_OR_INFEASIBLE = 3,
      LIBPAPILO_PRESOLVE_STATUS_INFEASIBLE = 4
   } libpapilo_presolve_status_t;

   /* Postsolve status codes */
   typedef enum
   {
      LIBPAPILO_POSTSOLVE_STATUS_OK = 0,
      LIBPAPILO_POSTSOLVE_STATUS_ERROR = 1
   } libpapilo_postsolve_status_t;

   /* Reduction type for columns
    * Note: Negative values are used to maintain compatibility with the
    * underlying C++ implementation where these values distinguish reduction
    * types from row/column indices in the internal data structures. */
   typedef enum
   {
      LIBPAPILO_COL_REDUCTION_NONE = -1,
      LIBPAPILO_COL_REDUCTION_LOWER_BOUND = -3,
      LIBPAPILO_COL_REDUCTION_UPPER_BOUND = -4,
      LIBPAPILO_COL_REDUCTION_FIXED = -5,
      LIBPAPILO_COL_REDUCTION_LOCKED = -6,
      LIBPAPILO_COL_REDUCTION_SUBSTITUTE = -8,
      LIBPAPILO_COL_REDUCTION_BOUNDS_LOCKED = -9,
      LIBPAPILO_COL_REDUCTION_REPLACE = -10,
      LIBPAPILO_COL_REDUCTION_SUBSTITUTE_OBJ = -11,
      LIBPAPILO_COL_REDUCTION_PARALLEL = -12,
      LIBPAPILO_COL_REDUCTION_IMPL_INT = -13,
      LIBPAPILO_COL_REDUCTION_FIXED_INFINITY = -14
   } libpapilo_col_reduction_t;

   /* Reduction type for rows
    * Note: Negative values are used to maintain compatibility with the
    * underlying C++ implementation where these values distinguish reduction
    * types from row/column indices in the internal data structures. */
   typedef enum
   {
      LIBPAPILO_ROW_REDUCTION_NONE = -1,
      LIBPAPILO_ROW_REDUCTION_RHS = -2,
      LIBPAPILO_ROW_REDUCTION_LHS = -3,
      LIBPAPILO_ROW_REDUCTION_REDUNDANT = -4,
      LIBPAPILO_ROW_REDUCTION_LOCKED = -5,
      LIBPAPILO_ROW_REDUCTION_RHS_INF = -7,
      LIBPAPILO_ROW_REDUCTION_LHS_INF = -8,
      LIBPAPILO_ROW_REDUCTION_SPARSIFY = -9,
      LIBPAPILO_ROW_REDUCTION_RHS_LESS_RESTRICTIVE = -10,
      LIBPAPILO_ROW_REDUCTION_LHS_LESS_RESTRICTIVE = -11,
      LIBPAPILO_ROW_REDUCTION_REASON_FOR_LESS_RESTRICTIVE_BOUND_CHANGE = -12,
      LIBPAPILO_ROW_REDUCTION_SAVE_ROW = -13,
      LIBPAPILO_ROW_REDUCTION_CERTIFICATE_RHS_GCD = -14,
      LIBPAPILO_ROW_REDUCTION_IMPLIED_BOUNDS = -15,
      LIBPAPILO_ROW_REDUCTION_PARALLEL_ROW = -16
   } libpapilo_row_reduction_t;

   /* Reduction info structure */
   typedef struct
   {
      int row;
      int col;
      double newval;
   } libpapilo_reduction_info_t;

   /** Opaque pointer for papilo::Problem<double> */
   typedef struct libpapilo_problem_t libpapilo_problem_t;
   /** Opaque pointer for papilo::ProblemBuilder<double> */
   typedef struct libpapilo_problem_builder_t libpapilo_problem_builder_t;
   /** Opaque pointer for papilo::PresolveOptions */
   typedef struct libpapilo_presolve_options_t libpapilo_presolve_options_t;
   /** Opaque pointer for papilo::Statistics */
   typedef struct libpapilo_statistics_t libpapilo_statistics_t;
   /** Opaque pointer for papilo::PostsolveStorage<double> */
   typedef struct libpapilo_postsolve_storage_t libpapilo_postsolve_storage_t;
   /** Opaque pointer for papilo::ProblemUpdate<double> */
   typedef struct libpapilo_problem_update_t libpapilo_problem_update_t;
   /** Opaque pointer for papilo::Reductions<double> */
   typedef struct libpapilo_reductions_t libpapilo_reductions_t;
   /** Opaque pointer for papilo::SingletonCols<double> */
   typedef struct libpapilo_singleton_cols_t libpapilo_singleton_cols_t;
   /** Opaque pointer for papilo::SimpleSubstitution<double> */
   typedef struct libpapilo_simple_substitution_t
       libpapilo_simple_substitution_t;
   /** Opaque pointer for papilo::Num<double> */
   typedef struct libpapilo_num_t libpapilo_num_t;
   /** Opaque pointer for papilo::Timer */
   typedef struct libpapilo_timer_t libpapilo_timer_t;
   /** Opaque pointer for papilo::Message */
   typedef struct libpapilo_message_t libpapilo_message_t;
   /** Opaque pointer for papilo::Presolve<double> */
   typedef struct libpapilo_presolve_t libpapilo_presolve_t;
   /** Opaque pointer for papilo::Solution<double> */
   typedef struct libpapilo_solution_t libpapilo_solution_t;
   /** Opaque pointer for papilo::Postsolve<double> */
   typedef struct libpapilo_postsolve_t libpapilo_postsolve_t;

   /**
    * Get the version string of the libpapilo library.
    *
    * @return A constant string containing the version information
    */
   LIBPAPILO_EXPORT
   const char*
   libpapilo_version();

   LIBPAPILO_EXPORT
   libpapilo_problem_builder_t*
   libpapilo_problem_builder_create();

   LIBPAPILO_EXPORT
   void
   libpapilo_problem_builder_free( libpapilo_problem_builder_t* builder );

   LIBPAPILO_EXPORT void
   libpapilo_problem_builder_reserve( libpapilo_problem_builder_t* builder,
                                      int nnz, int nrows, int ncols );

   /* Set dimensions */
   LIBPAPILO_EXPORT void
   libpapilo_problem_builder_set_num_cols( libpapilo_problem_builder_t* builder,
                                           int ncols );

   LIBPAPILO_EXPORT void
   libpapilo_problem_builder_set_num_rows( libpapilo_problem_builder_t* builder,
                                           int nrows );

   LIBPAPILO_EXPORT int
   libpapilo_problem_builder_get_num_cols(
       const libpapilo_problem_builder_t* builder );

   LIBPAPILO_EXPORT int
   libpapilo_problem_builder_get_num_rows(
       const libpapilo_problem_builder_t* builder );

   /* Objective functions */
   LIBPAPILO_EXPORT void
   libpapilo_problem_builder_set_obj( libpapilo_problem_builder_t* builder,
                                      int col, double val );

   LIBPAPILO_EXPORT void
   libpapilo_problem_builder_set_obj_all( libpapilo_problem_builder_t* builder,
                                          const double* values );

   LIBPAPILO_EXPORT void
   libpapilo_problem_builder_set_obj_offset(
       libpapilo_problem_builder_t* builder, double val );

   /* Column bounds */
   LIBPAPILO_EXPORT void
   libpapilo_problem_builder_set_col_lb( libpapilo_problem_builder_t* builder,
                                         int col, double lb );

   LIBPAPILO_EXPORT void
   libpapilo_problem_builder_set_col_lb_all(
       libpapilo_problem_builder_t* builder, const double* lbs );

   LIBPAPILO_EXPORT void
   libpapilo_problem_builder_set_col_ub( libpapilo_problem_builder_t* builder,
                                         int col, double ub );

   LIBPAPILO_EXPORT void
   libpapilo_problem_builder_set_col_ub_all(
       libpapilo_problem_builder_t* builder, const double* ubs );

   /* Column infinity bounds */
   LIBPAPILO_EXPORT void
   libpapilo_problem_builder_set_col_lb_inf_all(
       libpapilo_problem_builder_t* builder, const uint8_t* is_inf );

   LIBPAPILO_EXPORT void
   libpapilo_problem_builder_set_col_ub_inf_all(
       libpapilo_problem_builder_t* builder, const uint8_t* is_inf );

   /* Column properties */
   LIBPAPILO_EXPORT void
   libpapilo_problem_builder_set_col_integral(
       libpapilo_problem_builder_t* builder, int col, int is_integral );

   LIBPAPILO_EXPORT void
   libpapilo_problem_builder_set_col_integral_all(
       libpapilo_problem_builder_t* builder, const uint8_t* is_integral );

   /* Row bounds */
   LIBPAPILO_EXPORT void
   libpapilo_problem_builder_set_row_lhs( libpapilo_problem_builder_t* builder,
                                          int row, double lhs );

   LIBPAPILO_EXPORT void
   libpapilo_problem_builder_set_row_lhs_all(
       libpapilo_problem_builder_t* builder, const double* lhs_vals );

   LIBPAPILO_EXPORT void
   libpapilo_problem_builder_set_row_rhs( libpapilo_problem_builder_t* builder,
                                          int row, double rhs );

   LIBPAPILO_EXPORT void
   libpapilo_problem_builder_set_row_rhs_all(
       libpapilo_problem_builder_t* builder, const double* rhs_vals );

   /* Row infinity bounds */
   LIBPAPILO_EXPORT void
   libpapilo_problem_builder_set_row_lhs_inf_all(
       libpapilo_problem_builder_t* builder, const uint8_t* is_inf );

   LIBPAPILO_EXPORT void
   libpapilo_problem_builder_set_row_rhs_inf_all(
       libpapilo_problem_builder_t* builder, const uint8_t* is_inf );

   /* Matrix entries */
   LIBPAPILO_EXPORT void
   libpapilo_problem_builder_add_entry( libpapilo_problem_builder_t* builder,
                                        int row, int col, double val );

   LIBPAPILO_EXPORT void
   libpapilo_problem_builder_add_entry_all(
       libpapilo_problem_builder_t* builder, int count, const int* rows,
       const int* cols, const double* vals );

   LIBPAPILO_EXPORT void
   libpapilo_problem_builder_add_row_entries(
       libpapilo_problem_builder_t* builder, int row, int len, const int* cols,
       const double* vals );

   LIBPAPILO_EXPORT void
   libpapilo_problem_builder_add_col_entries(
       libpapilo_problem_builder_t* builder, int col, int len, const int* rows,
       const double* vals );

   /* Names */
   LIBPAPILO_EXPORT void
   libpapilo_problem_builder_set_problem_name(
       libpapilo_problem_builder_t* builder, const char* name );

   LIBPAPILO_EXPORT void
   libpapilo_problem_builder_set_col_name( libpapilo_problem_builder_t* builder,
                                           int col, const char* name );

   LIBPAPILO_EXPORT void
   libpapilo_problem_builder_set_row_name( libpapilo_problem_builder_t* builder,
                                           int row, const char* name );

   /* Batch name setters */
   LIBPAPILO_EXPORT void
   libpapilo_problem_builder_set_col_name_all(
       libpapilo_problem_builder_t* builder, const char* const* names );

   LIBPAPILO_EXPORT void
   libpapilo_problem_builder_set_row_name_all(
       libpapilo_problem_builder_t* builder, const char* const* names );

   /* Build */
   LIBPAPILO_EXPORT
   libpapilo_problem_t*
   libpapilo_problem_builder_build( libpapilo_problem_builder_t* builder );

   /* Problem API */
   LIBPAPILO_EXPORT void
   libpapilo_problem_free( libpapilo_problem_t* problem );

   LIBPAPILO_EXPORT int
   libpapilo_problem_get_nrows( const libpapilo_problem_t* problem );

   LIBPAPILO_EXPORT int
   libpapilo_problem_get_ncols( const libpapilo_problem_t* problem );

   LIBPAPILO_EXPORT int
   libpapilo_problem_get_nnz( const libpapilo_problem_t* problem );

   /* Problem data getters */
   LIBPAPILO_EXPORT int
   libpapilo_problem_get_num_integral_cols(
       const libpapilo_problem_t* problem );

   LIBPAPILO_EXPORT int
   libpapilo_problem_get_num_continuous_cols(
       const libpapilo_problem_t* problem );

   /* Objective getters */
   LIBPAPILO_EXPORT const double*
   libpapilo_problem_get_objective_coefficients(
       const libpapilo_problem_t* problem, int* size );

   LIBPAPILO_EXPORT double
   libpapilo_problem_get_objective_offset( const libpapilo_problem_t* problem );

   /* Bounds getters */
   LIBPAPILO_EXPORT const double*
   libpapilo_problem_get_lower_bounds( const libpapilo_problem_t* problem,
                                       int* size );

   LIBPAPILO_EXPORT const double*
   libpapilo_problem_get_upper_bounds( const libpapilo_problem_t* problem,
                                       int* size );

   /* Constraint matrix getters */
   LIBPAPILO_EXPORT const double*
   libpapilo_problem_get_row_lhs( const libpapilo_problem_t* problem,
                                  int* size );

   LIBPAPILO_EXPORT const double*
   libpapilo_problem_get_row_rhs( const libpapilo_problem_t* problem,
                                  int* size );

   LIBPAPILO_EXPORT const int*
   libpapilo_problem_get_row_sizes( const libpapilo_problem_t* problem,
                                    int* size );

   LIBPAPILO_EXPORT const int*
   libpapilo_problem_get_col_sizes( const libpapilo_problem_t* problem,
                                    int* size );

   /* Sparse matrix entry getters */
   LIBPAPILO_EXPORT int
   libpapilo_problem_get_row_entries( const libpapilo_problem_t* problem,
                                      int row, const int** cols,
                                      const double** vals );

   LIBPAPILO_EXPORT int
   libpapilo_problem_get_col_entries( const libpapilo_problem_t* problem,
                                      int col, const int** rows,
                                      const double** vals );

   /* Name getters */
   LIBPAPILO_EXPORT const char*
   libpapilo_problem_get_name( const libpapilo_problem_t* problem );

   LIBPAPILO_EXPORT const char*
   libpapilo_problem_get_variable_name( const libpapilo_problem_t* problem,
                                        int col );

   LIBPAPILO_EXPORT const char*
   libpapilo_problem_get_constraint_name( const libpapilo_problem_t* problem,
                                          int row );

   /* Flag getters */
   LIBPAPILO_EXPORT uint8_t
   libpapilo_problem_get_col_flags( const libpapilo_problem_t* problem,
                                    int col );

   LIBPAPILO_EXPORT uint8_t
   libpapilo_problem_get_row_flags( const libpapilo_problem_t* problem,
                                    int row );

   /* Flag helper functions */
   LIBPAPILO_EXPORT int
   libpapilo_problem_is_row_redundant( const libpapilo_problem_t* problem,
                                       int row );

   LIBPAPILO_EXPORT int
   libpapilo_problem_is_col_substituted( const libpapilo_problem_t* problem,
                                         int col );

   /* Additional Problem query APIs */
   LIBPAPILO_EXPORT double*
   libpapilo_problem_get_objective_coefficients_mutable(
       libpapilo_problem_t* problem, int* size );

   LIBPAPILO_EXPORT const double*
   libpapilo_problem_get_row_left_hand_sides(
       const libpapilo_problem_t* problem, int* size );

   LIBPAPILO_EXPORT const double*
   libpapilo_problem_get_row_right_hand_sides(
       const libpapilo_problem_t* problem, int* size );

   /* Phase 2: Presolve API */

   /* Core Presolve API */
   LIBPAPILO_EXPORT libpapilo_presolve_t*
   libpapilo_presolve_create();

   LIBPAPILO_EXPORT void
   libpapilo_presolve_free( libpapilo_presolve_t* presolve );

   LIBPAPILO_EXPORT void
   libpapilo_presolve_add_default_presolvers( libpapilo_presolve_t* presolve );

   LIBPAPILO_EXPORT void
   libpapilo_presolve_set_options( libpapilo_presolve_t* presolve,
                                   libpapilo_presolve_options_t* options );

   LIBPAPILO_EXPORT libpapilo_presolve_status_t
   libpapilo_presolve_apply_simple( libpapilo_presolve_t* presolve,
                                    libpapilo_problem_t* problem );

   LIBPAPILO_EXPORT void
   libpapilo_presolve_apply_reductions( libpapilo_presolve_t* presolve,
                                        int round,
                                        libpapilo_reductions_t* reductions,
                                        libpapilo_problem_update_t* update,
                                        int* num_rounds, int* num_changes );

   /* PresolveOptions management */
   LIBPAPILO_EXPORT libpapilo_presolve_options_t*
   libpapilo_presolve_options_create();

   LIBPAPILO_EXPORT void
   libpapilo_presolve_options_free( libpapilo_presolve_options_t* options );

   LIBPAPILO_EXPORT void
   libpapilo_presolve_options_set_dualreds(
       libpapilo_presolve_options_t* options, int dualreds );

   /* Main presolve function */
   LIBPAPILO_EXPORT libpapilo_presolve_status_t
   libpapilo_presolve_apply( libpapilo_problem_t* problem,
                             libpapilo_presolve_options_t* options,
                             libpapilo_reductions_t** reductions,
                             libpapilo_postsolve_storage_t** postsolve,
                             libpapilo_statistics_t** statistics );

   /* Reductions access API */
   LIBPAPILO_EXPORT libpapilo_reductions_t*
   libpapilo_reductions_create();

   LIBPAPILO_EXPORT int
   libpapilo_reductions_get_size( libpapilo_reductions_t* reductions );

   LIBPAPILO_EXPORT libpapilo_reduction_info_t
   libpapilo_reductions_get_info( libpapilo_reductions_t* reductions,
                                  int index );

   LIBPAPILO_EXPORT void
   libpapilo_reductions_free( libpapilo_reductions_t* reductions );

   /* Reductions manipulation API */
   LIBPAPILO_EXPORT void
   libpapilo_reductions_replace_col( libpapilo_reductions_t* reductions,
                                     int col, int replace_col, double scale,
                                     double offset );

   LIBPAPILO_EXPORT void
   libpapilo_reductions_lock_col_bounds( libpapilo_reductions_t* reductions,
                                         int col );

   LIBPAPILO_EXPORT void
   libpapilo_reductions_lock_row( libpapilo_reductions_t* reductions, int row );

   LIBPAPILO_EXPORT void
   libpapilo_reductions_substitute_col_in_objective(
       libpapilo_reductions_t* reductions, int col, int row );

   LIBPAPILO_EXPORT void
   libpapilo_reductions_mark_row_redundant( libpapilo_reductions_t* reductions,
                                            int row );

   LIBPAPILO_EXPORT void
   libpapilo_reductions_aggregate_free_col( libpapilo_reductions_t* reductions,
                                            int col, int row );

   LIBPAPILO_EXPORT void
   libpapilo_reductions_begin_transaction( libpapilo_reductions_t* reductions );

   LIBPAPILO_EXPORT void
   libpapilo_reductions_end_transaction( libpapilo_reductions_t* reductions );

   /* PostsolveStorage management */
   LIBPAPILO_EXPORT libpapilo_postsolve_storage_t*
   libpapilo_postsolve_storage_create(
       const libpapilo_problem_t* problem, const libpapilo_num_t* num,
       const libpapilo_presolve_options_t* options );

   LIBPAPILO_EXPORT void
   libpapilo_postsolve_storage_free( libpapilo_postsolve_storage_t* postsolve );

   /* Statistics access API */
   LIBPAPILO_EXPORT libpapilo_statistics_t*
   libpapilo_statistics_create();

   LIBPAPILO_EXPORT void
   libpapilo_statistics_free( libpapilo_statistics_t* statistics );

   /* Problem Modification API */
   LIBPAPILO_EXPORT void
   libpapilo_problem_modify_row_lhs( libpapilo_problem_t* problem, int row,
                                     double lhs );

   LIBPAPILO_EXPORT void
   libpapilo_problem_recompute_locks( libpapilo_problem_t* problem );

   LIBPAPILO_EXPORT void
   libpapilo_problem_recompute_activities( libpapilo_problem_t* problem );

   LIBPAPILO_EXPORT void
   libpapilo_problem_recompute_all_activities( libpapilo_problem_t* problem );

   /* Utility Objects API */
   LIBPAPILO_EXPORT libpapilo_num_t*
   libpapilo_num_create();

   LIBPAPILO_EXPORT void
   libpapilo_num_free( libpapilo_num_t* num );

   LIBPAPILO_EXPORT libpapilo_timer_t*
   libpapilo_timer_create( double* time_ref );

   LIBPAPILO_EXPORT void
   libpapilo_timer_free( libpapilo_timer_t* timer );

   LIBPAPILO_EXPORT libpapilo_message_t*
   libpapilo_message_create();

   LIBPAPILO_EXPORT void
   libpapilo_message_free( libpapilo_message_t* message );

   /* ProblemUpdate Control API */
   LIBPAPILO_EXPORT libpapilo_problem_update_t*
   libpapilo_problem_update_create( libpapilo_problem_t* problem,
                                    libpapilo_postsolve_storage_t* postsolve,
                                    libpapilo_statistics_t* statistics,
                                    const libpapilo_presolve_options_t* options,
                                    const libpapilo_num_t* num,
                                    const libpapilo_message_t* message );

   LIBPAPILO_EXPORT void
   libpapilo_problem_update_free( libpapilo_problem_update_t* update );

   LIBPAPILO_EXPORT libpapilo_presolve_status_t
   libpapilo_problem_update_trivial_column_presolve(
       libpapilo_problem_update_t* update );

   LIBPAPILO_EXPORT libpapilo_presolve_status_t
   libpapilo_problem_update_trivial_presolve(
       libpapilo_problem_update_t* update );

   LIBPAPILO_EXPORT int
   libpapilo_problem_update_get_singleton_cols_count(
       libpapilo_problem_update_t* update );

   LIBPAPILO_EXPORT libpapilo_reductions_t*
   libpapilo_problem_update_get_reductions(
       libpapilo_problem_update_t* update );

   LIBPAPILO_EXPORT void
   libpapilo_problem_update_set_postpone_substitutions(
       libpapilo_problem_update_t* update, int postpone );

   /* Individual Presolver API */
   LIBPAPILO_EXPORT libpapilo_singleton_cols_t*
   libpapilo_singleton_cols_create();

   LIBPAPILO_EXPORT void
   libpapilo_singleton_cols_free( libpapilo_singleton_cols_t* presolver );

   LIBPAPILO_EXPORT libpapilo_presolve_status_t
   libpapilo_singleton_cols_execute( libpapilo_singleton_cols_t* presolver,
                                     const libpapilo_problem_t* problem,
                                     const libpapilo_problem_update_t* update,
                                     const libpapilo_num_t* num,
                                     libpapilo_reductions_t* reductions,
                                     const libpapilo_timer_t* timer,
                                     int* cause );

   /* SimpleSubstitution Presolver API */
   LIBPAPILO_EXPORT libpapilo_simple_substitution_t*
   libpapilo_simple_substitution_create();

   LIBPAPILO_EXPORT void
   libpapilo_simple_substitution_free(
       libpapilo_simple_substitution_t* presolver );

   LIBPAPILO_EXPORT libpapilo_presolve_status_t
   libpapilo_simple_substitution_execute(
       libpapilo_simple_substitution_t* presolver,
       const libpapilo_problem_t* problem,
       const libpapilo_problem_update_t* update, const libpapilo_num_t* num,
       libpapilo_reductions_t* reductions, const libpapilo_timer_t* timer,
       int* cause );

   /* Solution Management API */
   LIBPAPILO_EXPORT libpapilo_solution_t*
   libpapilo_solution_create();

   LIBPAPILO_EXPORT void
   libpapilo_solution_free( libpapilo_solution_t* solution );

   LIBPAPILO_EXPORT const double*
   libpapilo_solution_get_primal( const libpapilo_solution_t* solution,
                                  int* size );

   LIBPAPILO_EXPORT void
   libpapilo_solution_set_primal( libpapilo_solution_t* solution,
                                  const double* values, int size );

   /* Postsolve Engine API */
   LIBPAPILO_EXPORT libpapilo_postsolve_t*
   libpapilo_postsolve_create( const libpapilo_message_t* message,
                               const libpapilo_num_t* num );

   LIBPAPILO_EXPORT void
   libpapilo_postsolve_free( libpapilo_postsolve_t* postsolve );

   LIBPAPILO_EXPORT libpapilo_postsolve_status_t
   libpapilo_postsolve_undo( libpapilo_postsolve_t* postsolve,
                             const libpapilo_solution_t* reduced_solution,
                             libpapilo_solution_t* original_solution,
                             const libpapilo_postsolve_storage_t* storage );

   /* PostsolveStorage File I/O API */
   LIBPAPILO_EXPORT libpapilo_postsolve_storage_t*
   libpapilo_postsolve_storage_load_from_file( const char* filename );

#ifdef __cplusplus
}
#endif

#endif /* LIBPAPILO_H */
