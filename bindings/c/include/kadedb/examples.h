/**
 * Example C Header for KadeDB FFI Integration
 * ===========================================
 *
 * This file demonstrates the complete KadeDB C API interface design,
 * showcasing FFI compatibility patterns, memory management, and error handling.
 *
 * Use this as a reference for creating language bindings or as documentation
 * for the C API design principles.
 */

#ifndef KADEDB_EXAMPLE_C_INTERFACE_H
#define KADEDB_EXAMPLE_C_INTERFACE_H

#include "kadedb/kadedb.h"
#include "kadedb/kadedb_ffi_helpers.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * =============================================================================
 * EXAMPLE: COMPLETE TABLE SCHEMA WORKFLOW
 * =============================================================================
 */

/**
 * Example function: Create a complete table schema with constraints
 *
 * This demonstrates:
 * - Opaque handle creation and management
 * - Error handling across FFI boundaries
 * - String parameter handling
 * - Resource cleanup patterns
 */
int example_create_user_table_schema(KDB_TableSchema **out_schema,
                                     KDB_ErrorInfo *error);

/**
 * Example function: Validate user data against schema
 *
 * This demonstrates:
 * - View-based data structures for bulk operations
 * - Error reporting with detailed messages
 * - Constraint validation across FFI
 */
int example_validate_user_data(const KDB_TableSchema *schema,
                               const KDB_RowView *users,
                               unsigned long long user_count,
                               KDB_ErrorInfo *error);

/**
 * =============================================================================
 * EXAMPLE: VALUE HANDLE MANIPULATION
 * =============================================================================
 */

/**
 * Example function: Create and manipulate individual values
 *
 * This demonstrates:
 * - Value handle lifecycle management
 * - Type-safe value creation and extraction
 * - Memory ownership transfer
 */
int example_value_operations(KDB_ErrorInfo *error);

/**
 * Example function: Create a row with mixed value types
 *
 * This demonstrates:
 * - Row creation with handle-based values
 * - Mixed data type handling
 * - Resource management with multiple handles
 */
KDB_Row *example_create_mixed_row(KDB_ErrorInfo *error);

/**
 * =============================================================================
 * EXAMPLE: DOCUMENT SCHEMA OPERATIONS
 * =============================================================================
 */

/**
 * Example function: Create and validate document schemas
 *
 * This demonstrates:
 * - Document schema creation and management
 * - Field constraints configuration
 * - Document validation patterns
 */
int example_document_workflow(KDB_ErrorInfo *error);

/**
 * =============================================================================
 * EXAMPLE: RESOURCE MANAGEMENT PATTERNS
 * =============================================================================
 */

/**
 * Example function: Demonstrate automatic resource cleanup
 *
 * This demonstrates:
 * - Resource manager usage
 * - Automatic cleanup on error conditions
 * - Exception-safe resource handling
 */
int example_automatic_cleanup(KDB_ErrorInfo *error);

/**
 * Example function: Manual resource management with safe patterns
 *
 * This demonstrates:
 * - Manual resource lifecycle management
 * - Safe destruction patterns
 * - Error handling with cleanup
 */
int example_manual_cleanup(KDB_ErrorInfo *error);

/**
 * =============================================================================
 * EXAMPLE: BULK DATA OPERATIONS
 * =============================================================================
 */

/**
 * Example function: Process large datasets efficiently
 *
 * This demonstrates:
 * - View-based APIs for performance
 * - Batch processing patterns
 * - Memory-efficient data handling
 */
int example_bulk_data_processing(const KDB_RowView *rows,
                                 unsigned long long row_count, char *csv_output,
                                 unsigned long long output_size,
                                 KDB_ErrorInfo *error);

/**
 * =============================================================================
 * EXAMPLE: ERROR HANDLING PATTERNS
 * =============================================================================
 */

/**
 * Example function: Comprehensive error handling demonstration
 *
 * This demonstrates:
 * - All error types and their handling
 * - Error propagation patterns
 * - Error recovery strategies
 */
int example_error_handling_patterns(KDB_ErrorInfo *error);

/**
 * =============================================================================
 * EXAMPLE: CROSS-LANGUAGE TYPE MAPPINGS
 * =============================================================================
 */

/**
 * Example structures showing C-compatible data layouts
 * for easy mapping to other languages
 */

typedef struct ExampleUserRecord {
  long long user_id; // Maps to i64 in Rust, int64 in Go, etc.
  char username[64]; // Fixed-size string for predictable layout
  double balance;    // Maps to f64 in Rust, float64 in Go, etc.
  int is_active;     // Maps to bool-like types (0/1)
  char email[128];   // Another fixed-size string
} ExampleUserRecord;

