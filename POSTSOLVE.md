# PaPILO Postsolve Specification (Complete & Detailed)

This document provides a complete specification of the Postsolve process in PaPILO. It is intended for developers who may need to re-implement the Postsolve logic outside of the C++ library, for example, in a different programming language using the C API's data access functions.

**Primary Implementation File**: `src/papilo/core/postsolve/Postsolve.hpp`

## 1. Core Concept

Postsolve is the process of transforming a solution from the reduced (presolved) problem space back into the original problem space.

The entire process is **data-driven**. It relies on a log of "reductions" that were generated during the presolve phase. These reductions are stored in a LIFO (Last-In, First-Out) manner. The Postsolve process simply reads this log in reverse order and applies the inverse of each reduction.

**Implementation Reference**: The main entry point is the `Postsolve<REAL>::undo(...)` method in `src/papilo/core/postsolve/Postsolve.hpp`. This method contains the main `for` loop that iterates backwards and a `switch` statement that dispatches to the appropriate logic for each `ReductionType`.

## 2. Data Structures

An external implementation must access the following data from the `PostsolveStorage` object:

-   `types`: An array of `ReductionType` enums.
-   `indices`, `values`, `start`: Arrays storing the detailed data for each reduction.
-   `origcol_mapping`: Map from reduced column indices to original column indices.

These data structures are defined in `src/papilo/core/postsolve/PostsolveStorage.hpp`.

## 3. The Postsolve Algorithm

The algorithm proceeds as follows:

1.  **Initialization**:
    -   Create an `original_solution` array of size `nColsOriginal`, initialized to zero.
    -   Create `original_reduced_costs` and `original_dual` arrays if dual values are needed.
    -   **Implementation Reference**: `Postsolve<REAL>::copy_from_reduced_to_original(...)` in `src/papilo/core/postsolve/Postsolve.hpp`.
2.  **Initial Solution Mapping**:
    -   Copy the solution values from the `reduced_solution` to the `original_solution` using the `origcol_mapping`.
3.  **Iterate Through Reductions (in Reverse)**:
    -   Loop `i` from `types.size() - 1` down to `0`.
    -   For each `i`, get the `ReductionType` as `type = types[i]`.
    -   Get the data slice for this reduction using `first = start[i]` and `last = start[i+1]`.
    -   Execute the undo operation corresponding to `type`.
    -   **Implementation Reference**: The main `for` loop and `switch` statement within `Postsolve<REAL>::undo(...)`.

## 4. Undo Operations by `ReductionType`

This section details the undo operation for every `ReductionType` defined in `src/papilo/core/postsolve/ReductionType.hpp`.

---

#### `kFixedCol`

-   **Purpose**: A variable was fixed to a constant value during presolve.
-   **Undo Action**:
    -   **Primal**: Sets the solution value for the fixed variable. `original_solution[col] = val`.
    -   **Dual**: Calculates the reduced cost of the fixed variable using its original objective coefficient and the dual values of the rows it participated in. `reduced_cost = c_j - sum(a_ij * y_i)`.
-   **Implementation**:
    -   **Dispatch**: `case ReductionType::kFixedCol:` in `Postsolve::undo` (src/papilo/core/postsolve/Postsolve.hpp).
    -   **Logic**: `Postsolve<REAL>::apply_fix_var_in_original_solution(...)` in `src/papilo/core/postsolve/Postsolve.hpp`.

---

#### `kFixedInfCol`

-   **Purpose**: A variable, determined to be unbounded, was fixed.
-   **Undo Action**:
    -   **Primal**: Calculates the variable's primal value based on the most binding constraint it appears in, given the current (partially postsolved) solution values of other variables.
    -   **Dual**: Calculates the reduced cost (which should be zero if correctly identified as unbounded).
-   **Implementation**:
    -   **Dispatch**: `case ReductionType::kFixedInfCol:` in `Postsolve::undo` (src/papilo/core/postsolve/Postsolve.hpp).
    -   **Logic**: `Postsolve<REAL>::apply_fix_infinity_variable_in_original_solution(...)` in `src/papilo/core/postsolve/Postsolve.hpp`.

