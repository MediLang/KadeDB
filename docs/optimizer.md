# KadeDB Query Optimizer (MVP)

This document describes the minimal query optimization performed in the KadeDB executor for the MVP. The optimizer focuses on correctness-first simplifications and predicate pushdown to the storage layer.

## Scope and Goals

- Keep semantics correct while simplifying WHERE predicates.
- Reduce work in the storage scan by normalizing predicates.
- Provide clear error reporting for unknown columns referenced in predicates.
- Avoid introducing a separate planner; integrate a lightweight pass in the executor.

## Where It Runs

- File: `cpp/src/core/query_executor.cpp`
- Entry points:
  - `QueryExecutor::executeSelect()`
  - `QueryExecutor::executeUpdate()`
  - `QueryExecutor::executeDelete()`

For each statement kind, the executor:
1. Builds a predicate from the parsed KadeQL AST (`buildPredicate(...)`).
2. Simplifies the predicate (`simplifyPred(...)`).
3. Validates referenced columns against the table schema (`validatePredicateColumns(...)`).
4. Pushes the simplified predicate into storage APIs (`RelationalStorage::select/updateRows/deleteRows`).

## Predicate Model

- Struct: `Predicate` in `cpp/include/kadedb/storage.h`.
- Supports:
  - `Comparison` with `column`, `op`, and `rhs` (`std::unique_ptr<Value>`)
  - Logical nodes: `And`, `Or`, `Not`
- Empty-children semantics:
  - `And([])` evaluates to true.
  - `Or([])` evaluates to false.
  - `Not([])` evaluates to false.

## Simplification Strategies

Implemented in `query_executor.cpp`:

- **NOT pushdown / Double-NOT elimination**
  - `NOT (a < b)` → `(a >= b)` by inverting comparison operator.
  - `NOT (A AND B)` → `(NOT A) OR (NOT B)` (De Morgan).
  - `NOT (A OR B)` → `(NOT A) AND (NOT B)`.
  - `NOT NOT A` → `A`.

- **Flattening and deduplication**
  - Nested `AND`/`OR` trees flattened to a single level.
  - Child predicates sorted deterministically and deduplicated by a structural key.

- **Logical identities (short-circuit)**
  - `AND(true, X)` → `X`, `AND(false, X)` → `false`.
  - `OR(false, X)` → `X`, `OR(true, X)` → `true`.
  - Implemented using empty-children constants:
    - `And([])` is `true`, `Or([])` is `false`.

- **Constant folding**
  - Literal-vs-literal comparisons are evaluated in `buildPredicate(...)`:
    - e.g. `(1 < 2)` folded to a logical constant predicate.
  - Combined with identities, simplifies surrounding `AND/OR`.

## Column Validation

- Helper: `validatePredicateColumns(...)` in `QueryExecutor`.
- Retrieves the table schema (via `select *`) and checks that all predicate `Comparison` columns exist.
- If an unknown column is found, returns `Status::InvalidArgument("Unknown column in predicate: <name>")`.
- Applied after simplification and before calling storage.

## Storage Pushdown

- Storage applies filters before projection in `cpp/src/core/storage.cpp` (`InMemoryRelationalStorage::select`), achieving selection-before-projection for MVP.
- The simplified predicate is passed unchanged to storage for evaluation.

## Tests

- Canonicalization and folding: `cpp/test/kadeql_optimizer_canonicalization_test.cpp`
- Unknown-column validation: `cpp/test/kadeql_unknown_column_predicate_test.cpp`
- Additional predicate behavior tests:
  - `cpp/test/storage_predicates_test.cpp`
  - `cpp/test/kadeql_not_predicates_test.cpp`

## Notes and Future Work

- Additional constant folding (more expression forms) can be added.
- A fuller planner could support join reordering, index selection, and cost-based strategies in the future.