typedef struct ExampleResultSet {
  ExampleUserRecord *records;  // Array of records
  unsigned long long count;    // Number of records
  unsigned long long capacity; // Allocated capacity
} ExampleResultSet;

/**
 * Example function: Convert KadeDB rows to language-friendly structures
 *
 * This demonstrates:
 * - Data conversion between KadeDB and external formats
 * - Memory layout considerations for FFI
 * - Bulk data extraction patterns
 */
int example_convert_to_user_records(const KDB_RowView *rows,
                                    unsigned long long row_count,
                                    ExampleResultSet *out_result_set,
                                    KDB_ErrorInfo *error);

/**
 * Example function: Free the result set
 */
void example_free_user_records(ExampleResultSet *result_set);

/**
 * =============================================================================
 * EXAMPLE: THREAD SAFETY DEMONSTRATION
 * =============================================================================
 */

/**
 * Example function: Thread-safe schema operations
 *
 * This demonstrates:
 * - Thread-safe API usage patterns
 * - Proper synchronization guidelines
 * - Per-thread error handling
 */
int example_thread_safe_operations(const KDB_TableSchema *shared_schema,
                                   const KDB_RowView *thread_local_data,
                                   unsigned long long data_count, int thread_id,
                                   KDB_ErrorInfo *thread_local_error);

/**
 * =============================================================================
 * EXAMPLE: PERFORMANCE OPTIMIZATION PATTERNS
 * =============================================================================
 */

/**
 * Example function: Optimized bulk validation
 *
 * This demonstrates:
 * - Performance-oriented API usage
 * - Memory reuse patterns
 * - Batch processing for efficiency
 */
int example_optimized_bulk_validation(const KDB_TableSchema *schema,
                                      const KDB_RowView *rows,
                                      unsigned long long row_count,
                                      int *validation_results,
                                      KDB_ErrorInfo *error);

/**
 * =============================================================================
 * EXAMPLE: DEBUGGING AND DIAGNOSTICS
 * =============================================================================
 */

/**
 * Example function: Comprehensive diagnostics
 *
 * This demonstrates:
 * - Memory usage monitoring
 * - Leak detection
 * - Performance profiling hooks
 */
int example_diagnostics_and_debugging(KDB_ErrorInfo *error);

/**
 * =============================================================================
 * EXAMPLE: SERIALIZATION AND DATA EXCHANGE
 * =============================================================================
 */

/**
 * Example function: Data serialization workflow
 *
 * This demonstrates:
 * - CSV/JSON export functionality
 * - Data format conversion
 * - Buffer management for serialization
 */
int example_data_serialization(const KDB_RowView *rows,
                               unsigned long long row_count,
                               const char *const *column_names,
                               unsigned long long column_count, char **out_json,
                               char **out_csv, KDB_ErrorInfo *error);

/**
 * =============================================================================
 * EXAMPLE: LANGUAGE-SPECIFIC INTEGRATION HELPERS
 * =============================================================================
 */

/**
 * Helper functions designed specifically for Python ctypes integration
 */
typedef struct PythonCompatibleValue {
  int type; // Simple integer type ID
  union {
    long long i64_val;
    double f64_val;
    char *str_val; // Allocated string (caller must free)
    int bool_val;
  } data;
} PythonCompatibleValue;

int example_python_value_conversion(const KDB_ValueHandle *handle,
                                    PythonCompatibleValue *out_value,
                                    KDB_ErrorInfo *error);

void example_free_python_value(PythonCompatibleValue *value);

/**
 * Helper functions designed specifically for Rust integration
 */
typedef struct RustCompatibleSlice {
  const void *data;       // Pointer to data
  unsigned long long len; // Length in elements
  unsigned long long cap; // Capacity (for owned slices)
} RustCompatibleSlice;

int example_rust_slice_conversion(const KDB_RowView *rows,
                                  unsigned long long row_count,
                                  RustCompatibleSlice *out_slice,
                                  KDB_ErrorInfo *error);

/**
 * Helper functions designed specifically for Go cgo integration
 */
typedef struct GoCompatibleString {
  char *data; // C string data
  int len;    // Length (Go-style)
} GoCompatibleString;

int example_go_string_conversion(const KDB_ValueHandle *string_value,
                                 GoCompatibleString *out_string,
                                 KDB_ErrorInfo *error);

void example_free_go_string(GoCompatibleString *str);

#ifdef __cplusplus
}
#endif

#endif // KADEDB_EXAMPLE_C_INTERFACE_H
