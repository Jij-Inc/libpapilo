# PaPILO Presolve Internals

This document provides a detailed look into the architecture and mechanics of the presolving process within PaPILO.

## Problem Representation and Modification

A key architectural design choice in PaPILO is that the presolve process modifies the problem representation **in-place**. This means that the input problem and the output (presolved) problem are the **same C++ object**, specifically an instance of `papilo::Problem<REAL>`.

-   **No Separate Output Object**: PaPILO does not create a new `Problem` object to store the presolved result. Instead, it directly alters the data within the original object.
-   **In-Place Modification**: Methods that drive the presolving, such as `Presolve::apply`, take a non-const reference (`Problem<REAL>&`) to the problem object, giving them the authority to modify it directly.
-   **Efficiency**: This approach is highly memory-efficient as it avoids the overhead of copying large problem data structures (like the constraint matrix).
-   **Controlled Changes**: While the modification is in-place, changes are not made arbitrarily. They are managed through a `ProblemUpdate` object, which ensures that all modifications are applied consistently and that the necessary information for postsolving is recorded.

This in-place design is fundamental to understanding how data flows through the presolving pipeline.

### Variable and Constraint Handling

PaPILO employs a hybrid approach to identify and manage variables and constraints, balancing performance with the need to track entities from the original problem.

-   **Active Representation (Index-based)**: During the presolve process, all active variables and constraints are stored in dense arrays (`papilo::Vec<T>`). They are identified solely by their **index** within these arrays. This allows for extremely fast, cache-friendly access to data (e.g., bounds, objective coefficients, row data), which is critical for the performance of the presolvers.

-   **Dynamic Indices and Delayed Compression**: As presolvers fix variables or remove redundant constraints, these entities are marked as inactive using flags (`ColFlag::kInactive`, `RowFlag::kRedundant`). The actual removal from the data arrays is a potentially expensive operation, so it is not performed immediately. Instead, PaPILO uses a delayed execution strategy for this `compress` operation.

    -   **Triggering Condition**: The `compress` operation is triggered only when the number of active entities (rows or columns) drops below a certain threshold relative to the size at the last compression. This is controlled by the `compressfac` parameter (default: 0.8), meaning compression happens when at least 20% of the entities have been marked for deletion. The logic can be found in `ProblemUpdate::check_and_compress()`.
    -   **Execution Point**: This check is performed at the end of each presolve round, inside the `Presolve::finishRound()` method.
    -   **Index Changes**: When `compress` is executed, inactive entities are physically removed, and the remaining ones are packed together. This action **changes the indices** of the still-active variables and constraints. For example, if the variable at index 5 is removed, the variable that was at index 6 now moves to index 5.

-   **Original Identity (Mapping-based)**: To maintain a link back to the user's original problem, PaPILO uses mapping arrays stored in the `PostsolveStorage` object. The `origcol_mapping` and `origrow_mapping` arrays serve as the persistent "identity" layer.
    -   `origcol_mapping[i]` holds the original index of the variable currently at index `i` in the active problem.
    -   These maps are initialized to be an identity mapping (e.g., `origcol_mapping[i] = i`) and are carefully updated during every `compress` operation.

This dual system allows PaPILO to benefit from the raw speed of index-based access during computation while guaranteeing that every reduction can be correctly traced back to the original problem structure for the final postsolve step.

## Presolve Driver Routine

The `Presolve` class in `src/papilo/core/Presolve.hpp` acts as the main driver for the entire presolving process. Its main entry point is the `Presolve::apply()` method.

-   **Automated Presolver Combination**: The `Presolve::addDefaultPresolvers()` method registers a default set of 17 presolvers. Inside `Presolve::apply()`, these are sorted by their `PresolverTiming` (`kFast`, `kMedium`, `kExhaustive`) to create distinct groups for the execution loop.

-   **Execution Flow**: The `Presolve::apply()` method contains the main `do-while` loop that manages the presolving rounds.
    -   The loop's state is controlled by the `round_to_evaluate` enum (`Delegator`), which determines which group of presolvers to run (`kFast`, `kMedium`, or `kExhaustive`).
    -   The private method `run_presolvers()` is called to execute all presolvers within the currently selected group, potentially in parallel using TBB.
    -   After each group execution, `evaluate_and_apply()` is called. This method checks the results from all presolvers and applies the successful reductions to the problem.
    -   Based on the progress made, `determine_next_round()` decides whether to move to the next difficulty level (e.g., from `kFast` to `kMedium`), restart from `kFast` if significant changes were made, or abort if no further progress is detected or limits are reached.

