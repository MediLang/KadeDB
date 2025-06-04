# KadeDB Development Guidelines

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

1. **CMake Configuration**:
   - Use `option()` for build-time configurations.
   - Document all options in the main `CMakeLists.txt`.
   - Filter sensitive environment variables.

2. **Dependencies**:
   - Prefer system packages when available.
   - Pin dependency versions for reproducible builds.
   - Document all dependencies in `docs/dependency_management.md`.

## Code Style

1. **Formatting**:
   - Use `clang-format` for C++ code.
   - Configure your editor to format on save.

2. **Naming Conventions**:
   - Use descriptive names for variables and functions.
   - Follow the project's naming conventions.

## Testing

1. **Unit Tests**:
   - Write tests for all new features.
   - Keep tests focused and independent.
   - Mock external dependencies.

2. **Integration Tests**:
   - Test components together.
   - Use test fixtures for setup/teardown.
   - Clean up after tests.

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
