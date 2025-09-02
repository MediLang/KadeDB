# KadeDB FFI Implementation Guide

## Overview

This document describes the complete Foreign Function Interface (FFI) implementation for KadeDB, including the architecture, design patterns, and usage examples that demonstrate all the requirements from the task.

## Implemented Components

### 1. C-Compatible Struct Representations

**Location**: `bindings/c/include/kadedb/kadedb.h`

The API provides C-compatible data structures that mirror C++ types:

```c
// Basic value container
typedef struct KDB_Value {
    KDB_ValueType type;
    union {
        long long i64;
        double f64;
        const char *str;
        int boolean;
    } as;
} KDB_Value;

// Row representation for bulk operations
typedef struct KDB_RowView {
    const KDB_Value *values;
    unsigned long long count;
} KDB_RowView;

// Column definition with constraints
typedef struct KDB_TableColumnEx {
    const char *name;
    KDB_ColumnType type;
    int nullable;
    int unique;
    const KDB_ColumnConstraints *constraints;
} KDB_TableColumnEx;
```

### 2. Conversion Functions Between C++ and C

**Location**: `bindings/c/src/kadedb_c.cpp` and `bindings/c/src/kadedb_ffi_helpers.cpp`

Helper functions convert between C and C++ representations:

```cpp
// Convert C KDB_Value to C++ Value
static std::unique_ptr<Value> from_c_value(const KDB_Value &v) {
    switch (v.type) {
        case KDB_VAL_NULL: return ValueFactory::createNull();
        case KDB_VAL_INTEGER: return ValueFactory::createInteger(v.as.i64);
        case KDB_VAL_FLOAT: return ValueFactory::createFloat(v.as.f64);
        case KDB_VAL_STRING: return ValueFactory::createString(v.as.str ? std::string(v.as.str) : std::string());
        case KDB_VAL_BOOLEAN: return ValueFactory::createBoolean(v.as.boolean != 0);
    }
}

// Convert C++ Value to C KDB_Value
static void to_c_value(const Value &cpp_val, KDB_Value &c_val) {
    switch (cpp_val.type()) {
        case ValueType::Integer:
            c_val.type = KDB_VAL_INTEGER;
            c_val.as.i64 = cpp_val.asInt();
            break;
        // ... other types
    }
}
```

### 3. Memory Ownership Rules Documentation

**Location**: `bindings/c/FFI_GUIDE.md`

Clear ownership rules are documented and enforced:

- **Opaque Handles**: Caller owns, must call corresponding `Destroy()` function
- **String Parameters**: Borrowed during function call, library makes copies
- **Output Buffers**: Caller owns and manages
- **Return Values**: New handles must be destroyed, string literals are static

### 4. Opaque Pointer Patterns

**Location**: `bindings/c/include/kadedb/kadedb_ffi_helpers.h`

Opaque handles provide type safety and ABI stability:

```c
// Opaque handle declarations
typedef struct KDB_ValueHandle KDB_ValueHandle;
typedef struct KDB_Row KDB_Row;
typedef struct KDB_RowShallow KDB_RowShallow;

// Handle management functions
KDB_ValueHandle *KadeDB_Value_CreateInteger(long long value);
void KadeDB_Value_Destroy(KDB_ValueHandle *value);
KDB_ValueHandle *KadeDB_Value_Clone(const KDB_ValueHandle *value);
```

Implementation wraps C++ objects:

```cpp
struct KDB_ValueHandle {
    std::unique_ptr<Value> impl;
    explicit KDB_ValueHandle(std::unique_ptr<Value> v) : impl(std::move(v)) {}
};
```

### 5. Error Handling Across Language Boundaries

**Location**: `bindings/c/include/kadedb/kadedb_ffi_helpers.h`

Comprehensive error handling system:

```c
typedef enum KDB_ErrorCode {
    KDB_SUCCESS = 0,
    KDB_ERROR_INVALID_ARGUMENT = 1,
    KDB_ERROR_OUT_OF_RANGE = 2,
    // ... more error codes
} KDB_ErrorCode;

typedef struct KDB_ErrorInfo {
    KDB_ErrorCode code;
    char message[512];
    char context[256];
    int line;
} KDB_ErrorInfo;

// Error handling helpers
void kadedb_clear_error(KDB_ErrorInfo *error);
int kadedb_has_error(const KDB_ErrorInfo *error);
const char *kadedb_error_code_string(KDB_ErrorCode code);
```