-   **Customization**: While `addDefaultPresolvers()` offers a standard configuration, advanced users can manually add specific presolvers using `Presolve::addPresolveMethod()` to create a custom presolving sequence.

This design allows general users to benefit from a powerful, automated presolving engine by simply calling `apply()`, while still offering fine-grained control to expert users.

## Inter-Presolver Communication
PaPILO's presolvers (PresolveMethods) do not operate in complete isolation. They can communicate and share information, leading to more effective reductions. This communication happens in two primary ways:

-   **Indirect Communication (via Problem State)**: This is the most common form of interaction. When one presolver makes a change to the problem—such as tightening a variable's bounds, removing a redundant constraint, or fixing a variable—it modifies the central `Problem` object. Subsequent presolvers then see this simplified problem, which may enable them to find new reductions that were not possible before. This is an implicit form of communication where the evolving state of the problem itself is the medium.

-   **Direct Communication (via Shared Data Structures)**: For more sophisticated interactions, PaPILO employs shared data structures stored within the `Problem` object. A presolver focused on analysis can populate these structures, and other presolvers can then consume this information to perform more powerful reductions.

A prime example of this direct communication is the use of an **Implication Graph** for binary variables:

1.  **Analysis and Population (`ImplIntDetection`)**: The `ImplIntDetection` presolver analyzes the problem's constraints to find logical relationships between binary variables (e.g., "if variable `x` is 1, then variable `y` must be 0"). It then populates a graph data structure (an `ImplicationGraph`) within the `Problem` object with these findings. This presolver's main role is to generate this analytical data, not necessarily to perform major reductions itself.

2.  **Consumption and Reduction (`Probing`)**: The `Probing` presolver then utilizes this pre-computed `ImplicationGraph`. It can perform "test" assignments (e.g., temporarily setting a binary variable to 1) and rapidly traverse the graph to see all the logical consequences. If a probe leads to a contradiction (e.g., proving that some other variable must be both 0 and 1), the initial assumption must be false, allowing `Probing` to permanently fix the variable to its opposite value.

This cooperative approach—where one presolver analyzes and another utilizes that analysis—allows PaPILO to perform much more complex and powerful reasoning than if each presolver operated independently.

## Implemented Presolvers

This section provides a detailed description of each presolver implemented in PaPILO.

### Singleton Columns (`SingletonCols`)

- **File**: `src/papilo/presolvers/SingletonCols.hpp`
- **Timing**: `kFast`

The `SingletonCols` presolver identifies and processes columns (variables) that appear in only one constraint (row). Such variables are called "singleton columns." The core idea is to either fix the variable to a specific value or substitute it out of the problem, thereby reducing the problem size.

**Key Actions**:

1.  **Equation Handling**: If a singleton column `x` appears in an **equation** (e.g., `a*x + ... = b`), the presolver can often express `x` in terms of other variables.
    -   The variable `x` is substituted out of the objective function.
    -   The constraint is modified. If the variable's bounds are implied by the constraint (making it "implied free"), the entire constraint can be marked as redundant and removed. Otherwise, the constraint is updated to reflect the substitution.
    -   For integer variables, this substitution is only performed if it does not introduce non-integer coefficients into the constraint.

2.  **Inequality and Dual Reductions**: If the singleton column appears in an **inequality**, the logic is more complex.
    -   **Dual Fixing**: Based on the sign of the variable's coefficient in the constraint and its objective function coefficient, the presolver can sometimes perform a "dual fix." This means fixing the variable to its lower or upper bound. For example, if a variable has a positive objective coefficient and appears in a <= constraint with a positive coefficient, it is optimal to set the variable to its lowest possible value (its lower bound).
    -   **Implied Equations**: For continuous variables, the presolver can analyze the dual problem. If the dual information implies that the primal inequality must be binding (i.e., it must hold with equality), the presolver can convert the inequality into an equation. Once it's an equation, the logic from point 1 applies, and the variable can be substituted out.

The `SingletonCols` presolver is a fundamental and effective technique for simplifying linear and integer programs.

### Simple Substitution (`SimpleSubstitution`)

- **File**: `src/papilo/presolvers/SimpleSubstitution.hpp`
- **Timing**: `kMedium`

The `SimpleSubstitution` presolver targets **doubleton equality constraints**, which are equations involving exactly two variables (e.g., `a*x + b*y = c`). Its goal is to substitute one variable out of the problem.

**Key Actions**:

1.  **Identification**: The presolver iterates through the constraints and identifies equations that have exactly two non-zero coefficients.

