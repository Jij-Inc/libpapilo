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
