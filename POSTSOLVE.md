# PaPILO Postsolve Specification (Complete & Detailed)

This document provides a complete, implementation-level specification of the Postsolve process in PaPILO. It is intended for developers who need to understand or re-implement the Postsolve logic, for example, when using the C API.

**Primary Implementation File**: `src/papilo/core/postsolve/Postsolve.hpp`

## 1. Core Concept

Postsolve is the process of transforming a solution from the reduced (presolved) problem space back into the original problem space. The entire process is **data-driven**, relying on a log of "reductions" generated during the presolve phase. These reductions are stored in a LIFO (Last-In, First-Out) manner in the `PostsolveStorage` object. The Postsolve process simply reads this log in reverse order and applies the inverse of each reduction.

**Implementation Reference**: The main entry point is the `Postsolve<REAL>::undo(...)` method. This method contains the main `for` loop that iterates backwards through the reductions and a `switch` statement that dispatches to the appropriate logic for each `ReductionType`.

## 2. Data Structures

An external implementation must access the following data from the `PostsolveStorage` object (defined in `src/papilo/core/postsolve/PostsolveStorage.hpp`):

-   `types`: An array of `ReductionType` enums identifying each change.
-   `indices`, `values`, `start`: These arrays store the detailed data for each reduction in a compressed format. `start[i]` gives the starting index for the i-th reduction's data within the `indices` and `values` arrays.
-   `origcol_mapping`, `origrow_mapping`: Maps from the final reduced problem's indices back to the original problem's indices.

## 3. The Postsolve Algorithm

The algorithm, found in `Postsolve::undo`, proceeds as follows:

1.  **Initialization**:
    -   An `originalSolution` object is created.
    -   The solution from the reduced problem is copied into the `originalSolution` using the `origcol_mapping` and `origrow_mapping` to place values in their correct original slots. (See: `Postsolve::copy_from_reduced_to_original`).
    -   A `BoundStorage` helper object is created. This is critical for dual postsolve, as it tracks the current state of all variable and row bounds as they are progressively restored to their original values.

2.  **Iterate Through Reductions (in Reverse)**:
    -   The code loops from `i = types.size() - 1` down to `0`.
    -   For each `i`, it reads the `ReductionType` and its associated data.
    -   It executes the corresponding undo operation via a `switch` statement.

## 4. Undo Operations by `ReductionType`

This section details the undo operation for every `ReductionType` defined in `src/papilo/core/postsolve/ReductionType.hpp`.

---

#### `kFixedCol`
-   **Presolve Action**: A variable was fixed to a constant value.
-   **Undo Logic (`apply_fix_var_in_original_solution`)**:
    -   **Primal**: Sets the solution value for the fixed variable: `original_solution.primal[col] = val`.
    -   **Dual**: Calculates the reduced cost of the fixed variable using its original objective coefficient and the dual values of the rows it participated in (which were stored with the reduction): `reduced_cost = c_j - sum(a_ij * y_i)`.

---

#### `kFixedInfCol`
-   **Presolve Action**: A variable with an infinite bound was fixed (e.g., a variable that could be increased indefinitely without violating constraints was fixed to 0 if its objective was 0).
-   **Undo Logic (`apply_fix_infinity_variable_in_original_solution`)**:
    -   **Primal**: Calculates the variable's primal value. This is done by checking all the constraints it was involved in (which were saved) and determining which one is most binding, given the current (partially postsolved) solution values of other variables.
    -   **Dual**: Calculates the reduced cost, which should be zero if the variable was correctly identified as unbounded.

---

#### `kParallelCol`
-   **Presolve Action**: Two parallel columns (`col1`, `col2`) were merged into a single representative column (`y`), such that `y = col2 + scale * col1`.
-   **Undo Logic (`apply_parallel_col_to_original_solution`)**:
    -   **Primal**: Disaggregates the solution value of the merged column (`y_sol`) back into `col1_sol` and `col2_sol`. This is a small subproblem to find feasible values for `col1` and `col2` that satisfy the aggregation equation and their original bounds. The logic typically sets one variable to its bound and calculates the other.
    -   **Dual**: Disaggregates the reduced costs based on the same linear relationship.