2.  **Substitution Choice**: When an equation `a*x + b*y = c` is found, a decision is made on which variable to substitute out. This choice is based on several heuristics:
    -   If one variable is integer and the other is continuous, the continuous variable is typically chosen for substitution.
    -   For two integer variables, the substitution is only valid if the coefficient of the variable being substituted divides the other coefficient evenly. For example, in `2x + 4y = c`, `x` can be expressed as `x = (c - 4y) / 2 = c/2 - 2y`. This is a valid substitution for `x`.
    -   If both variables are continuous, the choice is guided by numerical stability (using a Markowitz tolerance) and by which substitution would lead to a sparser constraint matrix overall.

3.  **Aggregation**: Once the variable to be substituted (e.g., `x`) is chosen, the presolver performs an **aggregation**. This doesn't immediately replace `x` in all other constraints. Instead, it records a rule (`x = (c - b*y) / a`) in the `PostsolveStorage`. The variable `x` is then effectively removed from the active problem.

4.  **Bound Tightening**: The substitution also allows for tightening the bounds of the remaining variable (`y`). Since `x` has its own upper and lower bounds, these bounds imply new, potentially tighter bounds on `y` through the equation.

5.  **Feasibility Checks**: For integer variables, the presolver performs checks using the extended Euclidean algorithm to ensure that the equation has a feasible integer solution given the variables' bounds. If not, the problem can be declared infeasible.

This presolver is particularly effective at reducing the number of variables and constraints, simplifying the problem for the main solver.

### Fix Continuous Variables (`FixContinuous`)

- **File**: `src/papilo/presolvers/FixContinuous.hpp`
- **Timing**: `kMedium`

This presolver identifies continuous variables whose lower and upper bounds are very close to each other and fixes them to a single value.

**Key Actions**:

1.  **Identification**: The presolver iterates through all continuous variables.

2.  **Closeness Check**: For each variable, it checks if its lower bound (`lb`) and upper bound (`ub`) are numerically close. The condition for "close" is not just `ub - lb < epsilon`. It also considers the impact of this change. The variable is a candidate for fixing if the maximum possible change in the objective function and any constraint activity is negligible. This is checked by the condition `(ub - lb) * max(|obj_coeff|, |max_matrix_coeff|) <= 0`.

3.  **Fixing**: If a variable is identified as fixable, it is removed from the problem and replaced by a constant value.
    -   The value is chosen to be the lower bound if `floor(ub) == lb`, the upper bound if `ceil(lb) == ub`, or the midpoint `(lb + ub) / 2` otherwise.
    -   A `fixCol` reduction is added to `Reductions`, and the change is applied to the problem.

This is a straightforward but important technique for cleaning up numerical noise and simplifying problems with nearly-fixed continuous variables.

### Free Variable Substitution (`FreeVarSubstitution`)

- **File**: `src/papilo/presolvers/FreeVarSubstitution.hpp` (Class name is `Substitution`)
- **Timing**: `kExhaustive`

This presolver identifies variables that are "implied free" and substitutes them out of the problem. A variable is implied free if its bounds can be removed without changing the feasible region of the problem, because other constraints already enforce those bounds.

**Key Actions**:

1.  **Find Candidate Equations**: The presolver first identifies all equality constraints, as these are the most likely to be useful for creating substitutions.

2.  **Identify Substitution Candidates**: For each equation, it identifies variables that could potentially be substituted. The choice is guided by several factors:
    -   **Sparsity**: It prefers substitutions that are likely to keep the constraint matrix sparse.
    -   **Numerical Stability**: It avoids substitutions that could lead to very large or very small coefficients (poor numerical properties), using Markowitz-style checks.
    -   **Integrality**: For integer variables, a substitution is only valid if it doesn't create fractional coefficients for other integer variables in the row. This is checked by ensuring all coefficients in the equation are divisible by the coefficient of the candidate variable.

3.  **Check for Implied Free Status**: This is the core of the presolver. For a candidate variable `x` in an equation `R1`, the presolver checks if `x`'s bounds are enforced by other constraints in the problem. It iterates through all other constraints `R2, R3, ...` that involve `x` and checks if they collectively imply the original lower and upper bounds of `x`. For example, one constraint might imply `x <= 5` and another might imply `x >= 0`, which together match the original bounds of `x` being `[0, 5]`.

4.  **Perform Aggregation**: If a variable is confirmed to be implied free, the presolver performs an aggregation, similar to `SimpleSubstitution`. It uses the initial equation (`R1`) to express `x` in terms of other variables. This substitution rule is stored, and `x` is eliminated from the problem.

This is an exhaustive and powerful presolver that can significantly reduce the number of variables by finding and eliminating redundancies across multiple constraints.

### Dominated Columns (`DominatedCols`)

