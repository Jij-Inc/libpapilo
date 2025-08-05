# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

libpapilo is a fork of scipopt/papilo that aims to provide PaPILO (Parallel Presolve for Integer and Linear Optimization) as a **shared library with a C API**. While the original PaPILO is implemented in C++, this fork's goal is to make the presolving functionality accessible from other programming languages through a stable C interface.

**Key Design Decisions:**
- **Presolving only**: No solver integrations (SCIP, Gurobi, etc.) - users solve with their own choice of solver
- **Double precision only**: Simplifies C API by avoiding C++ template complexity (original supports double/quadruple/rational)
- **Phase 1 Complete**: Problem construction and data access C API fully implemented with comprehensive testing

The original PaPILO is a C++14-based presolve package for (mixed integer) linear programming problems with support for parallel execution and multiple precision arithmetic, licensed under LGPLv3.

## Development Strategy

This fork will create new `libpapilo.cpp/.h` files (separate from existing `papilolib.cpp/.h`) to avoid merge conflicts. The new API will focus exclusively on presolving functionality and will be built as a shared library.

### Target C API Interface

The goal is to support the three-stage workflow demonstrated in PaPILO tests:

1. **Construction**: `libpapilo_problem_builder_create()` → set dimensions → add variables/constraints → add matrix entries → `libpapilo_problem_builder_build()` ✅ **IMPLEMENTED**
2. **Execution**: `libpapilo_presolve()` → query status and reductions 🚧 **PHASE 2**
3. **Validation**: Extract presolved problem → manual postsolve when needed 🚧 **PHASE 3**

## Build Commands

The project supports both CMake presets and Task runner for build automation.

### Using Task Runner (Recommended)

The project includes a `Taskfile.yml` for common development tasks:

```bash
# Build everything (debug configuration)
task build:all

# Run all tests
task test:all

# Run only libpapilo C API tests
task test:libpapilo

# Format libpapilo code (C API files only)
task format:libpapilo
```

### Using CMake Presets Directly

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

