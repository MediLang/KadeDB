# Contributing

Thank you for considering a contribution to KadeDB!

## How to get started

1. Fork the repository and create a feature branch
2. Build with CMake presets and run the full test suite
3. Add or update tests for your change
4. Open a Pull Request with a clear description and checklist

## Code style

- C++17, warnings enabled, prefer modern C++ constructs
- Public headers live under `cpp/include/kadedb/`
- Use `Result<T>` and `Status` for error handling across APIs
- Add Doxygen comments and group tags (`@defgroup`, `@ingroup`)

## Documentation

- Update or add pages under `docs/sphinx/`
- API surface changes should include Doxygen comments in headers
- Build docs locally:
  ```bash
  doxygen docs/Doxyfile
  sphinx-build -b html docs/sphinx docs/sphinx/_build/html
  ```

## Tests

- Put new tests under `cpp/test/`
- Run targets with `ctest --output-on-failure`

## Where to help

- Examples in `cpp/examples/`
- Predicate builders and docs
- Storage API semantics and edge cases

See also the repository root `CONTRIBUTING.md` for project-wide guidance.