- **File**: `src/papilo/presolvers/DominatedCols.hpp`
- **Timing**: `kExhaustive`

This presolver identifies and eliminates "dominated" columns (variables). A variable `x` is dominated by another variable `y` if any feasible solution can be transformed into an equally good or better solution by fixing `x` to one of its bounds.

**Key Concepts**:

-   **Domination Condition**: Roughly, variable `x` is dominated by `y` if:
    1.  `y` is a "cheaper" variable (i.e., has a more favorable objective coefficient).
    2.  Increasing `y` has the same or a more favorable effect on all constraints than increasing `x` does. (This is checked by comparing their column vectors in the constraint matrix).

-   **Implied Free Variables**: This technique is most powerful when one of the variables is *implied free* on one side. For example, if variable `y`'s upper bound is not explicitly stated but is naturally enforced by the problem's constraints, it can dominate another variable `x`, allowing `x` to be fixed.

**Key Actions**:

1.  **Signature Generation**: To quickly find potential dominations, the presolver first computes a "signature" for each column. This signature is a hash that represents the set of constraints the variable appears in and the sign of its coefficient in each.

2.  **Candidate Search**: It then searches for pairs of columns (`x`, `y`) where the signature of `y` suggests it might dominate `x`. This pre-screening step avoids expensive, detailed comparisons for every possible pair of columns.

3.  **Detailed Check**: For candidate pairs, a full check is performed, comparing their objective coefficients and their coefficients in every row of the constraint matrix to confirm the dominance relationship.

4.  **Fixing**: If `y` is confirmed to dominate `x`, variable `x` can be fixed to one of its bounds (either lower or upper, depending on the nature of the domination). A `fixCol` reduction is added.

5.  **Topological Sort**: Since dominations can form chains (e.g., `x` is dominated by `y`, which is dominated by `z`), the reductions are applied in a topological order to avoid conflicts.

This is a strong dual reduction technique that can be computationally expensive, which is why it is run in the `kExhaustive` phase.

### Parallel Row Detection (`ParallelRowDetection`)

- **File**: `src/papilo/presolvers/ParallelRowDetection.hpp`
- **Timing**: `kMedium`

This presolver finds and merges "parallel rows." Two rows (constraints) are considered parallel if they have the same set of variables (support) and their coefficient vectors are linearly dependent (i.e., one is a multiple of the other).

For example, `2x + 4y <= 6` and `3x + 6y <= 9` are parallel rows because their coefficient vectors `(2, 4)` and `(3, 6)` are multiples of each other (`(3, 6) = 1.5 * (2, 4)`).

**Key Actions**:

1.  **Hashing and Bucketing**: To efficiently find parallel rows without comparing every pair, the presolver uses a hashing strategy:
    -   First, it computes a hash based on the **support** (the set of variable indices) of each row. Rows with different supports cannot be parallel.
    -   For rows with the same support hash, it computes a second hash based on the **normalized coefficient values**.
    -   Rows are then sorted into buckets where each bucket contains rows with identical support and coefficient hashes.

2.  **Detailed Comparison**: Within each bucket, the presolver performs a detailed, pairwise comparison to confirm if the rows are truly parallel by checking if their coefficient vectors are multiples of each other.

3.  **Merging and Redundancy**: Once a group of parallel rows is identified, they are merged into a single, stronger constraint.
    -   If two parallel inequalities are `a*x <= b1` and `c*x <= b2`, they are merged into a single inequality `a*x <= min(b1, b2_scaled)`.
    -   If one of the rows is an equality, the other rows might become redundant or the problem might be found to be infeasible if the right-hand sides are inconsistent.
    -   The most restrictive combination of the parallel rows is kept, and the others are marked as redundant and removed.

This presolver helps to remove redundant constraints and tighten the problem formulation.

### Parallel Column Detection (`ParallelColDetection`)

- **File**: `src/papilo/presolvers/ParallelColDetection.hpp`
- **Timing**: `kMedium`

This presolver is the column-based counterpart to `ParallelRowDetection`. It finds sets of "parallel columns" (variables) and aggregates them. Two columns `x` and `y` are parallel if their column vectors in the constraint matrix are linearly dependent (one is a multiple of the other) and their objective coefficients also respect this same ratio.

For example, if `x` and `y` appear in the same set of constraints, and for every constraint the coefficient of `y` is twice the coefficient of `x`, and the objective coefficient of `y` is also twice that of `x`, they are parallel.

**Key Actions**:

1.  **Hashing and Bucketing**: The process is analogous to parallel row detection. Columns are hashed based on their support (the set of rows they appear in) and their coefficient values to be grouped into buckets of potential parallel columns.

