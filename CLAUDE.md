# Claude Code Development Guide for libpapilo

This file provides guidance to Claude Code (claude.ai/code) for developing the libpapilo C API.

## 1. Project Overview

libpapilo is a fork of `scipopt/papilo` that aims to provide PaPILO (Parallel Presolve for Integer and Linear Optimization) as a **presolve-only shared library with a C API**. The goal is to make PaPILO's powerful presolving functionality accessible from other programming languages (e.g., C, Python, Julia) through a stable C interface, separating it from any specific solver.

**Key Design Principles:**
- **Presolve-Only Focus**: No solver integrations. Users perform presolve with libpapilo, then solve the reduced problem with their own solver.
- **Double Precision**: The C API exclusively uses `double` precision to avoid the complexity of C++ templates.
- **Standalone C API**: A new `libpapilo.h/.cpp` provides the C API, keeping it separate from the original `papilolib.h/.cpp` to avoid upstream merge conflicts.

## 2. Build and Test Instructions

The project uses CMake and includes presets for easy configuration. The Task runner provides convenient shortcuts for common workflows.

### Using Task Runner (Recommended)

```bash
# Build everything (debug configuration)
task build:all

# Run all tests
task test:all

# Run only the libpapilo C API tests
task test:libpapilo

# Format only the libpapilo C API source files
task format:libpapilo
```

### Using CMake Presets Directly

```bash
# Configure for debug or release build
cmake --preset debug
# cmake --preset default # for release

# Build with the configured preset
cmake --build --preset debug

# Run all tests
ctest --preset debug

# Run the libpapilo C API tests specifically
cd build/debug
./test/libpapilo/libpapilo_unit_test
```

### Testing Strategy
The project uses **Catch2** as its testing framework, which is bundled in the `src/papilo/external/` directory.

To ensure the C API is robust, tests are written in C++ (`.cpp` files) but **only call the public C API functions** defined in `libpapilo.h`. This validates the C interface from a C++ environment while leveraging the existing test infrastructure.

## 3. Architecture and Implementation

### Core C++ Components
- **`Problem` / `ProblemBuilder`**: Classes for problem representation and construction.
- **`Presolve`**: The main driver that orchestrates the pipeline of 17+ individual presolving methods.
- **`Reductions` / `PostsolveStorage`**: Data structures that track all modifications for later reconstruction of the solution (postsolve).

For a detailed explanation of the presolve and postsolve mechanics, refer to [PRESOLVE.md](./PRESOLVE.md) and [POSTSOLVE.md](./POSTSOLVE.md).

### C API Design Philosophy
The C API is designed to be a safe and maintainable wrapper around the C++ core.
- **Opaque Pointers**: All C++ objects (`Problem`, `ProblemBuilder`, etc.) are exposed to C as opaque `struct` pointers (e.g., `libpapilo_problem_t*`). This hides implementation details.
- **Error Handling**: A strict error handling strategy is enforced.
  - `check_..._ptr()` functions validate all incoming pointers using a magic number to prevent type confusion or use-after-free errors.
  - `check_run()` wraps C++ function calls to catch any exceptions, print a descriptive error, and safely terminate.
- **Resource Management**: All opaque objects have corresponding `_create()` and `_free()` functions for explicit resource management.

## 4. Implementation Plan & Roadmap

The development follows a phased approach to deliver a functional C API.

### Phase 1: Problem Construction & Data Access API ✅ **COMPLETED**

This phase established the C API foundation, providing the ability to build a problem in memory and inspect its data.

- **Key Deliverables**:
    - C API scaffolding (`libpapilo.h`, `libpapilo.cpp`, CMake shared library target).
    - Robust error handling and resource management infrastructure.
    - **38 C API functions** covering problem construction (`libpapilo_problem_builder_...`) and data retrieval (`libpapilo_problem_get_...`).
- **Status**: Complete and comprehensively tested.

### Phase 2: Individual Presolver API Implementation 🚧 **IN PROGRESS**

**Phase 2 Goal**: Complete reproduction of all tests in `test/papilo/presolve/` directory using libpapilo C API.

