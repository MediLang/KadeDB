# KadeDB Foreign Function Interface (FFI) Guide

## Overview

KadeDB provides a comprehensive C API designed for Foreign Function Interface (FFI) compatibility with Python, Rust, Go, and other languages.

## Memory Ownership Rules

### 1. Opaque Handles
All opaque types must be destroyed using corresponding `Destroy()` functions:

```c
KDB_TableSchema *schema = KadeDB_TableSchema_Create();
// Use schema...
KadeDB_TableSchema_Destroy(schema);
```

### 2. String Parameters
`const char*` parameters are borrowed - the library makes internal copies:

```c
const char *name = "column_name";
KadeDB_TableSchema_AddColumn(schema, &column); // Safe after call returns
```

### 3. Output Buffers
Caller owns and manages output buffers:

```c
char error_buf[256];
int result = KadeDB_ValidateRow(schema, &row, error_buf, sizeof(error_buf));
```

## Error Handling

### Structured Error Reporting

```c
#include "kadedb/kadedb_ffi_helpers.h"

KDB_ErrorInfo error;
kadedb_clear_error(&error);

int result = KadeDB_SomeOperation(&error);
if (!result && kadedb_has_error(&error)) {
    printf("Error %d: %s\n", error.code, error.message);

    switch (error.code) {
        case KDB_ERROR_TYPE_MISMATCH:
            // Handle type mismatch
            break;
        case KDB_ERROR_CONSTRAINT_VIOLATION:
            // Handle constraint violation
            break;
    }
}
```

### Error Codes

| Code | Description |
|------|-------------|
| `KDB_SUCCESS` | Operation successful |
| `KDB_ERROR_INVALID_ARGUMENT` | Invalid function argument |
| `KDB_ERROR_OUT_OF_RANGE` | Index out of bounds |
| `KDB_ERROR_TYPE_MISMATCH` | Value type mismatch |
| `KDB_ERROR_CONSTRAINT_VIOLATION` | Constraint violation |

## Opaque Handle Patterns

### Value Handles

```c
// Create values
KDB_ValueHandle *int_val = KadeDB_Value_CreateInteger(42);
KDB_ValueHandle *str_val = KadeDB_Value_CreateString("hello");

// Use values
KDB_ErrorInfo error;
long long value = KadeDB_Value_AsInteger(int_val, &error);

// Cleanup
KadeDB_Value_Destroy(int_val);
KadeDB_Value_Destroy(str_val);
```

### Safe Destruction

```c
#include "kadedb/kadedb_ffi_helpers.h"

KDB_TableSchema *schema = KadeDB_TableSchema_Create();
// Use schema...
KADEDB_SAFE_DESTROY(TableSchema, schema); // Sets schema to NULL
```

## Language Integration Examples

### Python (ctypes)

```python
import ctypes

# Load library
lib = ctypes.CDLL('./libkadedb_c.so')

# Define types
class KDB_ErrorInfo(ctypes.Structure):
    _fields_ = [
        ("code", ctypes.c_int),
        ("message", ctypes.c_char * 512),
        ("context", ctypes.c_char * 256),
        ("line", ctypes.c_int)
    ]

class ValueHandle:
    def __init__(self, handle):
        self._handle = handle

    def __del__(self):
        if self._handle:
            lib.KadeDB_Value_Destroy(self._handle)

    @classmethod
    def create_integer(cls, value):
        handle = lib.KadeDB_Value_CreateInteger(value)
        return cls(handle)

# Function signatures
lib.KadeDB_Value_CreateInteger.argtypes = [ctypes.c_longlong]
lib.KadeDB_Value_CreateInteger.restype = ctypes.c_void_p
```

### Rust