2.  **Detailed Comparison**: Within each bucket, a detailed check confirms that the column vectors and objective function coefficients are indeed multiples of each other.

3.  **Aggregation**: If two columns `x` and `y` are found to be parallel (e.g., `y = a*x`), one of them (say, `y`) can be eliminated from the problem. An aggregation is performed:
    -   A new aggregate variable `z` is implicitly created.
    -   The bounds of `z` are calculated based on the bounds of `x` and `y`.
    -   The variable `y` is marked as parallel to `x`, effectively replacing `y` with `a*x` throughout the problem.

4.  **Handling of Integer Variables**: For integer variables, the aggregation is more complex. It is only possible if the scaling factor `a` is an integer and if the domain of the new aggregated variable does not have "holes" (i.e., values it cannot take on).

This presolver is effective for models with highly structured or symmetric components, as it can significantly reduce the number of variables.

### Simplify Inequalities (`SimplifyInequalities`)

- **File**: `src/papilo/presolvers/SimplifyInequalities.hpp`
- **Timing**: `kMedium`

This presolver focuses on simplifying individual inequality constraints involving integer variables. It performs two main types of simplifications:

1.  **Coefficient Removal**: It can sometimes remove variables from a constraint entirely. Consider an inequality `10x + 10y + 3z <= 15` where `x, y, z` are non-negative integer variables. If we know that the maximum possible value of `10x + 10y` is, say, 10 (e.g., if `x` and `y` are binary), then the term `3z` is somewhat "protected" by the slack in the constraint. The presolver analyzes the activity of the row (the range of possible values for the left-hand side) to determine if some variables with small coefficients can be removed without changing the feasible region.

2.  **Greatest Common Divisor (GCD) Tightening**: For a constraint with all-integer variables and coefficients, the left-hand side must always be an integer multiple of the greatest common divisor (GCD) of its coefficients. For example, in `6x + 9y <= 16`, the left-hand side must be a multiple of `gcd(6, 9) = 3`. Therefore, the right-hand side can be tightened from 16 down to the next lowest multiple of 3, which is 15. The constraint becomes `6x + 9y <= 15`.

**Key Actions**:

-   **GCD Calculation**: The presolver computes the GCD of the coefficients in an integer row. It includes heuristics to handle floating-point numbers that are close to integers.
-   **Variable Sorting**: To identify removable variables, it sorts the variables in the constraint by their coefficient magnitude.
-   **Activity Analysis**: It uses pre-computed row activities to check if removing a variable is valid.
-   **RHS/LHS Tightening**: It tightens the right-hand side (for <= constraints) or left-hand side (for >= constraints) based on the GCD.

This presolver is crucial for strengthening the formulation of mixed-integer programs.

### Dual Fixing (`DualFix`)

- **File**: `src/papilo/presolvers/DualFix.hpp`
- **Timing**: `kMedium`

This presolver uses information from the dual of the linear programming relaxation to fix variables to their bounds or to tighten their bounds. The core idea is based on the signs of a variable's coefficients.

**Key Concepts**:

-   **Locking**: For a given variable, a constraint "locks" it in one direction. For example, in a minimization problem, a constraint `... + a_ij * x_j <= ...` where `a_ij > 0` provides an "up-lock" on `x_j`, meaning the constraint pushes `x_j` downwards. The objective function itself also provides a lock (e.g., a positive objective coefficient provides an up-lock, pushing the variable down towards its lower bound).

**Key Actions**:

1.  **Lock Counting**: For each variable, the presolver iterates through all constraints it appears in and counts the number of "up-locks" and "down-locks".

2.  **Dual Fixing**: If a variable has only up-locks and no down-locks, there is no incentive for the variable to be anything other than its lower bound. Therefore, the presolver can **fix the variable to its lower bound**. Conversely, if it only has down-locks, it can be fixed to its upper bound. If the corresponding bound is infinite, the problem may be unbounded or infeasible.

3.  **Bound Tightening**: If a variable has both up-locks and down-locks (so it cannot be fixed outright), the presolver can still use this information to tighten its bounds. By analyzing the activities of the locking constraints, it can sometimes derive a new, tighter bound for the variable. For example, if a variable `x_j` has a positive objective coefficient (an up-lock), the presolver will look at all the down-locking constraints to see if they collectively imply a new, tighter upper bound on `x_j`.

This is a powerful technique for fixing many variables and reducing the size of the feasible region, especially in the early stages of presolving.

### Constraint Propagation (`ConstraintPropagation`)

- **File**: `src/papilo/presolvers/ConstraintPropagation.hpp`
- **Timing**: `kFast`

