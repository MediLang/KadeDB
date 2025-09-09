# Troubleshooting

Common build and test issues and how to fix them.

## CMake configure errors

- Missing CMake version or compiler
  - Ensure CMake >= 3.21 and a C++17 compiler are installed
  - Linux: `sudo apt-get install cmake g++`

- Presets not found
  - Use: `cmake -S . --preset debug`
  - Or run `cmake -S . -B build/debug -G Ninja` manually if presets are unavailable

## Build failures

- Undefined references in tests
  - Re-run from repo root: `cmake --build --preset debug -j`
  - Ensure `cpp/test/CMakeLists.txt` targets are included

- Missing dependencies (docs)
  - Install: `pip install sphinx breathe myst-parser sphinx-rtd-theme`
  - Install system: `sudo apt-get install doxygen graphviz`

## Test failures

- Run a specific test with verbose output
  ```bash
  ctest --output-on-failure -R kadedb_storage_predicates_test
  ```

- Memory issues on AddressSanitizer
  - Rebuild in a clean tree and ensure `-DENABLE_ASAN=ON` only for Debug

## Docs generation

- Doxygen XML missing for Sphinx/Breathe
  - Run: `doxygen docs/Doxyfile`
  - Confirm XML at `docs/build/xml`

- Sphinx cannot find project `KadeDB`
  - Ensure `docs/sphinx/conf.py` `breathe_projects['KadeDB']` points to `../build/xml`

## CI/CD

- GH Pages not updating
  - Confirm Settings → Pages uses “GitHub Actions”
  - Check `.github/workflows/docs.yml` logs for build errors
