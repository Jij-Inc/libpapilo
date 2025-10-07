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

### Ordering Constraints

The reductions are undone strictly in reverse chronological order because many reconstructions depend on values restored by later log entries. Typical dependencies:

-   **Substitution-style reductions** (`kSubstitutedCol`, `kSubstitutedColWithDual`) evaluate stored linear expressions that reference other variables. Those variables must already hold their original values, which is guaranteed only when the log is processed backwards.
-   **Parallel or infinity fixes** (`kParallelCol`, `kFixedInfCol`) emit follow-up entries such as `kRedundantRow` that must remain adjacent so the undo logic can skip already handled rows without clobbering basis information.
-   **Bound propagation** (`kVarBoundChange`, `kRowBoundChange`, `kRowBoundChangeForcedByRow`) mutates the shared `BoundStorage`. Earlier (in presolve) steps tightened bounds based on later steps, so the inverse operations must relax them in exactly the opposite order to keep complementary slackness checks valid.
-   **Auxiliary payloads** (`kSaveRow`, `kReasonForRowBoundChangeForcedByRow`, `kCoefficientChange`) carry data that is consumed immediately by the next (earlier in presolve time) reduction. They have no standalone undo code, so reordering would orphan that context.

With these invariants in mind, the following subsections document the mathematics each undo routine performs.

---

#### `kFixedCol`
-   **Presolve Action**: A variable was fixed to a constant value.
-   **Undo Logic (`apply_fix_var_in_original_solution`)**:
    -   **Primal**: Sets the solution value for the fixed variable: `original_solution.primal[col] = val`.
    -   **Dual**: Calculates the reduced cost of the fixed variable using its original objective coefficient and the dual values of the rows it participated in (which were stored with the reduction): `reduced_cost = c_j - sum(a_ij * y_i)`.
    -   **Mathematics**: Denote the fixed value by `x_j^*`. The log stores `c_j` and all `(i, a_{ij})` pairs for rows containing column `j`. The primal undo sets `x_j := x_j^*`. The dual undo recomputes the reduced cost `\bar{c}_j = c_j - A_j^\top y`, where `y` are the current row duals.\*
    -   **Order Dependency**: Independent of other columns once their dual values are known.

---

#### `kFixedInfCol`
-   **Presolve Action**: A variable with an infinite bound was fixed (e.g., a variable that could be increased indefinitely without violating constraints was fixed to 0 if its objective was 0).
-   **Undo Logic (`apply_fix_infinity_variable_in_original_solution`)**:
    -   **Primal**: Calculates the variable's primal value. This is done by checking all the constraints it was involved in (which were saved) and determining which one is most binding, given the current (partially postsolved) solution values of other variables.
    -   **Dual**: Calculates the reduced cost, which should be zero if the variable was correctly identified as unbounded.
    -   **Mathematics**: For each stored row `i`, PaPILO reconstructs the residual `r_i = b_i - \sum_{k \neq j} a_{ik} x_k`. The value of the fixed-infinity column is the tightest compatible value, e.g. `x_j = r_i / a_{ij}` for the row that attains the minimum/maximum depending on the sign of `a_{ij}`. The reduction also records redundant rows that have to be skipped immediately afterwards.
    -   **Order Dependency**: Requires that every neighbour `k` already has its final `x_k` because the residuals depend on them; it also expects the subsequent `kRedundantRow` entries to remain adjacent.

---

#### `kParallelCol`
-   **Presolve Action**: Two parallel columns (`col1`, `col2`) were merged into a single representative column (`y`), such that `y = col2 + scale * col1`.
-   **Undo Logic (`apply_parallel_col_to_original_solution`)**:
    -   **Primal**: Disaggregates the solution value of the merged column (`y_sol`) back into `col1_sol` and `col2_sol`. This is a small subproblem to find feasible values for `col1` and `col2` that satisfy the aggregation equation and their original bounds. The logic typically sets one variable to its bound and calculates the other.
    -   **Dual**: Disaggregates the reduced costs based on the same linear relationship.
    -   **Mathematics**: Given `y = x_{2} + \lambda x_{1}` and bounds `l_1 \le x_1 \le u_1`, `l_2 \le x_2 \le u_2`, the undo chooses a feasible pair that satisfies the affine equation. The stored payload contains `\lambda`, objective coefficients, and the original bounds, allowing the routine to solve `x_1 = \operatorname{clip}((y - x_2)/\lambda)` and `x_2 = y - \lambda x_1`, taking a bound when necessary to keep both values feasible.
    -   **Order Dependency**: Needs `y`'s value from the reduced solution before recovering `x_1` and `x_2`. No other columns depend on this entry, but the row basis status updates rely on the immediate context.

