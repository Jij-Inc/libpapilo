# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

libpapilo is a fork of scipopt/papilo that aims to provide PaPILO (Parallel Presolve for Integer and Linear Optimization) as a **shared library with a C API**. While the original PaPILO is implemented in C++, this fork's goal is to make the presolving functionality accessible from other programming languages through a stable C interface.

**Key Design Decisions:**
- **Presolving only**: No solver integrations (SCIP, Gurobi, etc.) - users solve with their own choice of solver
- **Double precision only**: Simplifies C API by avoiding C++ template complexity (original supports double/quadruple/rational)
- **Fresh fork**: No development yet - starting from clean slate

The original PaPILO is a C++14-based presolve package for (mixed integer) linear programming problems with support for parallel execution and multiple precision arithmetic, licensed under LGPLv3.

## Development Strategy

This fork will create new `libpapilo.cpp/.h` files (separate from existing `papilolib.cpp/.h`) to avoid merge conflicts. The new API will focus exclusively on presolving functionality and will be built as a shared library.

### Target C API Interface

The goal is to support the three-stage workflow demonstrated in PaPILO tests:

1. **Construction**: `libpapilo_problem_create()` → set dimensions → add variables/constraints → add matrix entries
2. **Execution**: `libpapilo_presolve()` → query status and reductions  
3. **Validation**: Extract presolved problem → manual postsolve when needed

## Build Commands

The project uses CMake presets for consistent build configuration:

```bash
# Configure for release build (default)
cmake --preset default

# Configure for debug build
cmake --preset debug

# Build with the configured preset
cmake --build --preset default  # for release
cmake --build --preset debug    # for debug

# Run tests
ctest --preset default  # for release
ctest --preset debug    # for debug

# Run specific test
cd build/debug  # or build/release
./test/unit_test "dual-fix-happy-path"
```

Build directories:
- Release build: `build/release/`
- Debug build: `build/debug/`