---

#### `kSubstitutedColWithDual`
-   **Presolve Action**: A variable was eliminated via substitution using an equality constraint, with full information stored to reconstruct dual values.
-   **Undo Logic (`apply_substituted_column_to_original_solution`)**:
    -   **Primal**: Reconstructs the variable's value by solving the stored equation (`sum(a_i * x_i) = rhs`) for the substituted variable, using the known values of the other variables.
    -   **Dual**: This is a complex operation. Depending on whether the reconstructed primal value is at one of its original bounds, the logic either updates the dual value of the row used for substitution or the reduced cost of the substituted variable itself, ensuring dual feasibility is maintained.

---

#### `kSubstitutedCol`
-   **Presolve Action**: A variable was eliminated via substitution. This is a simpler, primal-only version.
-   **Undo Logic** (implemented directly in the `switch` statement):
    -   **Primal**: Calculates the variable's value by evaluating the stored linear expression. The logic is identical to the primal part of `kSubstitutedColWithDual`.
    -   **Dual**: None.

---

#### `kVarBoundChange`
-   **Presolve Action**: The bound of a variable was tightened.
-   **Undo Logic (`apply_var_bound_change_forced_by_column_in_original_solution`)**:
    -   **Primal**: None. The primal solution value is not changed by this step.
    -   **Dual**: This is a critical step. The original, looser bound is restored in the `BoundStorage` object. If the solution value of the variable was on the tightened bound, this undo operation may trigger a recalculation of dual values for the row that originally caused the bound change.

---

#### `kRedundantRow`
-   **Presolve Action**: A constraint was found to be redundant and was removed.
-   **Undo Logic**:
    -   **Primal**: None.
    -   **Dual**: Re-establishes the row's existence. Its dual value is set to zero, and if basis information is tracked, its `rowBasisStatus` is set to `BASIC`.

---

#### `kRowBoundChange`
-   **Presolve Action**: The bound (LHS or RHS) of a constraint was changed.
-   **Undo Logic**:
    -   **Primal**: None.
    -   **Dual**: Restores the original row bound in the `BoundStorage`. This can also trigger an update to the row's basis status (e.g., from `ON_LOWER` to `BASIC`).

---

#### `kRowBoundChangeForcedByRow`
-   **Presolve Action**: A row's bound was changed as a direct consequence of another row's modification (e.g., in parallel row reductions).
-   **Undo Logic (`apply_row_bound_change_to_original_solution`)**:
    -   **Primal**: None.
    -   **Dual**: A key dual operation that transfers dual values. If row `R1`'s bound change was forced by row `R2`, the dual value of `R1` is used to calculate the dual value of `R2` using a stored scaling factor.

---

### Auxiliary Reduction Types

Several `ReductionType`s exist not to be undone directly, but to provide data for other undo operations. They are processed in the loop, but their `case` statements are often empty as their data is consumed by a subsequent (earlier in the log) reduction's undo logic.

-   **`kReasonForRowBoundChangeForcedByRow`**: Stores the "reason" (the other row and a scaling factor) for a `kRowBoundChangeForcedByRow` reduction. It is read by the `kRowBoundChangeForcedByRow` undo logic.
-   **`kSaveRow`**: Stores the state of a row before modification. This data is used by operations like `kVarBoundChange` to know which row was responsible for a bound change and what its coefficients were.
-   **`kReducedBoundsCost`**: This is one of the last records processed. It populates the `BoundStorage` helper object with the final bounds and objective coefficients of the fully reduced problem, providing the necessary starting context for all subsequent dual calculations.
-   **`kColumnDualValue` / `kRowDualValue`**: Directly sets a dual value (reduced cost for a column, or shadow price for a row) that was determined during presolve.
-   **`kCoefficientChange`**: Records a change to a single matrix coefficient. It has no direct undo action but provides context for other dual calculations.