Constraint propagation is a fundamental presolving technique that tightens variable bounds by using the information from individual constraints. For any given constraint, if all variables but one are at their bounds, the constraint implies a new bound on the remaining variable.

**Key Idea**:

Consider a constraint `a_1*x_1 + a_2*x_2 + ... + a_n*x_n <= b`. We can rearrange this to solve for a single variable, say `x_1`:

`a_1*x_1 <= b - (a_2*x_2 + ... + a_n*x_n)`

By substituting the lower or upper bounds for the other variables (`x_2` to `x_n`) in a way that maximizes the right-hand side, we can compute a new, potentially tighter, upper bound for `x_1`.

**Key Actions**:

1.  **Identify Changed Rows**: This presolver operates on rows whose "activity" has changed. A row's activity changes when the bounds of any variable within that row are tightened.

2.  **Activity Calculation**: For a given row, the presolver calculates the minimum and maximum possible values of the linear expression (its activity range) based on the current bounds of the variables involved.

3.  **Bound Derivation**: It then iterates through each variable in the row. For each variable, it uses the row's activity range and its right-hand side (RHS) and left-hand side (LHS) to calculate an implied bound. For example, for a variable `x_j` in a `<=` constraint, a new upper bound can be derived as:
    `new_ub = (RHS - (min_activity - a_j*x_j_bound)) / a_j`

4.  **Applying Reductions**: If the newly derived bound is tighter than the variable's current bound, a `changeColUB` or `changeColLB` reduction is added. If the new bound is so tight that it crosses the opposite bound, the problem is declared infeasible. If the bounds become equal, the variable is fixed.

Since tightening one variable's bounds can lead to further tightening in other constraints, this process is repeated iteratively until no more improvements can be found.

### Sparsification (`Sparsify`)

- **File**: `src/papilo/presolvers/Sparsify.hpp`
- **Timing**: `kExhaustive`

This presolver aims to reduce the number of non-zero elements in the constraint matrix, making it sparser. It does this by using an equality constraint to cancel out variables in other constraints.

**Key Idea**:

If we have an equality constraint `E: x + y + z = 1`, we can use it to modify another constraint `C: 2x + 3y + 5w <= 10`. By adding a multiple of `E` to `C`, we can eliminate one of the common variables. For example, by subtracting `2*E` from `C`, we get:

`(2x + 3y + 5w) - 2*(x + y + z) <= 10 - 2*1`
`y - 2z + 5w <= 8`

The new constraint has the same number of non-zeros, but if the original constraint `C` had also contained `z`, this operation might have reduced the total number of non-zeros.

**Key Actions**:

1.  **Identify Equalities**: The presolver selects equality constraints to use for sparsification.

2.  **Find Candidate Rows**: For a given equality row `E`, it finds other rows `C` that share a significant number of variables with `E`. This is done efficiently by counting shared variables.

3.  **Find Best Scale**: For each candidate pair `(E, C)`, it determines the best scaling factor `s` to use. The goal is to choose `s` such that adding `s*E` to `C` cancels out the maximum number of variables.

4.  **Apply Sparsification**: If a good scaling factor is found that reduces the number of non-zeros, a `sparsify` reduction is added. This records the equality row, the target row, and the scaling factor. The actual matrix modification is handled by the presolve driver.

This is a computationally intensive presolver, which is why it runs in the `kExhaustive` phase. It can be very effective in simplifying complex, dense constraints.

### Singleton Stuffing (`SingletonStuffing`)

- **File**: `src/papilo/presolvers/SingletonStuffing.hpp`
- **Timing**: `kMedium`

This presolver is an advanced technique that deals with singleton columns in inequalities, particularly those that act as "penalty variables."

**Key Idea**:

Consider an inequality like `C: ... <= b`, and a set of singleton variables `s_1, s_2, ...` that appear only in this constraint. If these variables have objective coefficients that penalize them for moving away from one of their bounds (e.g., a positive objective for a variable `s_i` in a minimization problem, pushing it towards its lower bound), we can sometimes fix some of them.

The presolver calculates the available "slack" in the constraint `C`, assuming all non-singleton variables are at their worst-case bounds. It then sorts the singleton penalty variables by how "expensive" they are (i.e., by their objective coefficient divided by their constraint coefficient). It then "stuffs" the most expensive variables into the constraint one by one, assuming they take their most favorable values, until the slack is consumed. Any remaining penalty variables in the list can then be fixed to their cheapest bounds.

**Key Actions**:

1.  **Identify Penalty Singletons**: The presolver first identifies singleton columns in inequalities where the objective function and the constraint coefficient have opposing signs (e.g., `obj_j > 0` and `a_ij < 0` in a `<=` constraint), which creates a penalty for moving the variable.

