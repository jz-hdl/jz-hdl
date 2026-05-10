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
git clone https://github.com/zaun/jz-hdl.git
cd jz-hdl

# Configure the build
cmake -S compiler -B compiler/build

# Build
cmake --build compiler/build
```

On Unix-like single-config generators, the compiler binary is typically produced at `compiler/build/jz-hdl`.

On Windows multi-config generators such as Visual Studio, use an explicit configuration:

```bash
cmake --build compiler/build --config Release
```

In that case the binary is typically produced at `compiler/build/Release/jz-hdl.exe`.

## Building with tests

```bash
cmake -S compiler -B compiler/build -DBUILD_TESTING=ON
cmake --build compiler/build

# For Visual Studio or other multi-config generators
cmake --build compiler/build --config Release

# Run the CTest suite
ctest --test-dir compiler/build --output-on-failure

# Multi-config generators may also require:
ctest --test-dir compiler/build -C Release --output-on-failure
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

If you built with a multi-config generator, replace `compiler/build/jz-hdl` with the configuration-specific binary path such as `compiler/build/Release/jz-hdl.exe`.

## Regenerating the specification PDFs

The website ships PDF copies of the specifications, but the authoritative sources live in `specification/*.md`.

The existing regeneration path is defined in `compiler/CMakeLists.txt` as the `docs` target. It requires `pandoc` and a XeLaTeX installation with the needed LaTeX packages.

```bash
cmake -S compiler -B compiler/build
cmake --build compiler/build --target docs
```

That target rebuilds:

- `jz-hdl-specification.pdf`
- `simulation-specification.pdf`
- `testbench-specification.pdf`
- `chip-info-specification.pdf`
- `jzw-specification.pdf`

The generated PDFs are written to the compiler build directory. `scripts/gitpages-update` rebuilds them and stages them into the published site output under `/pdf/` during docs deployment.
