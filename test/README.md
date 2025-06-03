# KadeDB Tests

This directory contains the test suite for KadeDB, including unit tests, integration tests, and benchmarks.

## Directory Structure

- `unit/`: Unit tests for individual components
- `integration/`: Integration tests for component interactions
- `benchmark/`: Performance benchmarks

## Running Tests

### Prerequisites

- Google Test (for unit and integration tests)
- Google Benchmark (for benchmarks)
- CMake 3.14 or higher

### Building and Running Tests

1. Configure the build:
   ```bash
   mkdir -p build && cd build
   cmake .. -DBUILD_TESTS=ON
   ```

2. Build the tests:
   ```bash
   cmake --build . --target all test
   ```

3. Run all tests:
   ```bash
   ctest --output-on-failure
   ```

4. Run specific test suites:
   ```bash
   # Run unit tests
   ctest -R unit_ --output-on-failure
   
   # Run integration tests
   ctest -R integration_ --output-on-failure
   
   # Run benchmarks
   ctest -R benchmark_ --output-on-failure
   ```

## Writing Tests

### Unit Tests

1. Create a new test file in the `unit/` directory
2. Follow the naming convention: `test_<component>_<functionality>.cpp`
3. Include the necessary headers and write your tests using the Google Test framework

### Integration Tests

1. Create a new test file in the `integration/` directory
2. Follow the naming convention: `test_integration_<feature>.cpp`
3. Test interactions between components

### Benchmarks

1. Create a new benchmark file in the `benchmark/` directory
2. Follow the naming convention: `benchmark_<component>_<operation>.cpp`
3. Use the Google Benchmark framework to measure performance

## Test Coverage

To generate a test coverage report:

```bash
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON
cmake --build . --target coverage
```

The coverage report will be available in `build/coverage/index.html`.

## Continuous Integration

Tests are automatically run on every push and pull request using GitHub Actions. The CI configuration can be found in `.github/workflows/`.
