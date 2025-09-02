#include "kadedb/examples.h"
#include "kadedb/kadedb.h"
#include "kadedb/kadedb_ffi_helpers.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void test_basic_ffi_functionality() {
  printf("=== Testing Basic FFI Functionality ===\n");

  KDB_ErrorInfo error;
  kadedb_clear_error(&error);

  // Test version retrieval
  const char *version = KadeDB_GetVersion();
  printf("KadeDB Version: %s\n", version);
  assert(version != NULL);

  // Test value handle creation and manipulation
  printf("Testing value handle operations...\n");
  int result = example_value_operations(&error);
  if (!result) {
    printf("ERROR: Value operations failed: %s\n", error.message);
    assert(0);
  }
  printf("✓ Value operations successful\n");

  // Test table schema creation
  printf("Testing table schema creation...\n");
  KDB_TableSchema *schema = NULL;
  result = example_create_user_table_schema(&schema, &error);
  if (!result) {
    printf("ERROR: Schema creation failed: %s\n", error.message);
    assert(0);
  }
  assert(schema != NULL);
  printf("✓ Table schema created successfully\n");

  // Test row creation with mixed types
  printf("Testing row creation with mixed types...\n");
  kadedb_clear_error(&error);
  KDB_Row *row = example_create_mixed_row(&error);
  if (!row) {
    printf("ERROR: Row creation failed: %s\n", error.message);
    KadeDB_TableSchema_Destroy(schema);
    assert(0);
  }
  printf("✓ Mixed row created successfully\n");

  // Test row validation using view-based API
  printf("Testing row validation...\n");
  KDB_Value row_values[] = {{KDB_VAL_INTEGER, .as.i64 = 1001},
                            {KDB_VAL_STRING, .as.str = "john_doe"},
                            {KDB_VAL_STRING, .as.str = "john@example.com"},
                            {KDB_VAL_FLOAT, .as.f64 = 1234.56},
                            {KDB_VAL_BOOLEAN, .as.boolean = 1}};
  KDB_RowView row_view = {row_values, 5};

  kadedb_clear_error(&error);
  result = example_validate_user_data(schema, &row_view, 1, &error);
  if (!result) {
    printf("ERROR: Row validation failed: %s\n", error.message);
    KadeDB_Row_Destroy(row);
    KadeDB_TableSchema_Destroy(schema);
    assert(0);
  }
  printf("✓ Row validation successful\n");

  // Test resource management
  printf("Testing automatic resource management...\n");
  kadedb_clear_error(&error);
  result = example_automatic_cleanup(&error);
  if (!result) {
    printf("ERROR: Automatic cleanup test failed: %s\n", error.message);
    KadeDB_Row_Destroy(row);
    KadeDB_TableSchema_Destroy(schema);
    assert(0);
  }
  printf("✓ Automatic resource management successful\n");

  // Clean up
  KadeDB_Row_Destroy(row);
  KadeDB_TableSchema_Destroy(schema);

  printf("=== Basic FFI Functionality Tests PASSED ===\n\n");
}

void test_error_handling() {
  printf("=== Testing Error Handling ===\n");

  KDB_ErrorInfo error;
  kadedb_clear_error(&error);

  // Test error code string conversion
  printf("Testing error code strings...\n");
  const char *error_str = kadedb_error_code_string(KDB_ERROR_INVALID_ARGUMENT);
  printf("Error code 1: %s\n", error_str);
  assert(strcmp(error_str, "Invalid argument") == 0);

  // Test error handling with invalid operations
  printf("Testing error detection...\n");
  KDB_ValueHandle *null_handle = NULL;
  long long result = KadeDB_Value_AsInteger(null_handle, &error);

  if (!kadedb_has_error(&error)) {
    printf("ERROR: Expected error not detected!\n");
    assert(0);
  }

  printf("✓ Error detected: %s (code %d)\n", error.message, error.code);
  assert(error.code == KDB_ERROR_INVALID_ARGUMENT);

  // Test error clearing
  kadedb_clear_error(&error);
  assert(!kadedb_has_error(&error));
  printf("✓ Error cleared successfully\n");

  printf("=== Error Handling Tests PASSED ===\n\n");
}

