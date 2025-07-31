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

The `Presolve` class in `src/papilo/core/Presolve.hpp` acts as the main driver for the entire presolving process. It orchestrates the execution of individual presolving methods.

-   **Automated Presolver Combination**: The `Presolve::addDefaultPresolvers()` method registers a default set of 17 presolvers, categorized into `fast`, `medium`, and `exhaustive` timings. This provides a well-tested, general-purpose presolving pipeline out-of-the-box.
-   **Execution Flow**: The `Presolve::apply()` method manages the main presolve loop. It iteratively runs rounds of presolvers, starting with `fast` methods and progressing to more `exhaustive` ones based on the reduction achieved in each round. This continues until the problem is no longer being reduced or a limit is reached.
-   **Customization**: While `addDefaultPresolvers()` offers a standard configuration, advanced users can manually add specific presolvers using `Presolve::addPresolveMethod()` to create a custom presolving sequence.

This design allows general users to benefit from a powerful, automated presolving engine by simply calling `apply()`, while still offering fine-grained control to expert users.

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
