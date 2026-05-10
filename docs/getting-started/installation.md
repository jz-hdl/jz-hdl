---
title: Installation
lang: en-US

layout: doc
outline: deep
---

# Installation

## Prerequisites

- A C99-compatible compiler (GCC, Clang, or MSVC)
- A C++17-compatible compiler
- CMake 3.16 or later

## Building from source

```bash
# Clone the repository
git clone https://github.com/zaun/jz-hdl.git
cd jz-hdl

# Configure the build
cmake -S . -B build

# Build compiler + viewer
cmake --build build
```

On Unix-like single-config generators, the compiler binary is typically produced at `build/compiler/jz-hdl`, and the viewer at `build/viewer/jz-viewer`.

On Windows multi-config generators such as Visual Studio, use an explicit configuration:

```bash
cmake --build build --config Release
```

In that case the binaries are typically produced at `build/compiler/Release/jz-hdl.exe` and `build/viewer/Release/jz-viewer.exe`.

## Installing

```bash
cmake --install build --prefix /usr/local
```

The install tree places both `jz-hdl` and `jz-viewer` in the configured install `bin/` directory.

## Building with tests

```bash
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build

# For Visual Studio or other multi-config generators
cmake --build build --config Release

# Run the CTest suite
ctest --test-dir build --output-on-failure

# Multi-config generators may also require:
ctest --test-dir build -C Release --output-on-failure
```

Current CTest targets are:

- `const_eval`
- `type_semantics`
- `literal_semantics`
- `lint_validation`

## Verifying the installation

```bash
# Check that the compiler runs
build/compiler/jz-hdl --help

# Run the focused CTest validation target
ctest --test-dir build --output-on-failure -R lint_validation

# Run the full validation script directly
bash compiler/tests/run_validation.sh
```

If you built with a multi-config generator, replace `build/compiler/jz-hdl` with the configuration-specific binary path such as `build/compiler/Release/jz-hdl.exe`.

## Regenerating the specification PDFs

The website ships PDF copies of the specifications, but the authoritative sources live in `specification/*.md`.

The existing regeneration path is defined in `specification/CMakeLists.txt` and exposed through the root `docs` target. It requires `pandoc` and a XeLaTeX installation with the needed LaTeX packages.

```bash
cmake -S . -B build
cmake --build build --target docs
```

That target rebuilds:

- `jz-hdl-specification.pdf`
- `simulation-specification.pdf`
- `testbench-specification.pdf`
- `chip-info-specification.pdf`
- `jzw-specification.pdf`

The generated PDFs are written to `build/specification/`. `scripts/gitpages-update` rebuilds them and stages them into the published site output under `/pdf/` during docs deployment.
