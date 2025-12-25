# C Example (C ABI)

This example demonstrates usage of the **KadeDB C ABI**:
- Create an in-memory storage
- Build a table schema
- Create a table
- Insert a row
- Execute a simple query (`SELECT * FROM <table>`) and iterate results

## Build

From the repo root:

```bash
cmake -S . --preset debug
cmake --build --preset debug -j
```

## Run

```bash
./build/debug/bin/kadedb_c_example
```
