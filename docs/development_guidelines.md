# KadeDB Development Guidelines

## Table of Contents
- [Development Environment Setup](#development-environment-setup)
- [Build System](#build-system)
- [Code Style](#code-style)
- [Testing](#testing)
- [Version Control](#version-control)
- [Security](#security)
- [Documentation](#documentation)
- [CI/CD](#cicd)
- [Troubleshooting](#troubleshooting)

## Development Environment Setup

### Prerequisites

- **C++17 compatible compiler** (GCC 8+, Clang 7+, MSVC 2019+)
- **CMake 3.14+**
- **Git**
- **Python 3.8+** (for build scripts)
- **Conan** (package manager)
- **Docker** (optional, for containerized development)

### Quick Start

```bash
# Clone the repository
$ git clone --recurse-submodules https://github.com/MediLang/KadeDB.git
$ cd KadeDB

# Set up development environment (Linux/macOS)
$ ./scripts/setup_dev_env.sh

# Or on Windows
> .\scripts\setup_dev_env.ps1

# Build the project
$ mkdir -p build && cd build
$ cmake .. -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
$ make -j$(nproc)

# Run tests
$ ctest --output-on-failure
```

### IDE Setup

#### Visual Studio Code
1. Install the following extensions:
   - C/C++
   - CMake Tools
   - CMake Language Support
   - clangd
2. Open the project folder
3. Select the `clangd` configuration (bottom-right)
4. Use the CMake extension to configure and build

#### CLion
1. Open the project folder
2. Select the `clang` or `gcc` toolchain
3. Let CLion index the project
4. Use the built-in CMake tools to build and run

### Environment Configuration

1. Copy the example environment file:
   ```bash
   $ cp ENV.example .env
   ```
2. Update the `.env` file with your local configuration
3. Never commit the `.env` file to version control

## Handling Sensitive Information

### Environment Variables

1. **Never commit sensitive information** such as API keys, tokens, or passwords to version control.
2. **Use environment variables** for configuration that differs between environments.
3. **Prefix sensitive environment variables** with `KADEDB_` for project-specific variables.
4. **Document required environment variables** in `ENV.md` (see below).

### Environment Variables Documentation

Create an `ENV.md` file in your project root with the following template:

```markdown
# Environment Variables

## Required
- `KADEDB_DB_URI`: Database connection string
- `KADEDB_LOG_LEVEL`: Logging level (debug, info, warn, error)

## Optional
- `KADEDB_CACHE_SIZE`: Cache size in MB (default: 1024)
- `KADEDB_MAX_CONNECTIONS`: Maximum database connections (default: 10)

## Development Only
- `KADEDB_DEV_MODE`: Enable development features (default: false)
```

### .env Files

1. Create a `.env.example` file with placeholder values.
2. Add `.env` to `.gitignore`.
3. Never commit actual `.env` files.

Example `.env.example`:
```env
# Database
DB_URI=postgresql://user:password@localhost:5432/kadedb

# Logging
LOG_LEVEL=info

# Optional Settings
CACHE_SIZE=1024
MAX_CONNECTIONS=10

# Development
DEV_MODE=true
```

## Build System

### CMake Configuration

KadeDB uses CMake as its build system generator. The main build configuration is in the root `CMakeLists.txt` file.

#### Build Types

- `Debug`: Debug symbols, no optimizations, assertions enabled
- `Release`: Optimizations enabled, debug symbols stripped
- `RelWithDebInfo`: Optimizations with debug symbols
- `MinSizeRel`: Optimized for size

#### Common CMake Options

```bash
# Configure with default options
$ cmake -B build -DCMAKE_BUILD_TYPE=Debug

# Common configuration options
$ cmake -B build \
  -DCMAKE_BUILD_TYPE=Debug \
  -DUSE_SYSTEM_DEPS=ON \
  -DBUILD_TESTS=ON \
  -DBUILD_BENCHMARKS=OFF \
  -DENABLE_ASAN=ON \
  -DENABLE_UBSAN=ON
```

#### Dependency Management

KadeDB supports both system-installed and bundled dependencies. By default, it will use bundled dependencies.

```bash
# Use system-installed dependencies
$ cmake -B build -DUSE_SYSTEM_DEPS=ON

# Specify custom dependency paths
$ cmake -B build \
  -DROCKSDB_ROOT=/path/to/rocksdb \
  -DANTLR4_ROOT=/path/to/antlr4 \
  -DOPENSSL_ROOT_DIR=/path/to/openssl
```

#### Testing Options

```bash
# Build and run tests
$ cmake --build build --target test

# Run specific test
$ cd build && ctest -R test_name

# Run tests with verbose output
$ cd build && ctest --output-on-failure -V
```

### Build Artifacts

- Libraries: `build/lib/`
- Executables: `build/bin/`
- Test binaries: `build/test/`
- Generated headers: `build/include/`
- Documentation: `build/docs/`

### Cross-Compiling

To cross-compile for a different platform:

```bash
$ cmake -B build \
  -DCMAKE_TOOLCHAIN_FILE=/path/to/toolchain.cmake \
  -DCMAKE_BUILD_TYPE=Release
```

### Package Management

KadeDB can be used as a dependency in other CMake projects:

```cmake
find_package(KadeDB REQUIRED)
target_link_libraries(your_target PRIVATE KadeDB::Core)
```

### Build System Hooks

- `pre-commit`: Runs clang-format and basic checks
- `pre-push`: Runs tests and checks before pushing
- `post-checkout`: Updates submodules and dependencies

## Code Style

1. **Formatting**:
   - Use `clang-format` for C++ code.
   - Configure your editor to format on save.

2. **Naming Conventions**:
   - Use descriptive names for variables and functions.
   - Follow the project's naming conventions.

## Testing

KadeDB uses Google Test for unit testing and Google Benchmark for performance testing.

### Running Tests

```bash
# Run all tests
$ cd build && ctest --output-on-failure

# Run specific test
$ cd build && ctest -R test_name

# Run tests with debug output
$ cd build && ctest --verbose

# Run tests in parallel (e.g., 4 jobs)
$ cd build && ctest --output-on-failure -j4
```

### Writing Unit Tests

1. **Test File Organization**
   - Place tests in `test/unit/` directory
   - Name test files as `test_<module>.cpp`
   - Group related tests in test suites

2. **Test Structure**
   ```cpp
   #include <gtest/gtest.h>
   #include "kadedb/core/db.h"

   class DatabaseTest : public ::testing::Test {
   protected:
       void SetUp() override {
           // Setup code
           db = std::make_unique<kadedb::core::Database>();
       }

       void TearDown() override {
           // Cleanup code
           db.reset();
       }

       std::unique_ptr<kadedb::core::Database> db;
   };

   TEST_F(DatabaseTest, BasicOperations) {
       EXPECT_TRUE(db->open(":memory:"));
       EXPECT_TRUE(db->is_open());
       
       // Test operations
       EXPECT_TRUE(db->put("key", "value"));
       std::string value;
       EXPECT_TRUE(db->get("key", &value));
       EXPECT_EQ(value, "value");
   }
   ```

### Integration Tests

1. **Test Organization**
   - Place in `test/integration/`
   - Test component interactions
   - Use test fixtures for complex setups

2. **Best Practices**
   - Use test databases (e.g., in-memory or temporary files)
   - Clean up test data after each test
   - Test error conditions and edge cases

### Benchmarking

KadeDB uses Google Benchmark for performance testing:

```cpp
#include <benchmark/benchmark.h>

static void BM_DatabaseInsert(benchmark::State& state) {
    kadedb::core::Database db;
    db.open(":memory:");
    
    for (auto _ : state) {
        db.put("key" + std::to_string(state.iterations() % 1000), 
               "value" + std::to_string(state.iterations()));
    }
}
BENCHMARK(BM_DatabaseInsert);

BENCHMARK_MAIN();
```

Run benchmarks with:
```bash
$ ./build/test/benchmarks [--benchmark_filter=pattern] [--benchmark_min_time=0.5]
```

### Test Coverage

Generate coverage report:
```bash
$ mkdir -p build && cd build
$ cmake .. -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON
$ make -j$(nproc)
$ ctest --output-on-failure
$ lcov --capture --directory . --output-file coverage.info
$ genhtml coverage.info --output-directory coverage
```

### Mocking

Use Google Mock for mocking:
```cpp
#include <gmock/gmock.h>

class MockStorage : public kadedb::core::StorageInterface {
public:
    MOCK_METHOD(bool, open, (const std::string& path), (override));
    MOCK_METHOD(bool, put, (const std::string& key, const std::string& value), (override));
    MOCK_METHOD(bool, get, (const std::string& key, std::string* value), (const, override));
};

TEST(DatabaseTest, UsesStorage) {
    MockStorage mock;
    EXPECT_CALL(mock, open(":memory:")).WillOnce(Return(true));
    
    kadedb::core::Database db(&mock);
    EXPECT_TRUE(db.open(":memory:"));
}
```

### Continuous Integration

- All tests must pass before merging to `main`
- Code coverage is tracked and should not decrease
- Performance regressions are flagged in CI

## Security

1. **Input Validation**:
   - Validate all user input.
   - Use prepared statements for database queries.

2. **Dependencies**:
   - Keep dependencies up to date.
   - Audit dependencies for security vulnerabilities.

## Documentation

1. **Code Documentation**:
   - Document public APIs.
   - Use Doxygen-style comments for C++.
   - Keep documentation up to date.

2. **Project Documentation**:
   - Update `README.md` for major changes.
   - Document architecture decisions in `docs/`.

## Version Control

1. **Branching**:
   - Use feature branches for new features.
   - Create pull requests for code review.
   - Squash commits before merging.

2. **Commit Messages**:
   - Use the conventional commit format:
     ```
     type(scope): description
     
     [optional body]
     
     [optional footer]
     ```
   - Types: feat, fix, docs, style, refactor, test, chore

## CI/CD

1. **Automated Testing**:
   - Run tests on all pushes and PRs.
   - Enforce code style checks.

2. **Security Scanning**:
   - Scan for vulnerabilities in dependencies.
   - Use secret scanning for API keys.

## Performance

1. **Profiling**:
   - Profile performance-critical code.
   - Use benchmarks to measure improvements.

2. **Optimization**:
   - Optimize based on profiling data.
   - Document performance characteristics.

## License

This document is part of the KadeDB project and is licensed under the same terms as the main project.
