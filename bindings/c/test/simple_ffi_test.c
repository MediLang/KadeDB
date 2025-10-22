#include "kadedb/kadedb.h"
#include "kadedb/kadedb_ffi_helpers.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void test_error_handling() {
  printf("=== Testing Error Handling ===\n");

  KDB_ErrorInfo error;
  kadedb_clear_error(&error);

  // Verify error is cleared
  assert(!kadedb_has_error(&error));
  assert(error.code == KDB_SUCCESS);

  // Test error code string conversion
  const char *error_str = kadedb_error_code_string(KDB_ERROR_INVALID_ARGUMENT);
  printf("Error code 1: %s\n", error_str);
  assert(strcmp(error_str, "Invalid argument") == 0);

  printf("✓ Error handling tests passed\n\n");
}

void test_version() {
  printf("=== Testing Version ===\n");

  const char *version = KadeDB_GetVersion();
  printf("KadeDB Version: %s\n", version);
  assert(version != NULL);
  assert(strlen(version) > 0);

  int major = 0, minor = 0, patch = 0;
  int parsed = sscanf(version, "%d.%d.%d", &major, &minor, &patch);
  assert(parsed == 3);
  assert(major == KadeDB_GetMajorVersion());
  assert(minor == KadeDB_GetMinorVersion());
  assert(patch == KadeDB_GetPatchVersion());

  printf("✓ Version test passed\n\n");
}

void test_storage_query() {
  printf("=== Testing Storage + Query ABI ===\n");

  int init_result = KadeDB_Initialize();
  assert(init_result == 1);

  KadeDB_Storage *storage = KadeDB_CreateStorage();
  assert(storage != NULL);

  KDB_TableSchema *schema = KadeDB_TableSchema_Create();
  assert(schema != NULL);

  KDB_TableColumnEx column = {
      "name", KDB_COL_STRING,
      0,   // not nullable
      0,   // not unique
      NULL // no extra constraints
  };

  int add_status = KadeDB_TableSchema_AddColumn(schema, &column);
  assert(add_status == 1);

  int create_status = KadeDB_CreateTable(storage, "users", schema);
  assert(create_status == 1);

  KDB_Value row_values[1];
  row_values[0].type = KDB_VAL_STRING;
  row_values[0].as.str = "alice";

  KDB_RowView row = {row_values, 1};
  int insert_status = KadeDB_InsertRow(storage, "users", &row);
  assert(insert_status == 1);

  KadeDB_ResultSet *rs = KadeDB_ExecuteQuery(storage, "SELECT * FROM users");
  assert(rs != NULL);

  int has_row = KadeDB_ResultSet_NextRow(rs);
  assert(has_row == 1);
  const char *name = KadeDB_ResultSet_GetString(rs, 0);
  assert(name != NULL);
  assert(strcmp(name, "alice") == 0);
  assert(KadeDB_ResultSet_NextRow(rs) == 0);

  KadeDB_DestroyResultSet(rs);
  KadeDB_TableSchema_Destroy(schema);
  KadeDB_DestroyStorage(storage);
  KadeDB_Shutdown();

  printf("✓ Storage + Query tests passed\n\n");
}

void test_value_handles() {
  printf("=== Testing Value Handles ===\n");

  KDB_ErrorInfo error;
  kadedb_clear_error(&error);

  // Test integer value
  KDB_ValueHandle *int_val = KadeDB_Value_CreateInteger(42);
  assert(int_val != NULL);

  KDB_ValueType type = KadeDB_Value_GetType(int_val);
  assert(type == KDB_VAL_INTEGER);

  long long value = KadeDB_Value_AsInteger(int_val, &error);
  assert(!kadedb_has_error(&error));
  assert(value == 42);

  // Test string value
  KDB_ValueHandle *str_val = KadeDB_Value_CreateString("Hello, FFI!");
  assert(str_val != NULL);

  type = KadeDB_Value_GetType(str_val);
  assert(type == KDB_VAL_STRING);

  const char *str = KadeDB_Value_AsString(str_val, &error);
  assert(!kadedb_has_error(&error));
  assert(strcmp(str, "Hello, FFI!") == 0);

  // Test value cloning
  KDB_ValueHandle *cloned = KadeDB_Value_Clone(int_val);
  assert(cloned != NULL);
  assert(KadeDB_Value_Equals(int_val, cloned));

  // Test string representation
  char *str_repr = KadeDB_Value_ToString(int_val);
  assert(str_repr != NULL);
  assert(strcmp(str_repr, "42") == 0);

  // Cleanup
  KadeDB_String_Free(str_repr);
  KadeDB_Value_Destroy(int_val);
  KadeDB_Value_Destroy(str_val);
  KadeDB_Value_Destroy(cloned);

  printf("✓ Value handle tests passed\n\n");
}

void test_table_schema() {
  printf("=== Testing Table Schema ===\n");

  KDB_ErrorInfo error;
  kadedb_clear_error(&error);

  // Create schema
  KDB_TableSchema *schema = KadeDB_TableSchema_Create();
  assert(schema != NULL);

  // Add a simple column
  KDB_TableColumnEx column = {
      "test_col", KDB_COL_INTEGER,
      0,   // not nullable
      1,   // unique
      NULL // no constraints
  };

  int result = KadeDB_TableSchema_AddColumn(schema, &column);
  assert(result == 1);

  // Test validation with a simple row
  KDB_Value values[] = {{KDB_VAL_INTEGER, .as.i64 = 123}};
  KDB_RowView row = {values, 1};

  char err_buf[256];
  result =
      KadeDB_TableSchema_ValidateRow(schema, &row, err_buf, sizeof(err_buf));
  assert(result == 1);

  // Cleanup
  KadeDB_TableSchema_Destroy(schema);

  printf("✓ Table schema tests passed\n\n");
}

void test_memory_management() {
  printf("=== Testing Memory Management ===\n");

  // Test string duplication
  const char *original = "Test string";
  char *duplicate = KadeDB_String_Duplicate(original);
  assert(duplicate != NULL);
  assert(strcmp(duplicate, original) == 0);

  KadeDB_String_Free(duplicate);

  // Test safe destruction pattern
  KDB_TableSchema *schema = KadeDB_TableSchema_Create();
  assert(schema != NULL);

  KADEDB_SAFE_DESTROY(TableSchema, schema);
  assert(schema == NULL);

  printf("✓ Memory management tests passed\n\n");
}

int main() {
  printf("KadeDB FFI Basic Validation Test\n");
  printf("================================\n\n");

  test_error_handling();
  test_version();
  test_value_handles();
  test_table_schema();
  test_storage_query();
  test_memory_management();

  printf("🎉 ALL BASIC FFI TESTS PASSED! 🎉\n");
  printf("Core FFI functionality is working correctly.\n");

  return 0;
}
