# KadeDB Build System

This document provides detailed information about building KadeDB from source, including dependencies, build options, and development workflows.

## Table of Contents

- [Prerequisites](#prerequisites)
- [Getting Started](#getting-started)
- [Build Options](#build-options)
- [Build Types](#build-types)
- [Building with Sanitizers](#building-with-sanitizers)
- [Code Coverage](#code-coverage)
- [Installing](#installing)
- [Packaging](#packaging)
- [Development Workflow](#development-workflow)
- [Troubleshooting](#troubleshooting)

## Prerequisites

### Linux/macOS

```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    git \
    pkg-config \
    libssl-dev \
    zlib1g-dev \
    libgtest-dev \
    libgmock-dev \
    lcov \
    clang-format

# macOS (using Homebrew)
brew update
brew install cmake ninja pkg-config openssl llvm
```

### Windows

1. Install [Visual Studio 2022](https://visualstudio.microsoft.com/downloads/) with:
   - Desktop development with C++
   - Windows 10/11 SDK
   - C++ CMake tools for Windows

2. Install [Git for Windows](https://git-scm.com/download/win)
3. Install [CMake](https://cmake.org/download/)
4. Install [Python 3.10+](https://www.python.org/downloads/)

## Getting Started

### Clone the Repository

```bash
git clone https://github.com/yourusername/KadeDB.git
cd KadeDB
git submodule update --init --recursive
```

### Configure the Build

```bash
# Create build directory
mkdir -p build && cd build

# Configure with CMake (default options)
cmake ..

# Or with specific options
cmake .. \
    -DCMAKE_BUILD_TYPE=Debug \
    -DBUILD_TESTS=ON \
    -DENABLE_ASAN=ON \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
```

### Build the Project

```bash
# Build all targets
cmake --build . -- -j$(nproc)

# Or on Windows
# cmake --build . --config Debug -- /m

# Build a specific target
cmake --build . --target kadedb
```

### Run Tests

```bash
# Run all tests
ctest --output-on-failure

# Run specific test
./bin/kadedb_test
```

## Build Options

| Option | Description | Default |
|--------|-------------|---------|
| `BUILD_KADEDB_SERVER` | Build the KadeDB server | `ON` |
| `BUILD_KADEDB_LITE` | Build KadeDB-Lite | `ON` |
| `BUILD_TESTS` | Build tests | `ON` |
| `ENABLE_COVERAGE` | Enable code coverage | `OFF` |
| `ENABLE_ASAN` | Enable Address Sanitizer | `OFF` |
| `ENABLE_UBSAN` | Enable Undefined Behavior Sanitizer | `OFF` |
| `ENABLE_LTO` | Enable Link Time Optimization | `OFF` |
| `USE_SYSTEM_ROCKSDB` | Use system-installed RocksDB | `OFF` |
| `USE_SYSTEM_ANTLR` | Use system-installed ANTLR | `OFF` |

## Build Types

- **Debug**: Debug build with debug symbols and no optimizations
- **Release**: Optimized build with debug symbols
- **RelWithDebInfo**: Optimized build with debug symbols
- **MinSizeRel**: Optimized for size

## Building with Sanitizers

### Address Sanitizer (ASan)

```bash
cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_ASAN=ON ..
cmake --build .
```

### Undefined Behavior Sanitizer (UBSan)

```bash
cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_UBSAN=ON ..
cmake --build .
```

### Combined Sanitizers

```bash
cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_ASAN=ON -DENABLE_UBSAN=ON ..
cmake --build .
```

## Code Coverage

### Generate Coverage Report

```bash
# Configure with coverage
cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON ..
cmake --build .

# Run tests
ctest

# Generate HTML report
lcov --capture --directory . --output-file coverage.info
lcov --remove coverage.info '/usr/*' --output-file coverage.info
genhtml coverage.info --output-directory coverage_report
```

## Installing

```bash
# Install to default location (/usr/local on Unix, C:/Program Files on Windows)
cmake --build . --target install

# Specify custom install prefix
cmake -DCMAKE_INSTALL_PREFIX=/path/to/install ..
cmake --build . --target install
```

## Packaging

### Source Package

```bash
mkdir -p build && cd build
cmake ..
make package_source  # or 'cpack --config CPackSourceConfig.cmake'
```

### Binary Package

```bash
mkdir -p build && cd build
cmake ..
make package  # or 'cpack'
```

## Development Workflow

### Code Formatting

```bash
# Run clang-format on all source files
find src include test -name '*.h' -o -name '*.cpp' | xargs clang-format -i

# Or use the pre-commit hook
ln -s ../../.githooks/pre-commit .git/hooks/pre-commit
```

### Generate Compilation Database

```bash
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON .
ln -s build/compile_commands.json .
```

### IDE Integration

#### Visual Studio Code

1. Install the following extensions:
   - C/C++
   - CMake
   - CMake Tools
   - clangd

2. Configure clangd:
   ```json
   {
       "clangd.arguments": [
           "--compile-commands-dir=${workspaceFolder}/build",
           "--background-index",
           "--clang-tidy",
           "--suggest-missing-includes"
       ]
   }
   ```

#### CLion

1. Open the project directory
2. Select the desired build configuration
3. Build and run from the IDE

## Troubleshooting

### Common Issues

#### Missing Dependencies

```
-- Could NOT find RocksDB (missing: ROCKSDB_LIBRARIES ROCKSDB_INCLUDE_DIR)
```

Solution: Install the required development packages or set `USE_SYSTEM_ROCKSDB=OFF` to use the bundled version.

#### Build Failures on Windows

- Ensure you have the correct Visual Studio version installed
- Make sure you're using the correct generator:
  ```
  cmake -G "Visual Studio 17 2022" -A x64 ..
  ```

#### Test Failures

- Run tests with verbose output:
  ```
  ctest -V
  ```
- Run individual tests with debug output:
  ```
  ./bin/kadedb_test --gtest_filter=TestSuite.TestName --gtest_output=xml:test_results.xml
  ```

### Getting Help

If you encounter any issues, please:

1. Check the [GitHub issues](https://github.com/yourusername/KadeDB/issues)
2. Search the documentation
3. Open a new issue with detailed reproduction steps

## License

KadeDB is licensed under the [MIT License](LICENSE).