---

#### `kParallelCol`

-   **Purpose**: Two parallel columns (`col1`, `col2`) were merged into a single representative column (`y`).
-   **Undo Action**:
    -   **Primal**: Disaggregates the solution value of the merged column (`y_sol`) back into `col1_sol` and `col2_sol`, ensuring `y_sol = col2_sol + scale * col1_sol` and that both values respect their original bounds. This is a small feasibility problem.
    -   **Dual**: Disaggregates the reduced costs based on the same relationship.
-   **Implementation**:
    -   **Dispatch**: `case ReductionType::kParallelCol:` in `Postsolve::undo` (src/papilo/core/postsolve/Postsolve.hpp).
    -   **Logic**: `Postsolve<REAL>::apply_parallel_col_to_original_solution(...)` in `src/papilo/core/postsolve/Postsolve.hpp`.

---

#### `kSubstitutedColWithDual`

-   **Purpose**: A variable was eliminated via substitution, with full information stored to reconstruct dual values.
-   **Undo Action**:
    -   **Primal**: Reconstructs the variable's value by solving the stored equation (`sum(a_i * x_i) = rhs`) for the substituted variable, using the known values of the other `x_i`.
    -   **Dual**: A complex operation that reconstructs the dual values associated with the substitution, potentially updating both row duals and variable reduced costs based on the original variable bounds and equation.
-   **Implementation**:
    -   **Dispatch**: `case ReductionType::kSubstitutedColWithDual:` in `Postsolve::undo` (src/papilo/core/postsolve/Postsolve.hpp).
    -   **Logic**: `Postsolve<REAL>::apply_substituted_column_to_original_solution(...)` in `src/papilo/core/postsolve/Postsolve.hpp`.

---

#### `kSubstitutedCol`

-   **Purpose**: A variable was eliminated via substitution. This is a simpler, primal-only version.
-   **Undo Action**:
    -   **Primal**: Calculates the variable's value by evaluating the stored linear expression. The logic is identical to the primal part of `kSubstitutedColWithDual`.
    -   **Dual**: None.
-   **Implementation**:
    -   **Dispatch**: `case ReductionType::kSubstitutedCol:` in `Postsolve::undo` (src/papilo/core/postsolve/Postsolve.hpp).
    -   **Logic**: Implemented directly within the `case` block in `src/papilo/core/postsolve/Postsolve.hpp`.

---

#### `kVarBoundChange`

-   **Purpose**: The bound of a variable was tightened.
-   **Undo Action**:
    -   **Primal**: None. The primal solution value is not changed by this step.
    -   **Dual**: Restores the original, looser bound in a temporary `BoundStorage` object (defined in `src/papilo/core/postsolve/BoundStorage.hpp`). This is critical context for subsequent dual calculations, as the tightness of a bound affects dual values and basis statuses.
-   **Implementation**:
    -   **Dispatch**: `case ReductionType::kVarBoundChange:` in `Postsolve::undo` (src/papilo/core/postsolve/Postsolve.hpp).
    -   **Logic**: `Postsolve<REAL>::apply_var_bound_change_forced_by_column_in_original_solution(...)` in `src/papilo/core/postsolve/Postsolve.hpp`.

---

#### `kRedundantRow`

-   **Purpose**: A constraint was found to be redundant and was removed.
-   **Undo Action**:
    -   **Primal**: None.
    -   **Dual**: Re-establishes the row's existence in the basis representation, typically by marking its `rowBasisStatus` as `BASIC`. Its dual value is assumed to be zero unless other reductions modify it.
-   **Implementation**:
    -   **Dispatch**: `case ReductionType::kRedundantRow:` in `Postsolve::undo` (src/papilo/core/postsolve/Postsolve.hpp).
    -   **Logic**: A few lines of code directly within the `case` block in `src/papilo/core/postsolve/Postsolve.hpp`.

