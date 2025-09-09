# Getting Started

Welcome to KadeDB development. This guide helps you build the core, run tests, and explore examples quickly.

## Prerequisites

- CMake 3.21+
- A C++17 compiler (GCC/Clang/MSVC)
- Python (optional, for Sphinx docs)

## Build the Core (via CMake Presets)

```bash
cmake -S . --preset debug
cmake --build --preset debug -j
```

Run tests:

```bash
ctest --output-on-failure
```

## Explore the Relational Storage Examples

- `cpp/examples/inmemory_rel_example.cpp` (create/insert/select, AND/OR/NOT)
- `cpp/examples/inmemory_rel_errors_example.cpp` (invalid schema, duplicate unique, composite predicates)

Build targets are created automatically; binaries are in `build/<preset>/bin/`.

## Key Tests to Read

- `cpp/test/storage_api_test.cpp` — end-to-end relational API semantics
- `cpp/test/storage_predicates_test.cpp` — AND/OR/NOT and nested filters, plus corner cases
- `cpp/test/document_predicates_test.cpp` — document queries with composite predicates

Run a subset of tests:

```bash
# From the build directory for your preset
ctest --output-on-failure -R "kadedb_(storage|document)_predicates_test"
```

## Developer Docs (API Reference)

We generate Doxygen XML and render Sphinx HTML. The CI workflow publishes to GitHub Pages.

- Online: https://medilang.github.io/KadeDB/
- Locally:
  ```bash
  doxygen docs/Doxyfile
  sphinx-build -b html docs/sphinx docs/sphinx/_build/html
  ```

## Where to Start Contributing

- Improve docs or add examples under `cpp/examples/`
- Add tests in `cpp/test/`
- Extend the in-memory storages or add new API groups

## Style and Conventions

- Public headers are under `cpp/include/kadedb/`
- Core implementations are under `cpp/src/core/`
- Use `@defgroup` / `@ingroup` in headers to structure API docs
- Prefer `Result<T>` and `Status` for error handling in new APIs