```rust
use std::ffi::{CStr, CString};

#[repr(C)]
pub struct KDBErrorInfo {
    pub code: i32,
    pub message: [i8; 512],
    pub context: [i8; 256],
    pub line: i32,
}

extern "C" {
    fn KadeDB_Value_CreateInteger(value: i64) -> *mut std::ffi::c_void;
    fn KadeDB_Value_Destroy(value: *mut std::ffi::c_void);
    fn kadedb_clear_error(error: *mut KDBErrorInfo);
}

pub struct Value {
    handle: *mut std::ffi::c_void,
}

impl Value {
    pub fn new_integer(value: i64) -> Option<Self> {
        let handle = unsafe { KadeDB_Value_CreateInteger(value) };
        if handle.is_null() {
            None
        } else {
            Some(Value { handle })
        }
    }
}

impl Drop for Value {
    fn drop(&mut self) {
        if !self.handle.is_null() {
            unsafe { KadeDB_Value_Destroy(self.handle) };
        }
    }
}
```

### Go (cgo)

```go
package kadedb

/*
#cgo LDFLAGS: -lkadedb_c
#include "kadedb/kadedb.h"
#include "kadedb/kadedb_ffi_helpers.h"
*/
import "C"
import (
    "runtime"
    "unsafe"
)

type Value struct {
    handle C.KDB_ValueHandle
}

func NewIntegerValue(value int64) *Value {
    handle := C.KadeDB_Value_CreateInteger(C.longlong(value))
    if handle == nil {
        return nil
    }

    v := &Value{handle: handle}
    runtime.SetFinalizer(v, (*Value).destroy)
    return v
}

func (v *Value) destroy() {
    if v.handle != nil {
        C.KadeDB_Value_Destroy(v.handle)
        v.handle = nil
    }
}
```

## Best Practices

### 1. Always Initialize Error Structures
```c
KDB_ErrorInfo error;
kadedb_clear_error(&error);
```

### 2. Check Return Values
```c
if (!KadeDB_SomeOperation(&error)) {
    // Handle error
}
```

### 3. Use Resource Management
```c
KDB_ResourceManager manager;
kadedb_resource_manager_init(&manager, 10);

// Add resources
kadedb_resource_manager_add(&manager, schema,
                           (void(*)(void*))KadeDB_TableSchema_Destroy);

// Automatic cleanup
kadedb_resource_manager_cleanup(&manager);
```

### 4. Thread Safety
- Individual handles are NOT thread-safe
- Use external synchronization for shared access
- Each thread should use its own `KDB_ErrorInfo`

## Common Anti-Patterns

### ❌ Don't: Forget to destroy resources
```c
KDB_TableSchema *schema = KadeDB_TableSchema_Create();
// Missing: KadeDB_TableSchema_Destroy(schema);
```

### ❌ Don't: Use destroyed handles
```c
KadeDB_TableSchema_Destroy(schema);
KadeDB_TableSchema_AddColumn(schema, &col); // Use after free!
```

### ❌ Don't: Ignore error codes
```c
KadeDB_SomeOperation(); // No error checking
```

### ✅ Do: Use safe patterns
```c
KDB_ErrorInfo error;
kadedb_clear_error(&error);

KDB_TableSchema *schema = KadeDB_TableSchema_Create();
if (!schema) return -1;

if (!KadeDB_SomeOperation(&error)) {
    printf("Error: %s\n", error.message);
    KADEDB_SAFE_DESTROY(TableSchema, schema);
    return -1;
}

KADEDB_SAFE_DESTROY(TableSchema, schema);
```

## Performance Tips

1. **Use view-based APIs for bulk operations**
2. **Minimize handle creation/destruction in loops**
3. **Use shallow copy rows when sharing data**
4. **Batch operations when possible**
5. **Pre-allocate buffers for repeated operations**

## Debugging

Enable memory debugging:
```c
#ifdef KADEDB_MEM_DEBUG
void check_leaks() {
    if (kadedb_check_resource_leaks()) {
        printf("Resource leaks detected!\n");
        kadedb_print_memory_stats();
    }
}
#endif
```

For more details, see the API reference and language-specific examples.
