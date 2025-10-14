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
   /**
    * C representation of PaPILO's `papilo::ReductionType` enum.  The numeric
    * values match the C++ counterparts so the arrays returned by
    * `libpapilo_postsolve_storage_get_types()` can be interpreted directly.
    */
   typedef enum
   {
      LIBPAPILO_COLFLAG_LB_INF = 1 << 0,
      LIBPAPILO_COLFLAG_UB_INF = 1 << 1,
      LIBPAPILO_COLFLAG_INTEGRAL = 1 << 2,
      LIBPAPILO_COLFLAG_IMPLIED_INTEGRAL = 1 << 3,
      LIBPAPILO_COLFLAG_FIXED = 1 << 4
   } libpapilo_col_flags_t;

   /** Row flags for constraint properties */
   typedef enum
   {
      LIBPAPILO_ROWFLAG_LHS_INF = 1 << 0,
      LIBPAPILO_ROWFLAG_RHS_INF = 1 << 1,
      LIBPAPILO_ROWFLAG_REDUNDANT = 1 << 2,
      LIBPAPILO_ROWFLAG_EQUATION = 1 << 3
   } libpapilo_row_flags_t;

   /** Presolve status codes */
   typedef enum
   {
      LIBPAPILO_PRESOLVE_STATUS_UNCHANGED = 0,
      LIBPAPILO_PRESOLVE_STATUS_REDUCED = 1,
      LIBPAPILO_PRESOLVE_STATUS_UNBOUNDED = 2,
      LIBPAPILO_PRESOLVE_STATUS_UNBOUNDED_OR_INFEASIBLE = 3,
      LIBPAPILO_PRESOLVE_STATUS_INFEASIBLE = 4
   } libpapilo_presolve_status_t;

   /** Postsolve status codes */
   typedef enum
   {
      LIBPAPILO_POSTSOLVE_STATUS_OK = 0,
      LIBPAPILO_POSTSOLVE_STATUS_ERROR = 1
   } libpapilo_postsolve_status_t;

   /**
    * Dual reductions mode
    *
    * Controls the level of dual reductions applied during presolving.
    * Dual reductions use dual information to eliminate variables and
    * constraints.
    */
   typedef enum
   {
      /** Disable all dual reductions */
      LIBPAPILO_DUALREDS_DISABLE = 0,
      /** Allow only dual reductions that never cut off optimal solutions */
      LIBPAPILO_DUALREDS_SAFE = 1,
      /** Allow all dual reductions (default) */
      LIBPAPILO_DUALREDS_ALL = 2
   } libpapilo_dualreds_t;

   /**
    * Reduction type for columns
    *
    * Note: Negative values are used to maintain compatibility with the
    * underlying C++ implementation where these values distinguish reduction
    * types from row/column indices in the internal data structures.
    */
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

   /** Postsolve type enum for choosing postsolve strategy */
   typedef enum
   {
      LIBPAPILO_POSTSOLVE_TYPE_PRIMAL = 0,
      LIBPAPILO_POSTSOLVE_TYPE_FULL = 1
   } libpapilo_postsolve_type_t;

   /**
    * C representation of PaPILO's `papilo::ReductionType` enum.  The values
    * appear in the `types` array returned by
    * `libpapilo_postsolve_storage_get_types()` and determine how the
    * corresponding slices from the `indices` / `values` arrays are interpreted
    * during postsolve.  For a reduction at position `i`, consult `start[i] ..
    * start[i + 1]` to obtain the payload.  Unless noted otherwise,
    * `indices[start[i]]` stores the original row/column index (after applying
    * `orig*_mapping`).
    */
   typedef enum
   {
      /** A column was fixed to a scalar value. Payload: [orig_col], value. */
      LIBPAPILO_POSTSOLVE_REDUCTION_FIXED_COL = 0,
      /** Column eliminated via substitution (primal only). Payload encodes the
       * sparse equation defining the eliminated variable and its RHS. */
      LIBPAPILO_POSTSOLVE_REDUCTION_SUBSTITUTED_COL = 1,
      /** Two parallel columns merged. Payload carries both column ids, scaling
       * factor, and original bounds to disaggregate during undo. */
      LIBPAPILO_POSTSOLVE_REDUCTION_PARALLEL_COL = 2,
      /** Column substitution including dual/basis information for full
         postsolve. */
      LIBPAPILO_POSTSOLVE_REDUCTION_SUBSTITUTED_COL_WITH_DUAL = 3,
      /** Variable bound tightened. Payload: [orig_col], flag for lower/upper,
       * former bound, and infinity marker. */
      LIBPAPILO_POSTSOLVE_REDUCTION_VAR_BOUND_CHANGE = 4,
      /** Column fixed while an infinite bound was active. Payload references
       * the supporting rows/coefficients required to recover primal and dual
       * values. */
      LIBPAPILO_POSTSOLVE_REDUCTION_FIXED_INF_COL = 5,
      /** Redundant row removed. Payload: [orig_row]; dual postsolve sets its
       * dual to zero. */
      LIBPAPILO_POSTSOLVE_REDUCTION_REDUNDANT_ROW = 7,
      /** Row bound tightened. Payload: [orig_row], bound side, previous value,
         flag. */
      LIBPAPILO_POSTSOLVE_REDUCTION_ROW_BOUND_CHANGE = 8,
      /** Records the dependency between two rows when a bound change was forced
       * by another constraint. Payload references both row indices and the
       * scaling factor used. */
      LIBPAPILO_POSTSOLVE_REDUCTION_REASON_FOR_ROW_BOUND_CHANGE_FORCED_BY_ROW =
          9,
      /** Row bound change propagated from another row. Payload combines row
       * ids, direction, and stored bounds so dual feasibility can be restored.
       */
      LIBPAPILO_POSTSOLVE_REDUCTION_ROW_BOUND_CHANGE_FORCED_BY_ROW = 10,
      /** Complete row snapshot (bounds, flags, sparse entries). Payload lists
       * metadata followed by (orig_col, coefficient) pairs. */
      LIBPAPILO_POSTSOLVE_REDUCTION_SAVE_ROW = 11,
      /** Cached bounds/objective data used by reduced-bounds restoration. */
      LIBPAPILO_POSTSOLVE_REDUCTION_REDUCED_BOUNDS_COST = 12,
      /** Column dual value saved for full postsolve. Payload: [orig_col], dual.
       */
      LIBPAPILO_POSTSOLVE_REDUCTION_COLUMN_DUAL_VALUE = 13,
      /** Row dual value saved for full postsolve. Payload: [orig_row], dual. */
      LIBPAPILO_POSTSOLVE_REDUCTION_ROW_DUAL_VALUE = 14,
      /** Single matrix coefficient modification. Payload: [orig_row, orig_col],
       * new value. */
      LIBPAPILO_POSTSOLVE_REDUCTION_COEFFICIENT_CHANGE = 15
   } libpapilo_postsolve_reduction_type_t;

   /**
    * Reduction type for rows
    *
    * Note: Negative values are used to maintain compatibility with the
    * underlying C++ implementation where these values distinguish reduction
    * types from row/column indices in the internal data structures.
    */
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

   /** Reduction info structure */
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
       const libpapilo_problem_t* problem, size_t* size );

   LIBPAPILO_EXPORT double
   libpapilo_problem_get_objective_offset( const libpapilo_problem_t* problem );

   /* Bounds getters */
   LIBPAPILO_EXPORT const double*
   libpapilo_problem_get_lower_bounds( const libpapilo_problem_t* problem,
                                       size_t* size );

   LIBPAPILO_EXPORT const double*
   libpapilo_problem_get_upper_bounds( const libpapilo_problem_t* problem,
                                       size_t* size );

   /* Constraint matrix getters */
   LIBPAPILO_EXPORT const double*
   libpapilo_problem_get_row_lhs( const libpapilo_problem_t* problem,
                                  size_t* size );

   LIBPAPILO_EXPORT const double*
   libpapilo_problem_get_row_rhs( const libpapilo_problem_t* problem,
                                  size_t* size );

   LIBPAPILO_EXPORT const int*
   libpapilo_problem_get_row_sizes( const libpapilo_problem_t* problem,
                                    size_t* size );

   LIBPAPILO_EXPORT const int*
   libpapilo_problem_get_col_sizes( const libpapilo_problem_t* problem,
                                    size_t* size );

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
       libpapilo_problem_t* problem, size_t* size );

   LIBPAPILO_EXPORT const double*
   libpapilo_problem_get_row_left_hand_sides(
       const libpapilo_problem_t* problem, size_t* size );

   LIBPAPILO_EXPORT const double*
   libpapilo_problem_get_row_right_hand_sides(
       const libpapilo_problem_t* problem, size_t* size );

   /* Phase 2: Presolve API */

   /* Core Presolve API */
   LIBPAPILO_EXPORT libpapilo_presolve_t*
   libpapilo_presolve_create( const libpapilo_message_t* message );

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
       libpapilo_presolve_options_t* options, libpapilo_dualreds_t dualreds );

   LIBPAPILO_EXPORT void
   libpapilo_presolve_options_set_threads(
       libpapilo_presolve_options_t* options, int threads );

   /** Set random seed for presolving (default: 0, deterministic behavior) */
   LIBPAPILO_EXPORT void
   libpapilo_presolve_options_set_randomseed(
       libpapilo_presolve_options_t* options, unsigned int randomseed );

   LIBPAPILO_EXPORT libpapilo_dualreds_t
   libpapilo_presolve_options_get_dualreds(
       const libpapilo_presolve_options_t* options );

   LIBPAPILO_EXPORT int
   libpapilo_presolve_options_get_threads(
       const libpapilo_presolve_options_t* options );

   /** Get random seed for presolving (default: 0) */
   LIBPAPILO_EXPORT unsigned int
   libpapilo_presolve_options_get_randomseed(
       const libpapilo_presolve_options_t* options );

   /* Main presolve function */
   LIBPAPILO_EXPORT libpapilo_presolve_status_t
   libpapilo_presolve_apply( libpapilo_problem_t* problem,
                             libpapilo_presolve_options_t* options,
                             const libpapilo_message_t* message,
                             libpapilo_reductions_t** reductions,
                             libpapilo_postsolve_storage_t** postsolve,
                             libpapilo_statistics_t** statistics );

   /* Reductions access API */
   LIBPAPILO_EXPORT libpapilo_reductions_t*
   libpapilo_reductions_create();

   LIBPAPILO_EXPORT size_t
   libpapilo_reductions_get_size( const libpapilo_reductions_t* reductions );

   LIBPAPILO_EXPORT libpapilo_reduction_info_t
   libpapilo_reductions_get_info( const libpapilo_reductions_t* reductions,
                                  int index );

   LIBPAPILO_EXPORT void
   libpapilo_reductions_free( libpapilo_reductions_t* reductions );

   /* Reductions getter API */

   /** Get the number of reduction transactions. */
   LIBPAPILO_EXPORT size_t
   libpapilo_reductions_get_num_transactions(
       const libpapilo_reductions_t* reductions );

   /** Get the start index of a transaction. */
   LIBPAPILO_EXPORT size_t
   libpapilo_reductions_get_transaction_start(
       const libpapilo_reductions_t* reductions, int transaction_index );

   /** Get the end index of a transaction. */
   LIBPAPILO_EXPORT size_t
   libpapilo_reductions_get_transaction_end(
       const libpapilo_reductions_t* reductions, int transaction_index );

   /** Get the number of locks in a transaction. */
   LIBPAPILO_EXPORT size_t
   libpapilo_reductions_get_transaction_nlocks(
       const libpapilo_reductions_t* reductions, int transaction_index );

   /** Get the number of added coefficients in a transaction. */
   LIBPAPILO_EXPORT size_t
   libpapilo_reductions_get_transaction_naddcoeffs(
       const libpapilo_reductions_t* reductions, int transaction_index );

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

   /* PostsolveStorage getter API */

   /**
    * The PaPILO presolve engine records every transformation in a compact,
    * append-only log stored inside `PostsolveStorage`.  The information
    * required to undo a presolve session is split across:
    *   - `types`:  vector of `libpapilo_postsolve_reduction_type_t` entries
    * describing which reduction was applied.  The entries are ordered
    * chronologically.
    *   - `start`: prefix-array with length `types.size() + 1` marking subranges
    * inside `indices`/`values` that belong to a single reduction.  For
    * reduction `i`, the payload lives in the half-open interval `[start[i],
    * start[i + 1])`.
    *   - `indices`/`values`: heterogeneous payload describing the context for
    * each reduction.  The exact layout depends on the reduction type; for
    * example, `kFixedCol` stores the original column index followed by the
    * fixed value, while `kSaveRow` preserves the row bounds, flags, and sparse
    * coefficients.
    *   - `origcol_mapping`/`origrow_mapping`: mapping from remaining (reduced)
    * indices back to the original model after presolve.
    *
    * A postsolve implementation can therefore walk the log in reverse order:
    *   1. Determine the reduction kind via `types[i]` (see
    *      `libpapilo_postsolve_reduction_type_t`).
    *   2. Obtain the payload slice via `indices[start[i] .. start[i + 1])` and
    * the corresponding `values` slice.
    *   3. Apply the documented inverse transformation to reintroduce deleted
    * rows or variables, or to recover bounds and dual information.  The PaPILO
    * reference implementation performs this logic inside `Postsolve::undo`.
    *
    * The helper getters below expose raw pointers to the underlying arrays. The
    * returned memory is owned by the `PostsolveStorage` object and remains
    * valid until the storage is freed.
    */

   /** Get the original number of columns before presolving. Returns 0 on error.
    */
   LIBPAPILO_EXPORT unsigned int
   libpapilo_postsolve_storage_get_n_cols_original(
       const libpapilo_postsolve_storage_t* postsolve );

   /** Get the original number of rows before presolving. Returns 0 on error. */
   LIBPAPILO_EXPORT unsigned int
   libpapilo_postsolve_storage_get_n_rows_original(
       const libpapilo_postsolve_storage_t* postsolve );

   /** Get column mapping array: reduced_col_index -> original_col_index.
    * size: if not NULL, set to array size. Returns NULL on error. */
   LIBPAPILO_EXPORT const int*
   libpapilo_postsolve_storage_get_orig_col_mapping(
       const libpapilo_postsolve_storage_t* postsolve, size_t* size );

   /** Get row mapping array: reduced_row_index -> original_row_index.
    * size: if not NULL, set to array size. Returns NULL on error. */
   LIBPAPILO_EXPORT const int*
   libpapilo_postsolve_storage_get_orig_row_mapping(
       const libpapilo_postsolve_storage_t* postsolve, size_t* size );

   /** Get the postsolve type (primal or full). */
   LIBPAPILO_EXPORT libpapilo_postsolve_type_t
   libpapilo_postsolve_storage_get_postsolve_type(
       const libpapilo_postsolve_storage_t* postsolve );

   /** Get the number of reduction types. Returns -1 on error. */
   LIBPAPILO_EXPORT size_t
   libpapilo_postsolve_storage_get_num_types(
       const libpapilo_postsolve_storage_t* postsolve );

   /** Get the number of indices. Returns -1 on error. */
   LIBPAPILO_EXPORT size_t
   libpapilo_postsolve_storage_get_num_indices(
       const libpapilo_postsolve_storage_t* postsolve );

   /** Get the number of values. Returns -1 on error. */
   LIBPAPILO_EXPORT size_t
   libpapilo_postsolve_storage_get_num_values(
       const libpapilo_postsolve_storage_t* postsolve );

   /** Get the reduction type array. size: if not NULL, set to array size.
    * Returns NULL on error. */
   LIBPAPILO_EXPORT const libpapilo_postsolve_reduction_type_t*
   libpapilo_postsolve_storage_get_types(
       const libpapilo_postsolve_storage_t* postsolve, size_t* size );

   /** Get the indices array backing the reductions log. size: if not NULL, set
    * to array size. Returns NULL on error. */
   LIBPAPILO_EXPORT const int*
   libpapilo_postsolve_storage_get_indices(
       const libpapilo_postsolve_storage_t* postsolve, size_t* size );

   /** Get the values array backing the reductions log. size: if not NULL, set
    * to array size. Returns NULL on error. */
   LIBPAPILO_EXPORT const double*
   libpapilo_postsolve_storage_get_values(
       const libpapilo_postsolve_storage_t* postsolve, size_t* size );

   /** Get the start array (size == number of reductions + 1). size: if not
    * NULL, set to array size. Returns NULL on error. */
   LIBPAPILO_EXPORT const int*
   libpapilo_postsolve_storage_get_start(
       const libpapilo_postsolve_storage_t* postsolve, size_t* size );

   /** Get the original problem. Returns NULL on error.
    * Valid as long as postsolve storage exists. */
   LIBPAPILO_EXPORT const libpapilo_problem_t*
   libpapilo_postsolve_storage_get_original_problem(
       const libpapilo_postsolve_storage_t* postsolve );

   /* Statistics access API */
   LIBPAPILO_EXPORT libpapilo_statistics_t*
   libpapilo_statistics_create();

   LIBPAPILO_EXPORT void
   libpapilo_statistics_free( libpapilo_statistics_t* statistics );

   /* Statistics getter API */

   /** Get presolve execution time in seconds. Returns -1.0 on error. */
   LIBPAPILO_EXPORT double
   libpapilo_statistics_get_presolvetime(
       const libpapilo_statistics_t* statistics );

   /** Get number of transactions applied. Returns -1 on error. */
   LIBPAPILO_EXPORT size_t
   libpapilo_statistics_get_ntsxapplied(
       const libpapilo_statistics_t* statistics );

   /** Get number of transaction conflicts. */
   LIBPAPILO_EXPORT size_t
   libpapilo_statistics_get_ntsxconflicts(
       const libpapilo_statistics_t* statistics );

   /** Get number of bound changes. */
   LIBPAPILO_EXPORT size_t
   libpapilo_statistics_get_nboundchgs(
       const libpapilo_statistics_t* statistics );

   /** Get number of side changes. */
   LIBPAPILO_EXPORT size_t
   libpapilo_statistics_get_nsidechgs(
       const libpapilo_statistics_t* statistics );

   /** Get number of coefficient changes. */
   LIBPAPILO_EXPORT size_t
   libpapilo_statistics_get_ncoefchgs(
       const libpapilo_statistics_t* statistics );

   /** Get number of presolve rounds. */
   LIBPAPILO_EXPORT size_t
   libpapilo_statistics_get_nrounds( const libpapilo_statistics_t* statistics );

   /** Get number of deleted columns. */
   LIBPAPILO_EXPORT size_t
   libpapilo_statistics_get_ndeletedcols(
       const libpapilo_statistics_t* statistics );

   /** Get number of deleted rows. */
   LIBPAPILO_EXPORT size_t
   libpapilo_statistics_get_ndeletedrows(
       const libpapilo_statistics_t* statistics );

   /** Get consecutive rounds with only bound changes. */
   LIBPAPILO_EXPORT size_t
   libpapilo_statistics_get_consecutive_rounds_of_only_boundchanges(
       const libpapilo_statistics_t* statistics );

   /** Get single coefficient changes (excludes substitutions/deletions). */
   LIBPAPILO_EXPORT size_t
   libpapilo_statistics_get_single_matrix_coefficient_changes(
       const libpapilo_statistics_t* statistics );

   /* Per-presolver Statistics API */

   /** Get the number of presolvers. */
   LIBPAPILO_EXPORT size_t
   libpapilo_statistics_get_num_presolvers(
       const libpapilo_statistics_t* statistics );

   /** Get the name of a presolver. Returns NULL on error.
    * The returned string is valid as long as statistics exists. */
   LIBPAPILO_EXPORT const char*
   libpapilo_statistics_get_presolver_name(
       const libpapilo_statistics_t* statistics, int presolver_index );

   /** Get the number of calls for a presolver. */
   LIBPAPILO_EXPORT size_t
   libpapilo_statistics_get_presolver_ncalls(
       const libpapilo_statistics_t* statistics, int presolver_index );

   /** Get the number of successful calls for a presolver. */
   LIBPAPILO_EXPORT size_t
   libpapilo_statistics_get_presolver_nsuccessful(
       const libpapilo_statistics_t* statistics, int presolver_index );

   /** Get the total number of transactions generated by a presolver. */
   LIBPAPILO_EXPORT size_t
   libpapilo_statistics_get_presolver_ntransactions(
       const libpapilo_statistics_t* statistics, int presolver_index );

   /** Get the number of applied transactions for a presolver. */
   LIBPAPILO_EXPORT size_t
   libpapilo_statistics_get_presolver_napplied(
       const libpapilo_statistics_t* statistics, int presolver_index );

   /** Get the execution time for a presolver in seconds. Returns -1.0 on error.
    */
   LIBPAPILO_EXPORT double
   libpapilo_statistics_get_presolver_exectime(
       const libpapilo_statistics_t* statistics, int presolver_index );

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

   /* Message control API */
   typedef void ( *libpapilo_trace_callback )( int level, const char* data,
                                               size_t size, void* usr );

   LIBPAPILO_EXPORT void
   libpapilo_message_set_verbosity_level( libpapilo_message_t* message,
                                          int level );

   LIBPAPILO_EXPORT int
   libpapilo_message_get_verbosity_level( const libpapilo_message_t* message );

   LIBPAPILO_EXPORT void
   libpapilo_message_set_output_callback( libpapilo_message_t* message,
                                          libpapilo_trace_callback callback,
                                          void* usr );

   /* Print a message via Message pipeline (no formatting) */
   LIBPAPILO_EXPORT void
   libpapilo_message_print( libpapilo_message_t* message, int level,
                            const char* text );

   /* Presolve Message control: Presolve uses Message passed at create */

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

   LIBPAPILO_EXPORT size_t
   libpapilo_problem_update_get_singleton_cols_count(
       const libpapilo_problem_update_t* update );

   LIBPAPILO_EXPORT libpapilo_reductions_t*
   libpapilo_problem_update_get_reductions(
       const libpapilo_problem_update_t* update );

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
                                  size_t* size );

   LIBPAPILO_EXPORT void
   libpapilo_solution_set_primal( libpapilo_solution_t* solution,
                                  const double* values, size_t size );

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