2.  **Slack Calculation**: For each row with at least two such penalty variables, it calculates the residual slack, assuming all other variables are at their worst-case values.

3.  **Greedy Stuffing**: It sorts the penalty variables by their cost-effectiveness and greedily uses them to satisfy the constraint until the slack is gone.

4.  **Fixing**: Any penalty variables that were not needed to fill the slack are fixed to their optimal bounds.

This is a powerful dual reduction that can fix variables in situations where simple dual fixing fails.

### Coefficient Strengthening (`CoefficientStrengthening`)

- **File**: `src/papilo/presolvers/CoefficientStrengthening.hpp`
- **Timing**: `kFast`

This technique, also known as coefficient tightening, strengthens the coefficients of integer variables in inequalities. It is based on the idea that if a constraint is not tight, some of its coefficients can be made larger (in absolute value) without cutting off any feasible integer solutions.

**Key Idea**:

Consider an inequality `C: a_i*x_i + ... <= b`, where `x_i` is an integer variable. Let the maximum possible value of the left-hand side, based on the current variable bounds, be `max_activity`. The slack of the constraint is `slack = b - max_activity`. If this slack is positive, the constraint is not tight.

We can strengthen the coefficient `a_i` to a new value `a_i'` such that `|a_i'| > |a_i|`. The amount of strengthening is limited by the slack. Specifically, for a variable `x_i`, its coefficient `a_i` can be strengthened by at most `slack / (ub_i - lb_i)`.

**Key Actions**:

1.  **Select Candidate Rows**: The presolver operates on inequality constraints whose activity has recently changed, indicating that bounds have been tightened and there might be new opportunities for strengthening.

2.  **Normalize Constraint**: It normalizes the constraint to the form `... <= ...`.

3.  **Calculate Maximum Slack**: It calculates `newabscoef = max_activity - rhs`, which represents the maximum amount by which a coefficient can be strengthened.

4.  **Identify Target Coefficients**: It identifies integer variables in the row whose coefficients are smaller in magnitude than `newabscoef`. These are candidates for strengthening.

5.  **Apply Strengthening**: For each candidate variable, its coefficient is increased to `newabscoef` (or `-newabscoef`). The right-hand side of the constraint is then adjusted to compensate for this change, ensuring no feasible solutions are cut off.

This method makes the LP relaxation of the problem tighter, which can lead to better dual bounds and help other presolvers find more reductions.

### Implied Integer Detection (`ImplIntDetection`)

- **File**: `src/papilo/presolvers/ImplIntDetection.hpp`
- **Timing**: `kExhaustive`

This presolver identifies continuous variables that are forced to take on integer values due to the problem structure. These are called "implied integer" variables.

**Key Idea**:

Consider an equation `a_1*x_1 + a_2*x_2 + ... + a_n*x_n = b`, where all variables except `x_1` are known to be integer, and all coefficients `a_2, ..., a_n` and the right-hand side `b` are integer. If we rearrange for `x_1`:

`a_1*x_1 = b - (a_2*x_2 + ... + a_n*x_n)`

The right-hand side of this equation will always be an integer. If the coefficient `a_1` is also an integer (and often, if `a_1` is +/- 1), then `x_1` is forced to be integer-valued, even if it was originally declared as a continuous variable.

**Key Actions**:

1.  **Scan Constraints**: The presolver iterates through the constraints (primarily equations) looking for opportunities to prove implied integrality.

2.  **Check Divisibility**: For a given continuous variable `x_i` in a constraint, it checks if all other variables in that constraint are integer (or already known to be implied integer). It then checks if all coefficients and the right/left-hand side are integer-valued after being scaled by `1/a_i`.

3.  **Mark as Implied Integer**: If these conditions hold, the continuous variable `x_i` is proven to be an implied integer. An `impliedInteger` reduction is added, which changes the variable's type in the problem representation. The `ColFlag::kImplInt` flag is set for that variable.

Detecting implied integer variables is very powerful. Once a variable is known to be integer, it can enable many other integer-specific presolving techniques (like those in `SimplifyInequalities` or `Probing`) to be applied, potentially leading to a cascade of new reductions.

### Probing (`Probing`)

- **File**: `src/papilo/presolvers/Probing.hpp`
- **Timing**: `kExhaustive`

Probing is a powerful technique that explores the consequences of temporarily fixing a binary variable to one of its bounds (0 or 1). By propagating the implications of this temporary assignment, the presolver can discover new fixed variables, tighter bounds, and logical relationships.

**Key Actions**:

