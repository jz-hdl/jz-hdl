---
title: Installation
lang: en-US

layout: doc
outline: deep
---

# Installation

## Prerequisites

- A C99-compatible compiler (GCC, Clang, or MSVC)
- CMake 3.16 or later

## Building from source

```bash
# Clone the repository
git clone <repository-url>
cd JZ-HDL

# Configure the build
cmake -S compiler -B compiler/build

# Build
cmake --build compiler/build
```

The compiler binary is produced at `compiler/build/jz-hdl`.

## Building with tests

```bash
cmake -S compiler -B compiler/build -DBUILD_TESTING=ON
cmake --build compiler/build

# Run the CTest suite
ctest --test-dir compiler/build --output-on-failure
```

Current CTest targets are:

- `const_eval`
- `type_semantics`
- `literal_semantics`
- `lint_validation`

## Verifying the installation

```bash
# Check that the compiler runs
compiler/build/jz-hdl --help

# Run the focused CTest validation target
ctest --test-dir compiler/build --output-on-failure -R lint_validation

# Run the full validation script directly
bash compiler/tests/run_validation.sh
```
