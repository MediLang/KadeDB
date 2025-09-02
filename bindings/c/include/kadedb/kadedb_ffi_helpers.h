#ifndef KADEDB_FFI_HELPERS_H
#define KADEDB_FFI_HELPERS_H

#include "kadedb.h"

#include <stddef.h> // For size_t

#ifdef __cplusplus
extern "C" {
#endif

/**
 * KadeDB FFI Helper Functions and Utilities
 * =========================================
 *
 * This header provides helper functions, macros, and utilities to simplify
 * Foreign Function Interface (FFI) integration with KadeDB from various
 * programming languages including Python, Rust, Go, and others.
 *
 * Key Features:
 * - Enhanced error handling with structured error reporting
 * - Memory management helpers with automatic cleanup
 * - Opaque handle management utilities
 * - Type conversion helpers for cross-language compatibility
 * - Resource lifecycle management macros
 */

// ============================================================================
// ERROR HANDLING SYSTEM
// ============================================================================

/**
 * Enhanced error codes for detailed error reporting across FFI boundaries.
 * These codes provide programmatic error handling capabilities for foreign
 * language bindings.
 */
typedef enum KDB_ErrorCode {
  KDB_SUCCESS = 0,                    // Operation completed successfully
  KDB_ERROR_INVALID_ARGUMENT = 1,     // Invalid function argument provided
  KDB_ERROR_OUT_OF_RANGE = 2,         // Index or value out of valid range
  KDB_ERROR_DUPLICATE_NAME = 3,       // Duplicate column/field name
  KDB_ERROR_NOT_FOUND = 4,            // Requested item not found
  KDB_ERROR_VALIDATION_FAILED = 5,    // Schema validation failed
  KDB_ERROR_MEMORY_ALLOCATION = 6,    // Memory allocation failure
  KDB_ERROR_TYPE_MISMATCH = 7,        // Value type doesn't match expected type
  KDB_ERROR_CONSTRAINT_VIOLATION = 8, // Constraint validation failed
  KDB_ERROR_SCHEMA_CONFLICT = 9,      // Schema definition conflict
  KDB_ERROR_SERIALIZATION = 10,       // Serialization/deserialization error
  KDB_ERROR_IO = 11,                  // I/O operation error
  KDB_ERROR_UNKNOWN = 99              // Unknown or unexpected error
} KDB_ErrorCode;

/**
 * Comprehensive error information structure for FFI error reporting.
 * Provides both programmatic error codes and human-readable messages.
 */
typedef struct KDB_ErrorInfo {
  KDB_ErrorCode code; // Error code for programmatic handling
  char message[512];  // Human-readable error message (UTF-8)
  char context[256];  // Additional context information (function name, etc.)
  int line;           // Line number where error occurred (for debugging)
} KDB_ErrorInfo;

/**
 * Clear error information structure.
 * Call this before passing KDB_ErrorInfo to API functions.
 */
static inline void kadedb_clear_error(KDB_ErrorInfo *error) {
  if (error) {
    error->code = KDB_SUCCESS;
    error->message[0] = '\0';
    error->context[0] = '\0';
    error->line = 0;
  }
}

/**
 * Check if error information indicates an error state.
 * Returns non-zero if an error has occurred.
 */
static inline int kadedb_has_error(const KDB_ErrorInfo *error) {
  return error && error->code != KDB_SUCCESS;
}

/**
 * Get a human-readable string for the given error code.
 * Returns a static string that should not be freed.
 */
const char *kadedb_error_code_string(KDB_ErrorCode code);

/**
 * Set error information programmatically (for internal use).
 * This function is used internally by the API implementation.
 */
void kadedb_set_error(KDB_ErrorInfo *error, KDB_ErrorCode code,
                      const char *message, const char *context, int line);

// Convenience macro for setting error with current function and line
#define KADEDB_SET_ERROR(error, code, msg)                                     \
  kadedb_set_error((error), (code), (msg), __func__, __LINE__)

// ============================================================================
// MEMORY MANAGEMENT HELPERS
// ============================================================================

/**
 * Safe destruction macro for opaque handles.
 * Checks for null pointer and sets to null after destruction.
 */
#define KADEDB_SAFE_DESTROY(handle_type, handle_var)                           \
  do {                                                                         \
    if ((handle_var) != NULL) {                                                \
      KadeDB_##handle_type##_Destroy(handle_var);                              \
      (handle_var) = NULL;                                                     \
    }                                                                          \
  } while (0)

/**
 * Resource management helper for automatic cleanup.
 * Use this structure to ensure resources are cleaned up even if
 * exceptions occur in the calling language.
 */
typedef struct KDB_ResourceManager {
  void **resources;             // Array of resource pointers
  void (**destructors)(void *); // Array of destructor function pointers
  size_t count;                 // Number of managed resources
  size_t capacity;              // Capacity of arrays
} KDB_ResourceManager;

/**
 * Initialize a resource manager for automatic cleanup.
 */
int kadedb_resource_manager_init(KDB_ResourceManager *manager,
                                 size_t initial_capacity);

/**
 * Add a resource to be managed (automatically cleaned up).
 */
int kadedb_resource_manager_add(KDB_ResourceManager *manager, void *resource,
                                void (*destructor)(void *));

/**
 * Clean up all managed resources and free the resource manager.
 */
void kadedb_resource_manager_cleanup(KDB_ResourceManager *manager);

// ============================================================================
// OPAQUE HANDLE UTILITIES
// ============================================================================

/**
 * Opaque handles for core data types to provide memory-safe FFI boundaries.
 * These handles encapsulate C++ objects while providing C-compatible
 * interfaces.
 */

/**
 * Opaque handle to a Value object (C++ kadedb::Value).
 * Use this for passing individual values across FFI boundaries.
 */
typedef struct KDB_ValueHandle KDB_ValueHandle;

/**
 * Opaque handle to a Row object (C++ kadedb::Row).
 * Provides deep-copy semantics for row data.
 */
typedef struct KDB_Row KDB_Row;

/**
 * Opaque handle to a RowShallow object (C++ kadedb::RowShallow).
 * Provides shallow-copy semantics for efficient row sharing.
 */
typedef struct KDB_RowShallow KDB_RowShallow;

// Value handle management functions
KDB_ValueHandle *KadeDB_Value_CreateNull();
KDB_ValueHandle *KadeDB_Value_CreateInteger(long long value);
KDB_ValueHandle *KadeDB_Value_CreateFloat(double value);
KDB_ValueHandle *KadeDB_Value_CreateString(const char *value);
KDB_ValueHandle *KadeDB_Value_CreateBoolean(int value);
void KadeDB_Value_Destroy(KDB_ValueHandle *value);

// Value handle query functions
KDB_ValueType KadeDB_Value_GetType(const KDB_ValueHandle *value);
int KadeDB_Value_IsNull(const KDB_ValueHandle *value);
long long KadeDB_Value_AsInteger(const KDB_ValueHandle *value,
                                 KDB_ErrorInfo *error);
double KadeDB_Value_AsFloat(const KDB_ValueHandle *value, KDB_ErrorInfo *error);
const char *KadeDB_Value_AsString(const KDB_ValueHandle *value,
                                  KDB_ErrorInfo *error);
int KadeDB_Value_AsBoolean(const KDB_ValueHandle *value, KDB_ErrorInfo *error);

// Value handle utility functions
KDB_ValueHandle *KadeDB_Value_Clone(const KDB_ValueHandle *value);
int KadeDB_Value_Equals(const KDB_ValueHandle *a, const KDB_ValueHandle *b);
int KadeDB_Value_Compare(const KDB_ValueHandle *a, const KDB_ValueHandle *b);
char *KadeDB_Value_ToString(
    const KDB_ValueHandle *value); // Caller must free with KadeDB_String_Free

// Row handle management functions
KDB_Row *KadeDB_Row_Create(unsigned long long column_count);
void KadeDB_Row_Destroy(KDB_Row *row);
KDB_Row *KadeDB_Row_Clone(const KDB_Row *row);

// Row handle manipulation functions
unsigned long long KadeDB_Row_Size(const KDB_Row *row);
int KadeDB_Row_Set(KDB_Row *row, unsigned long long index,
                   const KDB_ValueHandle *value, KDB_ErrorInfo *error);
KDB_ValueHandle *KadeDB_Row_Get(const KDB_Row *row, unsigned long long index,
                                KDB_ErrorInfo *error);

// RowShallow handle management functions
KDB_RowShallow *KadeDB_RowShallow_Create(unsigned long long column_count);
void KadeDB_RowShallow_Destroy(KDB_RowShallow *row);
KDB_RowShallow *KadeDB_RowShallow_FromRow(const KDB_Row *row);
KDB_Row *KadeDB_RowShallow_ToRow(const KDB_RowShallow *row);

// RowShallow handle manipulation functions
unsigned long long KadeDB_RowShallow_Size(const KDB_RowShallow *row);
int KadeDB_RowShallow_Set(KDB_RowShallow *row, unsigned long long index,
                          const KDB_ValueHandle *value, KDB_ErrorInfo *error);
KDB_ValueHandle *KadeDB_RowShallow_Get(const KDB_RowShallow *row,
                                       unsigned long long index,
                                       KDB_ErrorInfo *error);

// ============================================================================
// STRING MEMORY MANAGEMENT
// ============================================================================

/**
 * Free strings allocated by KadeDB functions.
 * Use this to free strings returned by functions like KadeDB_Value_ToString.
 */
void KadeDB_String_Free(char *str);

/**
 * Duplicate a string using KadeDB's memory allocator.
 * The returned string must be freed with KadeDB_String_Free.
 */
char *KadeDB_String_Duplicate(const char *str);

// ============================================================================
// CONVENIENCE HELPERS FOR COMMON OPERATIONS
// ============================================================================

/**
 * Create a row with values from C arrays.
 * This is a convenience function to simplify row creation from foreign
 * languages.
 */
KDB_Row *kadedb_create_row_with_values(const KDB_Value *values,
                                       unsigned long long count,
                                       KDB_ErrorInfo *error);

/**
 * Convert a row to an array of KDB_Value structures.
 * The returned array must be freed with kadedb_free_value_array.
 */
KDB_Value *kadedb_row_to_value_array(const KDB_Row *row,
                                     unsigned long long *out_count,
                                     KDB_ErrorInfo *error);

/**
 * Free an array of KDB_Value structures created by kadedb_row_to_value_array.
 */
void kadedb_free_value_array(KDB_Value *values, unsigned long long count);

/**
 * Create a document from key-value pairs.
 */
int kadedb_create_document(const char **keys, const KDB_Value *values,
                           unsigned long long count, KDB_KeyValue **out_doc,
                           KDB_ErrorInfo *error);

/**
 * Free a document created by kadedb_create_document.
 */
void kadedb_free_document(KDB_KeyValue *doc, unsigned long long count);

// ============================================================================
// TYPE CONVERSION UTILITIES
// ============================================================================

/**
 * Convert C KDB_Value to a ValueHandle for opaque handle APIs.
 */
KDB_ValueHandle *kadedb_value_to_handle(const KDB_Value *c_value,
                                        KDB_ErrorInfo *error);

/**
 * Convert ValueHandle to C KDB_Value for view-based APIs.
 * Note: For string values, the returned pointer is only valid while the handle
 * exists.
 */
int kadedb_handle_to_value(const KDB_ValueHandle *handle, KDB_Value *out_value,
                           KDB_ErrorInfo *error);

// ============================================================================
// DEBUGGING AND DIAGNOSTICS
// ============================================================================

/**
 * Get detailed information about memory usage.
 * Useful for debugging memory leaks in FFI scenarios.
 */
typedef struct KDB_MemoryInfo {
  unsigned long long total_allocated;
  unsigned long long total_freed;
  unsigned long long current_usage;
  unsigned long long peak_usage;
} KDB_MemoryInfo;

/**
 * Retrieve current memory usage statistics (if KADEDB_MEM_DEBUG is enabled).
 */
int kadedb_get_memory_info(KDB_MemoryInfo *info);

/**
 * Print memory statistics to stderr (if KADEDB_MEM_DEBUG is enabled).
 */
void kadedb_print_memory_stats();

/**
 * Validate that all handles and resources have been properly cleaned up.
 * Returns non-zero if leaks are detected.
 */
int kadedb_check_resource_leaks();

// ============================================================================
// THREAD SAFETY NOTES
// ============================================================================

/**
 * THREAD SAFETY GUIDELINES:
 *
 * 1. Individual handles (KDB_Row*, KDB_ValueHandle*, etc.) are NOT thread-safe.
 *    Use external synchronization when accessing the same handle from multiple
 * threads.
 *
 * 2. Different handles can be used concurrently from different threads without
 *    synchronization (unless they share underlying data, as in RowShallow).
 *
 * 3. Schema objects (KDB_TableSchema*, KDB_DocumentSchema*) are thread-safe
 *    for read operations after construction, but modification requires external
 *    synchronization.
 *
 * 4. The error handling system is thread-safe - each thread should use its own
 *    KDB_ErrorInfo structure.
 *
 * 5. Memory management functions (KadeDB_*_Destroy, KadeDB_String_Free) are
 *    thread-safe but should only be called once per resource.
 */

#ifdef __cplusplus
}
#endif

#endif // KADEDB_FFI_HELPERS_H
