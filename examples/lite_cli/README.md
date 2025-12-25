# KadeDB-Lite CLI Example

This is a small interactive CLI demonstrating how to use **KadeDB-Lite**.

## Features

- Open a Lite DB at a given path
- Run Lite SQL-ish queries via `kadedb_lite_execute_query()`
- Pretty-print result sets (columns + rows)
- Commands:
  - `help`
  - `exit` / `quit`
  - `history` (shows entered commands)
  - `!N` (re-run history entry N)
  - `import <table> <csv_path>` (CSV lines of `id,value`)
  - `export <query> <csv_path>` (runs query and writes CSV)

## Build

From the repo root:

```bash
cmake -S . --preset debug
cmake --build --preset debug -j
```

## Run

```bash
./build/debug/bin/kadedb_lite_cli ./tmp_lite_cli_db
```

## Examples

```text
Lite> INSERT INTO users (id, value) VALUES (1, alice)
Lite> SELECT id, value FROM users WHERE id=1
Lite> export SELECT id, value FROM users WHERE id=1 out.csv
Lite> import users data.csv
Lite> history
Lite> !1
```