---

#### `kSubstitutedColWithDual`
-   **Presolve Action**: A variable was eliminated via substitution using an equality constraint, with full information stored to reconstruct dual values.
-   **Undo Logic (`apply_substituted_column_to_original_solution`)**:
    -   **Primal**: Reconstructs the variable's value by solving the stored equation (`sum(a_i * x_i) = rhs`) for the substituted variable, using the known values of the other variables.
    -   **Dual**: This is a complex operation. Depending on whether the reconstructed primal value is at one of its original bounds, the logic either updates the dual value of the row used for substitution or the reduced cost of the substituted variable itself, ensuring dual feasibility is maintained.
    -   **Mathematics**: The stored row provides coefficients `(a_{ij})_{j \in N}` and the right-hand side `b`. The undo computes `x_p = (b - \sum_{j \ne p} a_{ij} x_j) / a_{ip}` for the eliminated column `p`, so every `x_j` with `j \ne p` must already be reconstructed. Dual-wise, complementary slackness is enforced by checking whether `x_p` hits `l_p` or `u_p`. If so, the row dual `y_i` is updated; otherwise the reduced cost `\bar{c}_p` is restored from `\bar{c}_p = c_p - A_p^\top y`.
    -   **Order Dependency**: Strong—requires all other variables in the equation (and any subsequent `kCoefficientChange`/`kRowBoundChange` context) to be available. The routine also consumes payloads of adjacent `kCoefficientChange`, `kRowBoundChange`, or `kReasonForRowBoundChangeForcedByRow` entries.

---

#### `kSubstitutedCol`
-   **Presolve Action**: A variable was eliminated via substitution. This is a simpler, primal-only version.
-   **Undo Logic** (implemented directly in the `switch` statement):
    -   **Primal**: Calculates the variable's value by evaluating the stored linear expression. The logic is identical to the primal part of `kSubstitutedColWithDual`.
    -   **Dual**: None.
    -   **Mathematics**: Same as above but restricted to primal feasibility: solve a stored affine equality `a_{ip} x_p + \sum_{j \ne p} a_{ij} x_j = b` for `x_p`.
    -   **Order Dependency**: Same requirement that the neighbour `x_j` values already exist when this case executes.

---

#### `kVarBoundChange`
-   **Presolve Action**: The bound of a variable was tightened.
-   **Undo Logic (`apply_var_bound_change_forced_by_column_in_original_solution`)**:
    -   **Primal**: None. The primal solution value is not changed by this step.
    -   **Dual**: This is a critical step. The original, looser bound is restored in the `BoundStorage` object. If the solution value of the variable was on the tightened bound, this undo operation may trigger a recalculation of dual values for the row that originally caused the bound change.
    -   **Mathematics**: The stored payload contains the original bounds `(l_j^{old}, u_j^{old})`, the tightened ones, and the row that implied the change. Undo restores `(l_j^{old}, u_j^{old})` in `BoundStorage` so that later dual computations can use the correct slack `s_j = x_j - l_j` (or `u_j - x_j`) when checking optimality. If the variable sat on the tight bound, the row dual is adjusted using the saved multiplier.
    -   **Order Dependency**: Tightened bounds often chained across presolve steps; undoing them out of order would mix incompatible bound states and break the complementary slackness logic.

---

#### `kRedundantRow`
-   **Presolve Action**: A constraint was found to be redundant and was removed.
-   **Undo Logic**:
    -   **Primal**: None.
    -   **Dual**: Re-establishes the row's existence. Its dual value is set to zero, and if basis information is tracked, its `rowBasisStatus` is set to `BASIC`.
    -   **Mathematics**: Restores row `i` with dual `y_i := 0`. If the basis is available, the row basis status becomes `BASIC` so that the simplex tableau remains consistent.
    -   **Order Dependency**: Frequently follows `kFixedInfCol` to skip rows that assisted in the infinity fix; therefore it must appear immediately after the fix in the log.

