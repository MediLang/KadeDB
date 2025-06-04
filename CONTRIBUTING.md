# Contributing to KadeDB

Thank you for considering contributing to KadeDB! We welcome all contributions, whether they're bug reports, feature requests, documentation improvements, or code contributions.

## Table of Contents

- [Code of Conduct](#code-of-conduct)
- [Getting Started](#getting-started)
- [Development Workflow](#development-workflow)
- [Development Setup](#development-setup)
- [Coding Standards](#coding-standards)
- [Testing](#testing)
- [Code Review Process](#code-review-process)
- [Issue Reporting](#issue-reporting)
- [Pull Requests](#pull-requests)
- [Release Process](#release-process)
- [Community](#community)
- [License](#license)

## Code of Conduct

By participating in this project, you agree to abide by our [Code of Conduct](CODE_OF_CONDUCT.md). Please read it before making any contributions.

## Getting Started

1. **Fork** the repository on GitHub
2. **Clone** your fork locally
   ```bash
   git clone https://github.com/your-username/KadeDB.git
   cd KadeDB
   git submodule update --init --recursive
   ```
3. Set up the development environment (see [Development Setup](#development-setup) below)
4. Create a new branch for your changes
   ```bash
   git checkout -b type/scope/short-description
   # Example: git checkout -b feat/storage/add-new-engine
   ```
5. Make your changes and commit them following our [commit message guidelines](#commit-message-format)
6. Push your changes to your fork
7. Open a **Pull Request** following our [PR guidelines](#pull-requests)

## Development Workflow

### Branch Naming

Use the following format for branch names:
```
type/scope/short-description
```

Where `type` is one of:
- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation changes
- `style`: Code style/formatting
- `refactor`: Code changes that neither fix bugs nor add features
- `perf`: Performance improvements
- `test`: Adding or modifying tests
- `chore`: Maintenance tasks

Example: `feat/storage/add-rocksdb-backend`

### Commit Message Format

We follow the [Conventional Commits](https://www.conventionalcommits.org/) specification:

```
<type>[optional scope]: <description>

[optional body]

[optional footer(s)]
```

Example:
```
feat(storage): add support for RocksDB backend

- Implement RocksDB storage engine
- Add configuration options
- Update documentation

Closes #123
```

### Development Setup

#### Prerequisites

- **C++17** compatible compiler (GCC 9+, Clang 10+, MSVC 2019+)
- **CMake 3.14+**
- **Git**
- **Python 3.8+** (for build scripts)
- **Conan** (C++ package manager)
- **Docker** (optional, for containerized development)

#### Quick Start

```bash
# Clone the repository
$ git clone --recurse-submodules https://github.com/your-username/KadeDB.git
$ cd KadeDB

# Set up development environment (Linux/macOS)
$ ./scripts/setup_dev_env.sh

# Or on Windows
> .\scripts\setup_dev_env.ps1

# Build the project
$ mkdir -p build && cd build
$ cmake .. -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
$ cmake --build . -j$(nproc)

# Run tests
$ ctest --output-on-failure
```

#### IDE Setup

##### Visual Studio Code
1. Install the following extensions:
   - C/C++
   - CMake Tools
   - CMake Language Support
   - clangd
2. Open the project folder
3. Select the `clangd` configuration (bottom-right)
4. Use the CMake extension to configure and build

##### CLion
1. Open the project folder
2. Select the `clang` or `gcc` toolchain
3. Let CLion index the project
4. Use the built-in CMake tools to build and run

## Coding Standards

### C++ Style Guide

We follow the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html) with the following additions:

- Line length: 100 characters
- Use `#pragma once` for include guards
- Prefer `constexpr` over `#define` for constants
- Use `nullptr` instead of `NULL`
- Use `auto` when the type is obvious from the context

### Code Formatting

We use `clang-format` for code formatting. The configuration is in `.clang-format`.

To format your code:
```bash
# Format all source files
$ ./scripts/format.sh

# Format only changed files
$ ./scripts/format-changed.sh
```

### Documentation

- Document all public APIs using Doxygen-style comments
- Keep documentation up-to-date with code changes
- Add examples for complex functionality
- Document design decisions in the code or ADRs (Architecture Decision Records)

## Testing

### Writing Tests

- Write unit tests for all new features
- Keep tests focused and independent
- Use test fixtures for common setup/teardown
- Follow the [Testing Guidelines](docs/testing.md)

### Running Tests

```bash
# Run all tests
$ cd build && ctest --output-on-failure

# Run specific test
$ cd build && ctest -R test_name

# Run tests with debug output
$ cd build && ctest --verbose

# Run tests in parallel
$ cd build && ctest --output-on-failure -j$(nproc)
```

## Code Review Process

1. Create a draft PR early for feedback
2. Request reviews from relevant team members
3. Address all review comments
4. Ensure all CI checks pass
5. Get at least one approval before merging
6. Squash and merge PRs with a descriptive commit message

## Issue Reporting

When reporting issues, please include:

1. A clear, descriptive title
2. Steps to reproduce the issue
3. Expected vs. actual behavior
4. Environment details:
   - OS and version
   - Compiler version
   - KadeDB version or commit hash
5. Any relevant logs or error messages
6. Screenshots if applicable

## Pull Requests

1. **Before submitting a PR**:
   - Ensure your branch is up-to-date with `main`
   - Run all tests locally
   - Update documentation if needed
   - Ensure code is properly formatted

2. **PR Guidelines**:
   - Keep PRs focused on a single feature or bugfix
   - Reference related issues using keywords (e.g., `Closes #123`)
   - Follow the PR template
   - Include tests for new features
   - Update documentation as needed

3. **After submission**:
   - Address all CI failures
   - Respond to code review comments
   - Update the PR with any requested changes

## Release Process

1. Update version numbers in:
   - `CMakeLists.txt`
   - `include/kadedb/version.h`
   - Documentation
   
2. Create a release branch:
   ```bash
   git checkout -b release/vX.Y.Z
   ```

3. Update CHANGELOG.md with release notes

4. Create a signed tag:
   ```bash
   git tag -s vX.Y.Z -m "Release vX.Y.Z"
   git push origin vX.Y.Z
   ```

5. Create a GitHub release with the release notes

6. Merge the release branch into `main` and `develop`

## Community

- Join our [Discord/Slack]() for discussions
- Follow us on [Twitter]()
- Check out our [blog]() for updates

## License

By contributing to KadeDB, you agree that your contributions will be licensed under the [Apache License 2.0](LICENSE).

---

Thank you for your interest in contributing to KadeDB! Your contributions help make this project better for everyone.