1.  **Candidate Selection**: The presolver selects binary variables as probing candidates. The selection is guided by a scoring system that prioritizes variables that are likely to lead to many implications (e.g., those involved in constraints with other binary variables).

2.  **Probing a Variable**: For a candidate variable `x`, the presolver performs two experiments:
    -   **Probe 1**: Temporarily fix `x = 1`. Then, use constraint propagation to see what other bounds can be tightened as a result. This is the "down-implication."
    -   **Probe 2**: Temporarily fix `x = 0`. Propagate again to find the "up-implication."

3.  **Analyzing Implications**:
    -   **Fixing**: If probing `x = 1` leads to infeasibility, then `x` must be 0. The variable is permanently fixed to 0.
    -   **Bound Tightening**: If probing `x = 1` implies that another variable `y` must be `<= 5`, and probing `x = 0` also implies `y <= 5`, then this bound on `y` must hold regardless of the value of `x`. The bound on `y` is permanently tightened.
    -   **Substitutions (Cliques)**: If probing `x = 1` implies `y = 1`, and probing `y = 1` implies `x = 1`, then `x` and `y` are equivalent (`x = y`). One can be substituted out. If `x = 1` implies `y = 0`, this discovers a clique inequality (`x + y <= 1`).

4.  **Execution in Badges**: Probing can be very expensive. To manage the computational cost, PaPILO performs probing in "badges." It selects a small batch of the most promising candidates, probes them, applies the resulting reductions, and then uses the results to select the next badge.

Probing is one of the most effective presolvers for mixed-integer programs, capable of uncovering complex logical relationships that other methods cannot.

### Simple Probing (`SimpleProbing`)

- **File**: `src/papilo/presolvers/SimpleProbing.hpp`
- **Timing**: `kMedium`

This is a lightweight version of probing that targets a very specific structure: an equality constraint where the sum of the minimum possible activities of all variables equals the sum of their maximum possible activities. This implies a rigid structure where if one variable moves from its bound, another must move in a compensatory way.

**Key Idea**:

Consider an equation `a_1*x_1 + ... + a_n*x_n = b` where all variables are at their lower or upper bounds. If `min_activity + max_activity = 2*b`, it implies that the variables are tightly coupled. If one variable `x_i` (which must be binary for this presolver to engage) is flipped from 0 to 1, it forces a specific, predictable change in the other variables to maintain the equality.

**Key Actions**:

1.  **Identify Target Equalities**: The presolver looks for equality constraints that satisfy the `min_activity + max_activity = 2*b` condition.

2.  **Identify Binary Trigger**: Within such an equation, it identifies a binary variable (`x_b`) that can act as a "trigger."

3.  **Derive Substitutions**: The rigid structure of the equation allows other variables (`x_j`) in the constraint to be expressed as a simple linear function of the binary trigger variable: `x_j = factor * x_b + offset`. For example, it might discover that if binary variable `x_b` is 1, then `x_j` must be at its lower bound, and if `x_b` is 0, `x_j` must be at its upper bound.

4.  **Apply `replaceCol` Reduction**: For each such relationship found, the presolver adds a `replaceCol` reduction, effectively eliminating `x_j` from the problem and replacing it with an expression involving the binary variable `x_b`.

This is a much faster, more targeted version of probing that can quickly find and apply these specific types of substitutions.

### Dual Infer (`DualInfer`)

- **File**: `src/papilo/presolvers/DualInfer.hpp`
- **Timing**: `kExhaustive`

This presolver uses the dual of the LP relaxation to infer information about the primal problem. It can fix primal variables and tighten primal constraints (by turning inequalities into equalities).

**Key Actions**:

1.  **Construct Dual Problem**: The presolver implicitly constructs the dual problem. The dual variables correspond to the primal constraints, and the dual constraints correspond to the primal variables.

2.  **Dual Propagation**: It performs constraint propagation on this dual problem. By tightening the bounds of the dual variables, it gathers information about the primal.

3.  **Inferring Primal Information**:
    -   **Fixing Primal Variables**: The presolver calculates the reduced costs of the primal variables using the propagated dual bounds. If the reduced cost of a variable is strictly positive, the variable can be fixed to its lower bound. If it's strictly negative, it can be fixed to its upper bound.
    -   **Tightening Primal Constraints**: If the propagation on the dual problem shows that a dual variable corresponding to a primal inequality `C` must be non-zero, then by the theory of complementary slackness, the primal constraint `C` must be binding (i.e., it must hold with equality). The presolver then changes the inequality to an equality.

This is a powerful, globally-aware presolver that can find reductions by reasoning about the entire problem structure through the lens of the dual problem.