---

#### `kRowBoundChange`
-   **Presolve Action**: The bound (LHS or RHS) of a constraint was changed.
-   **Undo Logic**:
    -   **Primal**: None.
    -   **Dual**: Restores the original row bound in the `BoundStorage`. This can also trigger an update to the row's basis status (e.g., from `ON_LOWER` to `BASIC`).
    -   **Mathematics**: Restores `(lhs_i^{old}, rhs_i^{old})`. When basis data exists, PaPILO recomputes the row basis flag depending on whether the row remains active (`ON_LOWER`, `ON_UPPER`) or returns to `BASIC`. Complementary slackness uses the restored bounds to evaluate slack `b_i - A_i x`.
    -   **Order Dependency**: Must be processed after any substitutions or coefficient updates that referenced the same row, hence the guard `skip_if_row_bound_belongs_to_substitution`.

---

#### `kRowBoundChangeForcedByRow`
-   **Presolve Action**: A row's bound was changed as a direct consequence of another row's modification (e.g., in parallel row reductions).
-   **Undo Logic (`apply_row_bound_change_to_original_solution`)**:
    -   **Primal**: None.
    -   **Dual**: A key dual operation that transfers dual values. If row `R1`'s bound change was forced by row `R2`, the dual value of `R1` is used to calculate the dual value of `R2` using a stored scaling factor.
    -   **Mathematics**: Suppose presolve recorded that row `r_2` was aggregated into row `r_1` with coefficient `\theta`. Undo reconstructs `y_{r_2} = \theta \cdot y_{r_1}` (or the inverse, depending on the stored orientation) and restores the original bounds of `r_2`. The necessary tuple `(r_1, r_2, \theta)` lives in an adjacent `kReasonForRowBoundChangeForcedByRow` entry.
    -   **Order Dependency**: Strong—the reduction consumes the auxiliary "reason" entry immediately preceding it in the log. Swapping steps would decouple that data.

---

### Auxiliary Reduction Types

Several `ReductionType`s exist not to be undone directly, but to provide data for other undo operations. They are processed in the loop, but their `case` statements are often empty as their data is consumed by a subsequent (earlier in the log) reduction's undo logic.

-   **`kReasonForRowBoundChangeForcedByRow`**: Stores the "reason" (the other row and a scaling factor) for a `kRowBoundChangeForcedByRow` reduction. It is read by the `kRowBoundChangeForcedByRow` undo logic.
-   **`kSaveRow`**: Stores the state of a row before modification. This data is used by operations like `kVarBoundChange` to know which row was responsible for a bound change and what its coefficients were.
-   **`kReducedBoundsCost`**: This is one of the last records processed. It populates the `BoundStorage` helper object with the final bounds and objective coefficients of the fully reduced problem, providing the necessary starting context for all subsequent dual calculations.
-   **Mathematics**: Rehydrates every column/row bound pair `(l_j, u_j)` and `(lhs_i, rhs_i)` along with flags indicating infinite bounds. The entries initialise `BoundStorage`, effectively seeding the dual postsolve with the reduced problem's end-state data. Because the main loop runs backwards, this entry is encountered first and thus provides the initial conditions for the rest of the undo operations.
-   **`kColumnDualValue` / `kRowDualValue`**: Directly sets a dual value (reduced cost for a column, or shadow price for a row) that was determined during presolve.
-   **Mathematics**: Simple assignments of the form `\bar{c}_j := \bar{c}_j^{stored}` or `y_i := y_i^{stored}`. They depend on the surrounding context only through the index mapping.
-   **`kCoefficientChange`**: Records a change to a single matrix coefficient. It has no direct undo action but provides context for other dual calculations.
-   **Mathematics**: Supplies the coefficient `a_{ij}` that was modified during presolve so that substitution or bound-change undos can reconstruct the original constraint row when recomputing residuals. These entries are intentionally consumed by the adjacent undo routines, hence the empty switch case.

\* Notation: `A_j` denotes column `j` of the original constraint matrix, `c_j` the objective coefficient, `b` the right-hand side vector, and `y` dual multipliers.
