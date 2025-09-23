# Diagnostics Logs Layout

This directory consolidates diagnostic logs from CI and local runs.

Structure:

- `diagnostics/asan/<run-id>-<shortsha>/`
  - `asan_ctest_output.txt`
  - `asan_binary_output.txt`
  - `asan_gdb_bt.txt`
- `diagnostics/gdb/<run-id>-<shortsha>/`
  - `gdb_release_bt.txt`
- `diagnostics/latest/`
  - `asan` -> symlink to latest ASAN directory
  - `gdb` -> symlink to latest GDB directory

Manual imports (local runs) use `manual-<timestamp>-<suffix>` directories.

## Fetching artifacts from CI

Use GitHub CLI to download artifacts into a new run-scoped directory:

```bash
# ASAN artifacts
RUN_ID=1234567890
SHA=$(git rev-parse --short HEAD)
mkdir -p diagnostics/asan/${RUN_ID}-${SHA}
gh run download $RUN_ID --name asan-integration-logs --dir diagnostics/asan/${RUN_ID}-${SHA}
ln -snf ../asan/${RUN_ID}-${SHA} diagnostics/latest/asan

# GDB backtrace
mkdir -p diagnostics/gdb/${RUN_ID}-${SHA}
gh run download $RUN_ID --name gdb-release-backtrace --dir diagnostics/gdb/${RUN_ID}-${SHA}
ln -snf ../gdb/${RUN_ID}-${SHA} diagnostics/latest/gdb
```

## Notes

- Logs are intentionally ignored by git; this README is tracked.
- Adjust `RUN_ID` and `SHA` as needed.