**First Milestone Completed**: ✅ Full reproduction of `test/papilo/presolve/SingletonColsTest.cpp` using libpapilo C API ([PR #8](https://github.com/Jij-Inc/libpapilo/pull/8))

This phase implements the granular presolving functionality required to exactly replicate all existing C++ presolve tests, providing complete validation that the C API exposes all necessary PaPILO functionality.

#### Completed Components

- **Core C++ to C API Mappings** ✅:
    - `libpapilo_presolve_options_t` -> `papilo::PresolveOptions` ✅
    - `libpapilo_statistics_t` -> `papilo::Statistics` ✅
    - `libpapilo_postsolve_storage_t` -> `papilo::PostsolveStorage<double>` ✅
    - `libpapilo_problem_update_t` -> `papilo::ProblemUpdate<double>` ✅
    - `libpapilo_reductions_t` -> `papilo::Reductions<double>` ✅
    - `libpapilo_singleton_cols_t` -> `papilo::SingletonCols<double>` ✅
    - `libpapilo_simple_substitution_t` -> `papilo::SimpleSubstitution<double>` ✅
    - `libpapilo_num_t` -> `papilo::Num<double>` ✅
    - `libpapilo_timer_t` -> `papilo::Timer` ✅
    - `libpapilo_message_t` -> `papilo::Message` ✅

- **Implemented APIs** ✅:
    - **Problem Modification API**:
        - `libpapilo_problem_modify_row_lhs()` - Modify constraint left-hand side
        - `libpapilo_problem_recompute_locks()` - Recompute variable locks
        - `libpapilo_problem_recompute_activities()` - Recompute row activities
    - **ProblemUpdate Control API**:
        - `libpapilo_problem_update_create()` - Create ProblemUpdate with all dependencies
        - `libpapilo_problem_update_trivial_column_presolve()` - Execute trivial column presolve
        - `libpapilo_problem_update_get_reductions()` - Extract reductions from ProblemUpdate
    - **Individual Presolver API**:
        - `libpapilo_singleton_cols_create/free()` - SingletonCols presolver management
        - `libpapilo_singleton_cols_execute()` - Execute individual presolver with full control
        - `libpapilo_simple_substitution_create/free()` - SimpleSubstitution presolver management
        - `libpapilo_simple_substitution_execute()` - Execute SimpleSubstitution presolver
    - **Utility Objects API**:
        - `libpapilo_num_create/free()` - Numerical utilities
        - `libpapilo_timer_create/free()` - Timer functionality
        - `libpapilo_message_create/free()` - Message handling
    - **Reductions API**:
        - Full implementation extracting from PostsolveStorage/ProblemUpdate
        - Complete reduction type enums (14 column types, 16 row types)

- **Test Migrations Completed** ✅:
    - **SingletonColsTest.cpp** (7 test cases): All test cases successfully migrated to C API
    - **SimpleSubstitutionTest.cpp** (12 test cases): All test cases successfully migrated to C API
    - All `setupProblem*()` functions converted to C API calls
    - Every `REQUIRE()` assertion passes with identical values
    - Mathematical content verified for consistency between C++ and C API versions

#### Remaining Phase 2 Work

- **Test Migrations Required** (14 remaining tests):
    - `CoefficientStrengtheningTest.cpp`
    - `ConstraintPropagationTest.cpp`
    - `DominatedColsTest.cpp`
    - `DualFixTest.cpp`
    - `FixContinuousTest.cpp`
    - `FreeVarSubstitutionTest.cpp`
    - `ImplIntDetectionTest.cpp`
    - `ParallelColDetectionTest.cpp`
    - `ParallelRowDetectionTest.cpp`
    - `ProbingTest.cpp`
    - `SimpleProbingTest.cpp`
    - `SimplifyInequalitiesTest.cpp`
    - `SingletonStuffingTest.cpp`
    - `SparsifyTest.cpp`

- **Required Presolver Wrappers** (to be determined based on test requirements):
    - Each test file will require its corresponding presolver to be wrapped
    - Pattern established with SingletonCols and SimpleSubstitution can be replicated

- **High-Level Presolve Orchestrator** 🚧:
    - `libpapilo_presolve_t` wrapper implementation
    - Default presolver pipeline configuration
    - Batch presolve execution API

- **Postsolve API** 🚧:
    - Solution reconstruction functions
    - Dual value recovery
    - Basis information handling

### Phase 3: High-Level Automated API 🚧 **FUTURE WORK**

This phase will provide simplified automated presolving for end users.

- **Tasks**:
    - **Automated Presolve API**: High-level `papilo_presolve_apply()` using all default presolvers
    - **Batch Processing API**: Convenient functions for common use cases
    - **Documentation and Examples**: User-friendly guides for typical workflows

## 5. Current C API Reference

The libpapilo C API currently provides 38 functions for problem construction and data access.

- **Problem Builder API (19 functions)**: `create()`, `free()`, `build()`, `reserve()`, `set_num_rows/cols()`, `set_obj()`, `set_col_lb/ub()`, etc.
- **Problem Data API (19 functions)**: `get_nrows/ncols/nnz()`, `get_objective_coefficients()`, `get_lower/upper_bounds()`, `get_row/col_entries()`, `get_col/row_flags()`, etc.