---

#### `kRowBoundChange`

-   **Purpose**: The bound (LHS or RHS) of a constraint was changed.
-   **Undo Action**:
    -   **Primal**: None.
    -   **Dual**: Restores the original row bound in the `BoundStorage` (defined in `src/papilo/core/postsolve/BoundStorage.hpp`) and may update the row's basis status (e.g., from `ON_LOWER` to `BASIC`).
-   **Implementation**:
    -   **Dispatch**: `case ReductionType::kRowBoundChange:` in `Postsolve::undo` (src/papilo/core/postsolve/Postsolve.hpp).
    -   **Logic**: Implemented directly within the `case` block in `src/papilo/core/postsolve/Postsolve.hpp`.

---

#### `kRowBoundChangeForcedByRow`

-   **Purpose**: A row's bound was changed as a direct consequence of another row's modification.
-   **Undo Action**:
    -   **Primal**: None.
    -   **Dual**: A key dual operation that transfers dual values. If row `R1`'s bound change was forced by row `R2`, the dual value of `R1` is used to calculate the dual value of `R2` using a stored scaling factor.
-   **Implementation**:
    -   **Dispatch**: `case ReductionType::kRowBoundChangeForcedByRow:` in `Postsolve::undo` (src/papilo/core/postsolve/Postsolve.hpp).
    -   **Logic**: `Postsolve<REAL>::apply_row_bound_change_to_original_solution(...)` in `src/papilo/core/postsolve/Postsolve.hpp`.

---

#### `kReasonForRowBoundChangeForcedByRow`

-   **Purpose**: **Auxiliary record**. Stores the "reason" (the other row and a scaling factor) for a `kRowBoundChangeForcedByRow` reduction.
-   **Undo Action**: None. This record is data read by the `kRowBoundChangeForcedByRow` undo logic, which immediately precedes it in the reverse loop.
-   **Implementation**: The `case` statement is empty.

---

#### `kSaveRow`

-   **Purpose**: **Auxiliary record**. Saves the state of a row before modification. The saved row data structure is defined in `src/papilo/core/postsolve/SavedRow.hpp`.
-   **Undo Action**: None. This record is data read by other undo operations.
-   **Implementation**: The `case` statement is empty.

---

#### `kReducedBoundsCost`

-   **Purpose**: **Auxiliary record**. Stores the final bounds and objective coefficients of the fully reduced problem. It is one of the first records created (and thus last processed in postsolve).
-   **Undo Action**: Populates an internal `BoundStorage` helper object (defined in `src/papilo/core/postsolve/BoundStorage.hpp`) with the final reduced bounds. This provides the necessary starting context for all subsequent dual calculations.
-   **Implementation**:
    -   **Dispatch**: `case ReductionType::kReducedBoundsCost:` in `Postsolve::undo` (src/papilo/core/postsolve/Postsolve.hpp).
    -   **Logic**: Implemented directly within the `case` block in `src/papilo/core/postsolve/Postsolve.hpp`.

---

#### `kColumnDualValue` / `kRowDualValue`

-   **Purpose**: A dual value (reduced cost for a column, or shadow price for a row) was determined directly during presolve.
-   **Undo Action**: Directly sets the corresponding value in the solution's `reducedCosts` or `dual` array.
-   **Implementation**:
    -   **Dispatch**: `case ReductionType::kColumnDualValue:` and `case ReductionType::kRowDualValue:` in `Postsolve::undo` (src/papilo/core/postsolve/Postsolve.hpp).
    -   **Logic**: A trivial assignment inside the `case` blocks in `src/papilo/core/postsolve/Postsolve.hpp`.

---

#### `kCoefficientChange`

-   **Purpose**: A single coefficient in the constraint matrix was changed.
-   **Undo Action**:
    -   **Primal**: None.
    -   **Dual**: None directly. This is auxiliary information that could be used by other dual calculations, but in the current implementation, it does not trigger a direct action.
-   **Implementation**: The `case` statement is empty or contains a `continue`.