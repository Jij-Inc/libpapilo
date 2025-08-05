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

### Phase 2: Presolving API Implementation 🚧 **IN PROGRESS**

This phase will implement the core presolving functionality. The primary goal is to expose PaPILO's automated presolving pipeline.

- **C API Design**: The design will follow a **one-to-one mapping** of core C++ classes to C opaque pointers to ensure a clear and maintainable wrapper. The following handles will be introduced:
    - `libpapilo_presolve_options_t` -> `papilo::PresolveOptions`
    - `libpapilo_statistics_t` -> `papilo::Statistics`
    - `libpapilo_postsolve_storage_t` -> `papilo::PostsolveStorage<double>`
    - `libpapilo_problem_update_t` -> `papilo::ProblemUpdate<double>`
    - `libpapilo_reductions_t` -> `papilo::Reductions<double>`
- **Tasks**:
    - **Automated Presolve API**:
        - Implement a high-level `papilo_presolve_apply()` C function that internally creates a `papilo::Presolve` object, loads the default presolvers, and runs the `apply()` method.
        - This function will return the results in new opaque objects like `libpapilo_reductions_t` and `libpapilo_postsolve_storage_t`.
    - **Result Management API**:
        - Implement C functions to query the results (e.g., `papilo_reductions_get_size`, `papilo_reductions_get_info`).
        - Expose statistics (number of reductions, fixed variables, etc.).
    - **End-to-End Testing**:
        - Create tests that construct problems, run `papilo_presolve_apply`, and validate the results.

### Phase 3: Advanced Control and Customization API 🚧 **FUTURE WORK**

This phase will provide fine-grained control for expert users.

- **Tasks**:
    - **Presolve Customization API**:
        - Expose key options from `PresolveOptions` via the C API.
        - Implement functions to create and run custom presolving pipelines (e.g., `papilo_add_presolver_by_name`).
    - **Individual Presolver Testing**:
        - Migrate tests from `test/papilo/presolve/` to use the new C API, enabling targeted testing of each presolver through the C interface.

## 5. Current C API Reference

The libpapilo C API currently provides 38 functions for problem construction and data access.

- **Problem Builder API (19 functions)**: `create()`, `free()`, `build()`, `reserve()`, `set_num_rows/cols()`, `set_obj()`, `set_col_lb/ub()`, etc.
- **Problem Data API (19 functions)**: `get_nrows/ncols/nnz()`, `get_objective_coefficients()`, `get_lower/upper_bounds()`, `get_row/col_entries()`, `get_col/row_flags()`, etc.