# KadeDB Dependency Management

This directory contains the dependency management system for KadeDB. It provides a flexible way to handle both system-installed and bundled dependencies.

## Overview

The dependency management system is designed to:

1. Support both system-installed and bundled dependencies
2. Provide version control for all dependencies
3. Handle transitive dependencies automatically
4. Support cross-platform builds
5. Integrate with the build system

## Dependencies

### Required Dependencies

- **RocksDB**: Embedded key-value store
  - Version: 8.0.0 (default)
  - System package: `librocksdb-dev` (Ubuntu/Debian)
  - Homepage: https://rocksdb.org/

- **ANTLR4**: Parser generator runtime
  - Version: 4.9.3 (default)
  - System package: `libantlr4-runtime-dev` (Ubuntu/Debian)
  - Homepage: https://www.antlr.org/

- **OpenSSL**: Cryptography and SSL/TLS
  - Version: 1.1.1 (default)
  - System package: `libssl-dev` (Ubuntu/Debian)
  - Homepage: https://www.openssl.org/

### Optional Dependencies

- **Google Test**: Testing framework (enabled with `BUILD_TESTS=ON`)
- **Google Benchmark**: Benchmarking framework (enabled with `BUILD_BENCHMARKS=ON`)

## Usage

### Using System Dependencies

To use system-installed dependencies:

```bash
mkdir -p build && cd build
cmake .. -DUSE_SYSTEM_DEPS=ON
```

### Using Bundled Dependencies (Default)

To use bundled dependencies (automatically downloaded and built):

```bash
mkdir -p build && cd build
cmake ..
```

### Customizing Dependency Versions

You can specify custom versions for dependencies:

```bash
cmake .. \
  -DKADEDB_ROCKSDB_VERSION=8.0.0 \
  -DKADEDB_ANTLR_VERSION=4.9.3 \
  -DKADEDB_OPENSSL_VERSION=1.1.1
```

### Building with Tests and Benchmarks

To build with tests and benchmarks:

```bash
cmake .. -DBUILD_TESTS=ON -DBUILD_BENCHMARKS=ON
```

## Adding New Dependencies

To add a new dependency:

1. Add a new section in `cmake/dependencies.cmake`
2. Use `FetchContent` for bundled dependencies
3. Create a `Find<Package>.cmake` module in `cmake/Modules/` if needed
4. Update the documentation in this file

## Troubleshooting

### Missing System Dependencies

If you get errors about missing system dependencies, install them using your system package manager:

```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install -y librocksdb-dev libantlr4-runtime-dev libssl-dev

# RHEL/CentOS
sudo yum install -y rocksdb-devel antlr4-runtime openssl-devel
```

### Build Issues

If you encounter build issues:

1. Clean the build directory: `rm -rf build/*`
2. Check the CMake output for missing dependencies
3. Verify that you have a working C++17 compiler
4. Check the [GitHub Issues](https://github.com/yourusername/KadeDB/issues) for known issues

## License

This code is part of the KadeDB project and is licensed under the same terms as the main project.
