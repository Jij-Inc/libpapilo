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

The goal is to rewrite all tests in `test/papilo/presolve/` using the new libpapilo C API.

### Phase 1: Foundation and Problem Construction API
- [ ] Extract problem construction code from `src/papilolib.cpp` to create initial `src/libpapilo.cpp`
  - [ ] Identify reusable functions like `problem_create()` and related utilities
  - [ ] Create `src/libpapilo.h` with C-compatible function declarations
  - [ ] Create `src/libpapilo.cpp` with implementations wrapping C++ classes
- [ ] Implement core problem construction API
  - [ ] `libpapilo_problem_create()` - create problem instance
  - [ ] `libpapilo_problem_set_dimensions()` - set rows, cols, non-zeros
  - [ ] `libpapilo_problem_set_obj()` - set objective coefficients
  - [ ] `libpapilo_problem_set_col_bounds()` - set variable bounds
  - [ ] `libpapilo_problem_set_row_bounds()` - set constraint bounds
  - [ ] `libpapilo_problem_add_entry()` - add matrix entries
  - [ ] `libpapilo_problem_destroy()` - cleanup
- [ ] Set up CMake target for building libpapilo as a shared library
  - [ ] Add `add_library(libpapilo SHARED ...)` to CMakeLists.txt
  - [ ] Configure proper export symbols for C API
- [ ] Create test infrastructure
  - [ ] Create `test/libpapilo/` directory structure
  - [ ] Set up CMake configuration for C API tests
  - [ ] Create `test_problem_construction.cpp` using Catch2 to verify problem building works correctly

### Phase 2: Presolve API and Test Migration
- [ ] Design and implement core presolve API framework
  - Study existing C++ presolve API patterns in detail
  - Design C-compatible API based on test requirements
  - Implement minimal framework needed for first presolver

- [ ] Implement each presolver as a separate PR
  - Each PR will add C API wrapper for the presolver and port the corresponding test from `test/papilo/presolve/`
  - Tests will be written in C++ using Catch2 framework, but will only use the C API (not the C++ classes directly)
  - PR is considered complete when the ported test passes with the same results as the original C++ test
  - [ ] `SingletonRow`
  - [ ] `SingletonCols`
  - [ ] `DualFix`
  - [ ] `ImpliedBounds`
  - [ ] `CoefficientStrengthening`
  - [ ] `ConstraintPropagation`
  - [ ] `DominatedCols`
  - [ ] `DualInfer`
  - [ ] `FixContinuous`
  - [ ] `FreeVarSubstitution`
  - [ ] `ImpliedIntegers`
  - [ ] `ParallelCols`
  - [ ] `ParallelRows`
  - [ ] `Probing`
  - [ ] `SimpleProbing`
  - [ ] `SimpleSubstitution`
  - [ ] `SimplifyInequalities`
  - [ ] `Sparsify`

The new API will provide a clean, presolve-focused interface while preserving the existing codebase for future upstream compatibility.