void test_memory_management() {
  printf("=== Testing Memory Management ===\n");

  KDB_ErrorInfo error;
  kadedb_clear_error(&error);

  // Test string duplication and freeing
  printf("Testing string memory management...\n");
  const char *original = "Hello, KadeDB FFI!";
  char *duplicated = KadeDB_String_Duplicate(original);
  assert(duplicated != NULL);
  assert(strcmp(duplicated, original) == 0);

  KadeDB_String_Free(duplicated);
  printf("✓ String duplication and freeing successful\n");

  // Test value handle lifecycle
  printf("Testing value handle lifecycle...\n");
  KDB_ValueHandle *value = KadeDB_Value_CreateString("test string");
  assert(value != NULL);

  char *str_repr = KadeDB_Value_ToString(value);
  assert(str_repr != NULL);
  printf("Value string representation: %s\n", str_repr);

  KadeDB_String_Free(str_repr);
  KadeDB_Value_Destroy(value);
  printf("✓ Value handle lifecycle successful\n");

  // Test safe destruction macros
  printf("Testing safe destruction patterns...\n");
  KDB_TableSchema *schema = KadeDB_TableSchema_Create();
  assert(schema != NULL);

  KADEDB_SAFE_DESTROY(TableSchema, schema);
  assert(schema == NULL); // Should be set to NULL by the macro
  printf("✓ Safe destruction pattern successful\n");

  printf("=== Memory Management Tests PASSED ===\n\n");
}

void test_bulk_operations() {
  printf("=== Testing Bulk Operations ===\n");

  KDB_ErrorInfo error;
  kadedb_clear_error(&error);

  // Create test data
  printf("Creating test dataset...\n");
  KDB_Value user_data[][5] = {
      {{KDB_VAL_INTEGER, .as.i64 = 1001},
       {KDB_VAL_STRING, .as.str = "alice"},
       {KDB_VAL_STRING, .as.str = "alice@example.com"},
       {KDB_VAL_FLOAT, .as.f64 = 1000.0},
       {KDB_VAL_BOOLEAN, .as.boolean = 1}},
      {{KDB_VAL_INTEGER, .as.i64 = 1002},
       {KDB_VAL_STRING, .as.str = "bob"},
       {KDB_VAL_STRING, .as.str = "bob@example.com"},
       {KDB_VAL_FLOAT, .as.f64 = 2500.50},
       {KDB_VAL_BOOLEAN, .as.boolean = 1}},
      {{KDB_VAL_INTEGER, .as.i64 = 1003},
       {KDB_VAL_STRING, .as.str = "charlie"},
       {KDB_VAL_STRING, .as.str = "charlie@example.com"},
       {KDB_VAL_FLOAT, .as.f64 = 750.25},
       {KDB_VAL_BOOLEAN, .as.boolean = 0}}};

  KDB_RowView rows[] = {
      {user_data[0], 5}, {user_data[1], 5}, {user_data[2], 5}};

  // Test CSV conversion
  printf("Testing CSV conversion...\n");
  char csv_buffer[2048];
  int result = example_bulk_data_processing(rows, 3, csv_buffer,
                                            sizeof(csv_buffer), &error);

  if (!result) {
    printf("ERROR: Bulk data processing failed: %s\n", error.message);
    assert(0);
  }

  printf("Generated CSV:\n%s\n", csv_buffer);
  printf("✓ CSV conversion successful\n");

  printf("=== Bulk Operations Tests PASSED ===\n\n");
}

void test_python_compatibility() {
  printf("=== Testing Python Compatibility ===\n");

  KDB_ErrorInfo error;
  kadedb_clear_error(&error);

  // Create a value and convert to Python-compatible format
  printf("Testing Python value conversion...\n");
  KDB_ValueHandle *str_value = KadeDB_Value_CreateString("Python FFI Test");
  assert(str_value != NULL);

  PythonCompatibleValue py_value;
  int result = example_python_value_conversion(str_value, &py_value, &error);

  if (!result) {
    printf("ERROR: Python conversion failed: %s\n", error.message);
    KadeDB_Value_Destroy(str_value);
    assert(0);
  }

  assert(py_value.type == KDB_VAL_STRING);
  assert(py_value.data.str_val != NULL);
  assert(strcmp(py_value.data.str_val, "Python FFI Test") == 0);

  printf("✓ Python value conversion successful: %s\n", py_value.data.str_val);

  // Clean up
  example_free_python_value(&py_value);
  KadeDB_Value_Destroy(str_value);

  printf("=== Python Compatibility Tests PASSED ===\n\n");
}

int main() {
  printf("KadeDB C API / FFI Validation Tests\n");
  printf("====================================\n\n");

  // Run all test suites
  test_basic_ffi_functionality();
  test_error_handling();
  test_memory_management();
  test_bulk_operations();
  test_python_compatibility();

  printf("🎉 ALL TESTS PASSED! 🎉\n");
  printf("KadeDB FFI implementation is working correctly.\n");

  return 0;
}