# Run libpapilo C API tests specifically
./test/libpapilo/libpapilo_unit_test
```

Build directories:
- Release build: `build/release/`
- Debug build: `build/debug/`

The presets automatically:
- Use Ninja as the build system
- Disable GMP and QUADMATH support (as per this fork's design)
- Set appropriate optimization flags for each build type

**Note**: Both Task runner and direct CMake commands are supported. The Task runner provides convenient shortcuts for common development workflows and is used by the CI/CD pipeline.

## Key Dependencies

- **C++ Standard**: C++14
- **CMake**: >= 3.11.0  
- **Intel TBB**: >= 2020 (for parallelization)
- **Boost**: >= 1.65 (headers only for this fork)
- **Task**: >= 3.x (development task runner)
- **clang-format**: (for code formatting)

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

### Presolve and Postsolve Internals

The core of PaPILO is its presolving engine. The process is orchestrated by the `Presolve` class, which manages a pipeline of individual presolving methods (presolvers). These presolvers work cooperatively to simplify the problem. For a detailed explanation of the presolve driver, the interaction between different presolvers, and the postsolve mechanism, please refer to [PRESOLVE.md](./PRESOLVE.md) and [POSTSOLVE.md](./POSTSOLVE.md).


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

- **Memory safety**: All C API functions must handle allocation failures gracefully ✅ **IMPLEMENTED**
- **Error handling**: Use `custom_assert()` and `check_run()` for consistent error handling and production safety ✅ **IMPLEMENTED**
- **Opaque handles**: Hide C++ implementation details behind typed structs with magic numbers ✅ **IMPLEMENTED**
- **Resource management**: Provide explicit create/destroy functions for all objects ✅ **IMPLEMENTED**
- **Thread safety**: Document threading requirements (likely requires external synchronization)

### Error Handling Strategy

The implemented C API uses a strict error handling approach:
- **`custom_assert()`**: Production-safe assertions that print error messages and call `std::terminate()`
- **`check_run()`**: Template function for exception-safe operations with automatic error reporting
- **Magic number validation**: All opaque pointers are validated to prevent use-after-free and type confusion
- **Graceful degradation**: Functions return error codes or NULL where appropriate

## Implementation Plan

The development will be phased to deliver a functional C API quickly, prioritizing the automated, general-purpose use case first.

### Phase 1: Foundation and Problem Construction API ✅ **COMPLETED**
This phase focuses on creating the C API infrastructure and problem construction capabilities.

-   [x] **C API Scaffolding**:
    -   [x] Create `src/libpapilo.h` and `src/libpapilo.cpp` for the new C API
    -   [x] Set up a `libpapilo` shared library target in CMake
    -   [x] Create the `test/libpapilo/` directory and configure its CMake target
    -   [x] Implement robust error handling with `custom_assert()` and `check_run()`
-   [x] **Problem Construction API (19 functions)**:
    -   [x] Core builder functions: `libpapilo_problem_builder_create/free/build()`
    -   [x] Dimension management: `set_num_rows/cols()`, `reserve()`
    -   [x] Objective: `set_obj()`, `set_obj_all()`, `set_obj_offset()`
    -   [x] Variable bounds: `set_col_lb/ub()`, `set_col_lb/ub_all()`
    -   [x] Variable properties: `set_col_integral()`, `set_col_integral_all()`
    -   [x] Constraint bounds: `set_row_lhs/rhs()`, `set_row_lhs/rhs_all()`
    -   [x] Matrix entries: `add_entry()`, `add_entry_all()`, `add_row/col_entries()`
    -   [x] Names: `set_problem_name()`, `set_col/row_name()`
-   [x] **Data Retrieval API (19 functions)**:
    -   [x] Problem info: `get_nrows/ncols/nnz()`, `get_num_integral/continuous_cols()`
    -   [x] Objective: `get_objective_coefficients/offset()`
    -   [x] Bounds: `get_lower/upper_bounds()`, `get_row_lhs/rhs()`
    -   [x] Matrix structure: `get_row/col_sizes()`, `get_row/col_entries()`
    -   [x] Names: `get_name()`, `get_variable/constraint_name()`
    -   [x] Flags: `get_col/row_flags()` with C-compatible enum conversion
-   [x] **Comprehensive Testing**:
    -   [x] 153 lines of test code in `test/libpapilo/ProblemBuilderTest.cpp`
    -   [x] 86 assertions covering all API functions
    -   [x] Four test sections covering different construction methods
    -   [x] Full validation of problem construction and data retrieval

**Status**: Phase 1 is complete with 38 C API functions (19 setters + 19 getters) providing full problem construction and data access capabilities.

### Phase 2: Presolving API Implementation 🚧 **FUTURE WORK**
This phase will implement the core presolving functionality and APIs for automated presolving workflows.

-   [ ] **Automated Presolve API**:
    -   [ ] Implement a high-level `papilo_presolve()` C function.
    -   [ ] This function will internally:
        1.  Create a `papilo::Presolve<double>` object.
        2.  Call `addDefaultPresolvers()` to load the standard presolving pipeline.
        3.  Execute the presolve by calling the `apply()` method.
    -   [ ] Implement C functions to query the results (`papilo_result_get_status()`, `papilo_result_get_nrows()`, etc.).
-   [ ] **Presolve Result Management**:
    -   [ ] Design and implement result structures for presolve outcomes.
    -   [ ] Extract detailed presolve timing information.
    -   [ ] Add APIs for accessing presolve statistics (reductions, fixed variables, etc.).
    -   [ ] Expose fixed column counts and other detailed metrics from postsolve storage.
-   [ ] **End-to-End Testing**:
    -   [ ] Create comprehensive tests that verify the complete presolve workflow.
    -   [ ] Test cases should construct problems, run presolve, and validate results.
    -   [ ] Ensure presolve outcomes match expectations from C++ implementation.

### Phase 3: Advanced Control and Customization API 🚧 **FUTURE WORK**
Once automated presolving is available, this phase will introduce APIs for expert users who require fine-grained control.

-   [ ] **Presolve Customization API**:
    -   [ ] Design and implement C functions that allow users to configure the presolving process.
    -   [ ] Implement `papilo_add_presolver(papilo_t* p, const char* name)` to allow adding individual presolvers by name.
    -   [ ] Expose key options from `PresolveOptions` through the C API (e.g., `papilo_set_int_param()`, `papilo_set_real_param()`).
-   [ ] **Individual Presolver Testing**:
    -   [ ] Begin migrating tests from `test/papilo/presolve/` one by one.
    -   [ ] Each migrated test will use the new C API with customization to test specific presolvers.
    -   [ ] This ensures each presolver is correctly wrapped and behaves as expected through the C API.

**Current Status**: Phase 1 is complete and provides a solid foundation for problem construction and data access. Phase 2 will implement the core presolving functionality to make this library useful for optimization workflows.

## Current C API Summary

The libpapilo C API currently provides 38 functions across two main categories:

### Problem Builder API (19 functions)
- **Lifecycle**: `create()`, `free()`, `build()`
- **Setup**: `reserve()`, `set_num_rows/cols()`, `get_num_rows/cols()`
- **Objective**: `set_obj()`, `set_obj_all()`, `set_obj_offset()`
- **Bounds**: `set_col_lb/ub()`, `set_col_lb/ub_all()`, `set_row_lhs/rhs()`, `set_row_lhs/rhs_all()`
- **Properties**: `set_col_integral()`, `set_col_integral_all()`
- **Matrix**: `add_entry()`, `add_entry_all()`, `add_row/col_entries()`
- **Names**: `set_problem_name()`, `set_col/row_name()`

### Problem Data API (19 functions)  
- **Basic Info**: `get_nrows/ncols/nnz()`, `get_num_integral/continuous_cols()`
- **Objective**: `get_objective_coefficients()`, `get_objective_offset()`
- **Bounds**: `get_lower/upper_bounds()`, `get_row_lhs/rhs()`
- **Structure**: `get_row/col_sizes()`, `get_row/col_entries()`
- **Names**: `get_name()`, `get_variable/constraint_name()`
- **Properties**: `get_col/row_flags()`

All functions include robust error handling, type safety through magic number validation, and comprehensive test coverage.
