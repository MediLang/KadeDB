# Contributing to KadeDB

Thank you for considering contributing to KadeDB! We welcome all contributions, whether they're bug reports, feature requests, documentation improvements, or code contributions.

## Code of Conduct

By participating in this project, you agree to abide by our [Code of Conduct](CODE_OF_CONDUCT.md).

## Getting Started

1. **Fork** the repository on GitHub
2. **Clone** your fork locally
   ```bash
   git clone https://github.com/your-username/KadeDB.git
   cd KadeDB
   ```
3. Set up the development environment (see [Development Setup](#development-setup) below)
4. Create a new branch for your changes
   ```bash
   git checkout -b feature/your-feature-name
   ```
5. Make your changes and commit them
6. Push your changes to your fork
7. Open a **Pull Request**

## Development Setup

### Prerequisites

- C++17 compatible compiler (GCC 9+, Clang 10+, MSVC 2019+)
- CMake 3.14+
- Git
- Python 3.6+ (for build scripts)
- Conan (C++ package manager)

### Building from Source

```bash
# Create build directory
mkdir -p build && cd build

# Configure with CMake
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build . --config Release

# Run tests
ctest --output-on-failure
```

## Coding Standards

- Follow the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)
- Use `clang-format` for code formatting (see `.clang-format`)
- Include unit tests for new features
- Update documentation when adding new features
- Keep commits focused and atomic

## Issue Reporting

When reporting issues, please include:

1. A clear, descriptive title
2. Steps to reproduce the issue
3. Expected vs. actual behavior
4. Environment details (OS, compiler version, etc.)
5. Any relevant logs or error messages

## Pull Requests

1. Keep PRs focused on a single feature or bugfix
2. Ensure all tests pass
3. Update documentation as needed
4. Reference any related issues
5. Follow the PR template when submitting

## License

By contributing to KadeDB, you agree that your contributions will be licensed under the [LICENSE](LICENSE) file in the root directory of this source tree.