The presets automatically:
- Use Ninja as the build system
- Disable GMP and QUADMATH support (as per this fork's design)
- Set appropriate optimization flags for each build type

## Key Dependencies

- **C++ Standard**: C++14
- **CMake**: >= 3.11.0
- **Intel TBB**: >= 2020 (for parallelization)
- **Boost**: >= 1.65 (headers only for this fork)

## Testing Framework

PaPILO uses **Catch2** as its testing framework. The framework is bundled in `src/papilo/external/catch/` as an amalgamated version (single header + single source file) to simplify dependency management and ensure compatibility with the project's build system. All tests use the `TEST_CASE` macro from Catch2.

For the libpapilo C API development:
- Tests will continue to use Catch2 framework
- Test files will be written in C++ (`.cpp` extension)
- Tests will only use the C API functions, not C++ classes directly
- This allows testing the C API while leveraging existing test infrastructure

## Architecture Overview

### Core Components (C++ Implementation)
- **Problem/ProblemBuilder**: Problem representation and construction
- **Presolve**: Main orchestrator with 17 individual presolving methods
- **Reductions**: Tracks all presolving modifications
- **PostsolveStorage**: Enables solution transformation back to original space

### Template Design
Original PaPILO uses `REAL` template parameter for numerical precision. This fork simplifies by using `double` exclusively, avoiding template complexity in the C API.

### Existing vs New C API
- **Existing** (`src/papilolib.h/cpp`): Integrated solver interface with presolve+solve+postsolve
- **New** (`src/libpapilo.h/cpp`): Presolve-only interface for this fork
  - Avoids merge conflicts with upstream
  - Focus exclusively on presolving workflow
  - Shared library build target

### Presolve Driver Routine

The `Presolve` class in `src/papilo/core/Presolve.hpp` acts as the main driver for the entire presolving process. Its main entry point is the `Presolve::apply()` method.

-   **Automated Presolver Combination**: The `Presolve::addDefaultPresolvers()` method registers a default set of 17 presolvers. Inside `Presolve::apply()`, these are sorted by their `PresolverTiming` (`kFast`, `kMedium`, `kExhaustive`) to create distinct groups for the execution loop.

-   **Execution Flow**: The `Presolve::apply()` method contains the main `do-while` loop that manages the presolving rounds.
    -   The loop's state is controlled by the `round_to_evaluate` enum (`Delegator`), which determines which group of presolvers to run (`kFast`, `kMedium`, or `kExhaustive`).
    -   The private method `run_presolvers()` is called to execute all presolvers within the currently selected group, potentially in parallel using TBB.
    -   After each group execution, `evaluate_and_apply()` is called. This method checks the results from all presolvers and applies the successful reductions to the problem.
    -   Based on the progress made, `determine_next_round()` decides whether to move to the next difficulty level (e.g., from `kFast` to `kMedium`), restart from `kFast` if significant changes were made, or abort if no further progress is detected or limits are reached.

-   **Customization**: While `addDefaultPresolvers()` offers a standard configuration, advanced users can manually add specific presolvers using `Presolve::addPresolveMethod()` to create a custom presolving sequence.

This design allows general users to benefit from a powerful, automated presolving engine by simply calling `apply()`, while still offering fine-grained control to expert users.

### Postsolve Mechanism and Data Access

For every presolving modification, PaPILO records a corresponding "undo" operation. The process of applying these undo operations to transform a presolved solution back into the original problem's solution space is called **Postsolve**.

-   **Core Components**:
    -   `PostsolveStorage` (`src/papilo/core/postsolve/PostsolveStorage.hpp`): This class **records** all reductions performed during presolve. It acts as a stack, storing each modification (e.g., variable fixings, row deletions) so it can be undone in the reverse order.
    -   `Postsolve` (`src/papilo/core/postsolve/Postsolve.hpp`): This class contains the logic to **execute** the postsolve process. Its `undo()` method uses the data in `PostsolveStorage` to reconstruct the original solution.

-   **Data Access in C++**:
    The `PostsolveStorage` class exposes its internal data structures as `public` members. This provides direct access to the complete history of reductions, including:
    -   `types`: A `Vec` of `ReductionType` enums indicating the kind of modification.
    -   `indices`, `values`, `start`: `Vec`s that store the detailed data for each reduction, such as variable indices and coefficient values.
    -   `origcol_mapping`, `origrow_mapping`: `Vec`s that map the indices of the reduced problem's variables and constraints back to their original indices.
    -   `getOriginalProblem()`: A method to get a copy of the problem before any presolving was applied.

-   **Data-Driven (Not Object-Oriented) Design**: It is crucial to understand that this mechanism is data-driven. Individual `PresolveMethod`s do not register their own specific "undo" logic. The process is:
    1.  **A `PresolveMethod` reports a proposed change.** For example, in `SingletonCols::execute` (in `src/papilo/presolvers/SingletonCols.hpp`), a call like `reductions.fixCol(...)` is made. This only adds a "fix column" request to a temporary `Reductions` object and does not know how to undo itself.
    2.  **A central manager applies the change and records a generic log entry.** The main `Presolve` loop calls `Presolve::applyReductions`, which in turn calls `ProblemUpdate::applyTransaction`. This manager applies the change to the problem data. Crucially, inside methods like `ProblemUpdate::fixCol` (in `src/papilo/core/ProblemUpdate.hpp`), a call is made to `postsolve.storeFixedCol(...)`. This records a generic log entry (`type: kFixedCol`, data: `{col, val, ...}`) in `PostsolveStorage`. The log is agnostic to which presolver caused the change.
    3.  **The `Postsolve::undo()` method interprets the log.** The main `undo` method in `src/papilo/core/postsolve/Postsolve.hpp` iterates through the `PostsolveStorage` logs in reverse order. A large `switch` statement on the `ReductionType` determines the correct inverse action, calling helper functions like `apply_fix_var_in_original_solution` to reconstruct the original solution values.

    This approach decouples presolvers from postsolve logic, simplifying the addition of new presolving techniques.

-   **C API Design Strategy**:
    The C API will provide functions to both execute the postsolve and, for advanced users, query the underlying postsolve data.
    1.  **Execution**: A `papilo_postsolve()` function will take a presolved solution and a `papilo_postsolve_t` handle (an opaque pointer to the `PostsolveStorage` object), and return the reconstructed original solution.
    2.  **Data Query**: A set of getter functions will be provided to inspect the `PostsolveStorage` data from C. For example:
        -   `papilo_postsolve_get_num_reductions()`: Returns the total number of reductions.
        -   `papilo_postsolve_get_reduction_info()`: Retrieves the details of a specific reduction by its index.
        -   `papilo_postsolve_get_col_mapping()`: Returns the mapping from reduced column indices to original column indices.

This approach ensures that while the primary C API workflow is simple (presolve -> solve -> postsolve), advanced users have the tools to introspect the transformation process for debugging or custom analysis.

## Detailed Implementation Notes

### Source Structure
```
src/papilo/
├── core/              # Problem, Presolve, PostsolveStorage
├── presolvers/        # 17 presolving methods
├── io/                # MPS/OPB parsers (needed for C API)
├── misc/              # Vec<T>, utilities, numerical types
└── external/          # Third-party libraries (fmt, LUSOL, etc.)
```

### Key Implementation Details
- **Vec<T>**: Custom vector type (currently identical to std::vector, designed for future allocator optimization)
- **LUSOL**: Fortran-based sparse LU solver for advanced presolving techniques
- **Explicit instantiations**: All templates pre-instantiated for double/quad/rational types
- **LIFO postsolve**: Reductions stored in reverse order for correct solution reconstruction

### Test-Driven API Design
Analysis of `test/papilo/presolve/` reveals the essential API patterns:

**Problem Construction Pattern:**
```cpp
ProblemBuilder<double> pb;
pb.reserve(nnz, nrows, ncols);
pb.setColUbAll(bounds) / setObjAll(coeffs) / addEntryAll(triplets);
Problem<double> problem = pb.build();
```

**Presolve Execution Pattern:**
```cpp
PresolveOptions options{};
ProblemUpdate<double> update(problem, postsolve, stats, options, num, msg);
Reductions<double> reductions{};
PresolveStatus status = presolver.execute(problem, update, num, reductions, timer);
```

**Result Validation Pattern:**
```cpp
// Check status: kUnchanged, kReduced, kInfeasible, kUnbounded
// Access reductions: count, individual details (col, row, value, type)
// Extract modified problem data
```

The C API should mirror these patterns while providing C-compatible data structures and error handling.

## Development Guidelines

- **Memory safety**: All C API functions must handle allocation failures gracefully
- **Error handling**: Use return codes, never throw exceptions across C boundary  
- **Opaque handles**: Hide C++ implementation details behind void* handles
- **Resource management**: Provide explicit create/destroy functions for all objects
- **Thread safety**: Document threading requirements (likely requires external synchronization)

## Implementation Plan

The development will be phased to deliver a functional C API quickly, prioritizing the automated, general-purpose use case first.

### Phase 1: Foundation and Automated Presolve API
This phase focuses on creating a fully functional, high-level C API that leverages PaPILO's automated presolving capabilities.

-   [ ] **C API Scaffolding**:
    -   [ ] Create `src/libpapilo.h` and `src/libpapilo.cpp` for the new C API.
    -   Set up a `libpapilo` shared library target in CMake.
    -   Create the `test/libpapilo/` directory and configure its CMake target.
-   [ ] **Problem Construction API**:
    -   Implement C functions to build a problem instance from scratch, wrapping the C++ `ProblemBuilder` class.
    -   Functions: `papilo_create()`, `papilo_free()`, `papilo_set_problem_data()`, etc.
    -   Create `test/libpapilo/test_problem_construction.cpp` to verify this API.
-   [ ] **Automated Presolve API**:
    -   Implement a high-level `papilo_presolve()` C function.
    -   This function will internally:
        1.  Create a `papilo::Presolve<double>` object.
        2.  Call `addDefaultPresolvers()` to load the standard presolving pipeline.
        3.  Execute the presolve by calling the `apply()` method.
    -   Implement C functions to query the results (status, statistics, presolved problem).
-   [ ] **End-to-End Test**:
    -   Create a test case in `test/libpapilo/` that mimics a test from `test/papilo/presolve/`.
    -   The test will construct a problem, call `papilo_presolve()`, and verify that the outcome matches the original C++ test, ensuring the high-level API works as expected.

### Phase 2: Advanced Control and Customization API
Once the core automated functionality is available and tested, this phase will introduce APIs for expert users who require fine-grained control.

-   [ ] **Presolve Customization API**:
    -   Design and implement C functions that allow users to configure the presolving process.
    -   Implement `papilo_add_presolver(papilo_t* p, const char* name)` to allow adding individual presolvers by name. This will replace the call to `addDefaultPresolvers()`.
    -   Expose key options from `PresolveOptions` through the C API (e.g., `papilo_set_int_param()`, `papilo_set_real_param()`).
-   [ ] **Test Migration**:
    -   Begin migrating tests from `test/papilo/presolve/` one by one.
    -   Each migrated test will use the new C API, first by using the customization API to add only the specific presolver being tested.
    -   This ensures that each presolver is correctly wrapped and behaves as expected when called individually through the C API.

This revised plan ensures that a useful, automated presolving library is available after Phase 1, while paving a clear path for more advanced, customizable features in Phase 2.
