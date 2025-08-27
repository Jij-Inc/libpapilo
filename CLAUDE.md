# Claude Code Development Guide for libpapilo

This file provides guidance to Claude Code (claude.ai/code) for developing the libpapilo C API.

## Project Overview

libpapilo is a fork of `scipopt/papilo` that provides PaPILO (Parallel Presolve for Integer and Linear Optimization) as a **presolve-only shared library with a C API**. It makes PaPILO's presolving functionality accessible from other programming languages (C, Python, Julia) through a stable C interface.

**Key Features:**
- Presolve-only focus - no solver integrations
- Double precision C API
- Standalone implementation separate from upstream `papilolib.h/.cpp`

## Build and Test

### Using Task Runner (Recommended)
```bash
task build:all        # Build everything (debug)
task test:all         # Run all tests
task test:libpapilo   # Run C API tests only
task format:libpapilo # Format C API source files
```

### Using CMake Directly
```bash
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
```

## PaPILO Architecture

### Core C++ Components
- **`Problem`**: Represents the optimization problem (variables, constraints, objective)
- **`ProblemBuilder`**: Constructs Problem instances with validation
- **`Presolve`**: Orchestrates 17+ individual presolving methods
- **`Reductions`**: Tracks modifications to be applied to the problem
- **`PostsolveStorage`**: Records all transformations for solution reconstruction

### Presolving Methods
PaPILO includes various presolving techniques:
- Singleton column/row elimination
- Simple substitution
- Coefficient strengthening
- Constraint propagation
- Dominated columns detection
- Dual fixing
- Probing
- Parallel row/column detection
- And more...

For detailed mechanics, see [PRESOLVE.md](./PRESOLVE.md) and [POSTSOLVE.md](./POSTSOLVE.md).

## libpapilo C API Design

### Design Principles
- **Opaque Pointers**: C++ objects exposed as opaque struct pointers (e.g., `libpapilo_problem_t*`)
- **Error Handling**: Pointer validation with magic numbers, exception catching with `check_run()`
- **Resource Management**: Explicit `_create()` and `_free()` functions for all objects

### API Structure

The C API provides comprehensive access to PaPILO functionality:

1. **Problem Construction**: Build optimization problems via `ProblemBuilder`
2. **Problem Inspection**: Query problem data (constraints, variables, objective)
3. **Presolve Operations**: Apply individual or combined presolving methods
4. **Reduction Management**: Fine-grained control over problem transformations
5. **Postsolve Support**: Access transformation history for solution reconstruction

### Testing Strategy
Tests are written in C++ but only call the public C API functions from `libpapilo.h`. This validates the C interface while leveraging Catch2 testing framework.

## Development Workflow

1. **Understanding Requirements**: Check existing C++ tests in `test/papilo/` for functionality patterns
2. **API Implementation**: Add new functions to `libpapilo.h` and `libpapilo.cpp`
3. **Testing**: Create corresponding tests in `test/libpapilo/` that mirror C++ test behavior
4. **Validation**: Ensure mathematical correctness by comparing with original C++ implementation

## Current Status

The C API provides comprehensive coverage of PaPILO's core functionality:
- Complete problem construction and data access
- Advanced reduction operations with transaction safety
- Individual presolver wrappers (SingletonCols, SimpleSubstitution, etc.)
- High-level presolve orchestration
- Ongoing work on postsolve API and additional presolver wrappers