C++ exceptions are caught and converted:

```cpp
extern "C" long long KadeDB_Value_AsInteger(const KDB_ValueHandle *value, KDB_ErrorInfo *error) {
    kadedb_clear_error(error);

    if (!value || !value->impl) {
        KADEDB_SET_ERROR(error, KDB_ERROR_INVALID_ARGUMENT, "Value handle is null");
        return 0;
    }

    try {
        return static_cast<long long>(value->impl->asInt());
    } catch (const std::exception &e) {
        KADEDB_SET_ERROR(error, KDB_ERROR_TYPE_MISMATCH, e.what());
        return 0;
    }
}
```

### 6. Example C Header Files

**Location**: `bindings/c/include/kadedb/examples.h`

Comprehensive examples showing intended FFI interface patterns:

- Complete table schema workflow
- Value handle manipulation
- Document schema operations
- Resource management patterns
- Bulk data operations
- Language-specific integration helpers

### 7. Comprehensive Documentation

**Location**: `bindings/c/FFI_GUIDE.md`

Complete FFI documentation covering:

- Memory ownership and lifecycle management
- Error handling architecture
- Opaque pointer patterns
- Data type mappings
- Language-specific integration (Python, Rust, Go)
- Best practices and common patterns
- Performance considerations
- Thread safety guidelines

## Key Design Principles Implemented

### 1. Memory Safety
- Clear ownership rules prevent double-free and use-after-free
- Safe destruction macros handle null pointers
- Resource managers provide automatic cleanup

### 2. Error Transparency
- Structured error reporting with codes and messages
- C++ exceptions never cross FFI boundary
- Context information for debugging

### 3. Performance
- View-based APIs for bulk operations
- Zero-copy where possible
- Opaque handles minimize allocations

### 4. ABI Stability
- Opaque pointers hide C++ implementation details
- C-compatible data structures
- Stable function signatures

### 5. Language Compatibility
- Standard C types throughout
- Helper functions for common language patterns
- Documented type mappings

## Testing and Validation

**Location**: `bindings/c/test/ffi_validation_test.c`

Comprehensive test suite validates:
- Basic FFI functionality
- Error handling
- Memory management
- Bulk operations
- Language compatibility

## Usage Examples

### Python Integration
```python
import ctypes

lib = ctypes.CDLL('./libkadedb_c.so')

class ValueHandle:
    def __init__(self, handle):
        self._handle = handle

    def __del__(self):
        if self._handle:
            lib.KadeDB_Value_Destroy(self._handle)

    @classmethod
    def integer(cls, value):
        handle = lib.KadeDB_Value_CreateInteger(value)
        return cls(handle)
```

### Rust Integration
```rust
use std::ffi::CString;

extern "C" {
    fn KadeDB_Value_CreateString(value: *const i8) -> *mut std::ffi::c_void;
    fn KadeDB_Value_Destroy(value: *mut std::ffi::c_void);
}

pub struct Value {
    handle: *mut std::ffi::c_void,
}

impl Value {
    pub fn new_string(value: &str) -> Option<Self> {
        let c_str = CString::new(value).ok()?;
        let handle = unsafe { KadeDB_Value_CreateString(c_str.as_ptr()) };
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

### Go Integration
```go
/*
#include "kadedb/kadedb_ffi_helpers.h"
*/
import "C"

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

## Summary

This FFI implementation successfully addresses all requirements:

✅ **C-compatible struct representations** - Complete type mapping system
✅ **Conversion functions** - Bidirectional C++/C conversion
✅ **Memory ownership rules** - Documented and enforced patterns
✅ **Example C headers** - Comprehensive interface examples
✅ **Error handling** - Structured error reporting across boundaries
✅ **Opaque pointer patterns** - Type-safe handle system
✅ **Comprehensive documentation** - Complete FFI guide

The implementation provides a production-ready FFI layer that enables safe, efficient integration of KadeDB with any language that supports C FFI, while maintaining memory safety, performance, and API stability.
