# KadeDB Dependency Management Guide

This document provides comprehensive information about managing dependencies in the KadeDB project.

## Table of Contents

1. [Overview](#overview)
2. [Dependency Management System](#dependency-management-system)
3. [Available Dependencies](#available-dependencies)
4. [Configuration Options](#configuration-options)
5. [Building with System Dependencies](#building-with-system-dependencies)
6. [Building with Bundled Dependencies](#building-with-bundled-dependencies)
7. [Adding New Dependencies](#adding-new-dependencies)
8. [Troubleshooting](#troubleshooting)
9. [Best Practices](#best-practices)

## Overview

KadeDB uses a flexible dependency management system that supports:

- **System Dependencies**: Using pre-installed system libraries
- **Bundled Dependencies**: Automatically downloading and building dependencies
- **Version Pinning**: Specific versions for reproducible builds
- **Cross-Platform Support**: Works on Linux, macOS, and Windows

## Dependency Management System

The dependency management is handled by CMake with the following components:

- `cmake/dependencies.cmake`: Main dependency management logic
- `cmake/Modules/`: Custom Find modules for dependencies
- `CMakeLists.txt`: Project configuration and build settings

## Available Dependencies

### Core Dependencies

| Dependency | Version | System Package | Bundled | Required |
|------------|---------|----------------|---------|----------|
| RocksDB    | 8.0.0   | librocksdb-dev | Yes     | Yes      |
| ANTLR4     | 4.9.3   | libantlr4-runtime-dev | Yes | Yes |
| OpenSSL    | 1.1.1   | libssl-dev     | No      | Yes      |

### Optional Dependencies

| Dependency | Version | System Package | Bundled | Required |
|------------|---------|----------------|---------|----------|
| Google Test | 1.11.0 | libgtest-dev   | Yes     | No       |
| Google Benchmark | 1.7.0 | libbenchmark-dev | Yes  | No       |

## Configuration Options

### Build Options

| Option | Description | Default |
|--------|-------------|---------|
| `USE_SYSTEM_DEPS` | Use system-installed dependencies | `OFF` |
| `USE_SYSTEM_ROCKSDB` | Use system RocksDB | Value of `USE_SYSTEM_DEPS` |
| `USE_SYSTEM_ANTLR` | Use system ANTLR4 | Value of `USE_SYSTEM_DEPS` |
| `BUILD_TESTS` | Build tests | `OFF` |
| `BUILD_BENCHMARKS` | Build benchmarks | `OFF` |

### Version Control

You can specify versions for dependencies using these CMake variables:

```cmake
-DKADEDB_ROCKSDB_VERSION=8.0.0 \
-DKADEDB_ANTLR_VERSION=4.9.3 \
-DKADEDB_OPENSSL_VERSION=1.1.1
```

## Building with System Dependencies

### Prerequisites

On Ubuntu/Debian:

```bash
sudo apt-get update
sudo apt-get install -y \
    librocksdb-dev \
    libantlr4-runtime-dev \
    libssl-dev \
    pkg-config \
    build-essential \
    cmake \
    git
```

### Build Commands

```bash
mkdir -p build && cd build
cmake .. \
    -DUSE_SYSTEM_DEPS=ON \
    -DBUILD_TESTS=ON \
    -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

## Building with Bundled Dependencies

### Prerequisites

```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    git \
    python3-pip
pip3 install -r requirements-build.txt
```

### Build Commands

```bash
mkdir -p build && cd build
cmake .. \
    -DUSE_SYSTEM_DEPS=OFF \
    -DBUILD_TESTS=ON \
    -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

## Adding New Dependencies

To add a new dependency:

1. **Create a Find Module** (if needed):
   - Create `cmake/Modules/Find<Package>.cmake`
   - Implement standard CMake find module logic

2. **Update `dependencies.cmake`**:
   - Add logic to find system package
   - Add FetchContent configuration for bundled version
   - Set up imported targets

3. **Update Documentation**:
   - Add to the Available Dependencies table
   - Document version requirements
   - Add any special build instructions

## Troubleshooting

### Common Issues

1. **Missing System Dependencies**
   - Error: `Could NOT find <Package>`
   - Solution: Install the required system package

2. **Version Mismatch**
   - Error: `Could not find a version matching X.Y.Z`
   - Solution: Update the version in CMake or install the required version

3. **Build Failures**
   - Check the build log for specific errors
   - Try cleaning the build directory and rebuilding
   - Verify that all submodules are initialized

### Debugging

Enable verbose output:

```bash
cmake --build build --verbose
```

View dependency information:

```bash
cd build
cmake -LAH .. | grep -i <package>
```

## Best Practices

1. **Version Pinning**
   - Always pin dependency versions for reproducible builds
   - Update versions in a controlled manner

2. **System vs Bundled**
   - Use system dependencies in production when possible
   - Use bundled dependencies for development and CI

3. **Documentation**
   - Keep dependency documentation up to date
   - Document any special requirements or build flags

4. **Testing**
   - Test with both system and bundled dependencies
   - Verify on multiple platforms

5. **Security**
   - Keep dependencies updated with security patches
   - Use checksums for downloaded dependencies in production

## License

This document is part of the KadeDB project and is licensed under the same terms as the main project.
