# KadeDB Design Notes

This page links to key design choices around memory and copy semantics in the C++ core.

- __Reference-counted strings (`KADEDB_RC_STRINGS`)__
  - Header: `cpp/include/kadedb/value.h` (`StringValue` docs)
  - Build flag: `-DKADEDB_RC_STRINGS=ON|OFF` (default OFF)
  - Behavior: When enabled, `StringValue` holds `std::shared_ptr<std::string>` for reduced copies of large strings. API semantics remain content-based; `clone()` is still deep.

- __Deep vs Shallow row representations__
  - Header: `cpp/include/kadedb/schema.h` (Row vs RowShallow docs)
  - Types:
    - `kadedb::Row` — deep copy; `std::unique_ptr<Value>` per cell; copy/assign clones values.
    - `kadedb::RowShallow` — shallow copy; `std::shared_ptr<Value>` per cell; default copy shares values.
  - Conversions:
    - `RowShallow::fromClones(const Row&)` — one-time deep clone and wrap into shared ownership.
    - `RowShallow::toRowDeep() const` — deepen back into `Row` via `Value::clone()`.

## Quick examples

```cpp
#include <kadedb/schema.h>
#include <kadedb/value.h>
using namespace kadedb;

Row r(2);
r.set(0, std::make_unique<IntegerValue>(42));
r.set(1, std::make_unique<StringValue>("hello"));

RowShallow rs = RowShallow::fromClones(r); // shallow container over cloned values
RowShallow rs2 = rs; // shallow copy; shares values
Row r2 = rs.toRowDeep(); // deep copy
```

## Related tests

- `kadedb_row_shallow_test` — validates shallow aliasing and deep conversions.
- `kadedb_copy_move_test` — validates deep copy/move semantics of `Row` and related types.
- `kadedb_result_utils_test` — validates CSV escaping and JSON emission; ensures string handling is correct.

Run with:

```bash
# Configure with optional memory flags
cmake -S . -B build \
  -DBUILD_TESTS=ON \
  -DKADEDB_RC_STRINGS=ON \
  -DKADEDB_MEM_DEBUG=ON \
  -DKADEDB_ENABLE_SMALL_OBJECT_POOL=ON

cmake --build build -j
ctest --test-dir build --output-on-failure -R "kadedb_(row_shallow|copy_move|result_utils)_test"
```
