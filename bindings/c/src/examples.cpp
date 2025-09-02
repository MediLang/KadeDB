#include "kadedb/examples.h"
#include "kadedb/kadedb.h"
#include "kadedb/kadedb_ffi_helpers.h"

#include <cmath> // For NAN
#include <cstdio>
#include <cstdlib>
#include <cstring>

extern "C" {

// =============================================================================
// EXAMPLE: COMPLETE TABLE SCHEMA WORKFLOW
// =============================================================================

int example_create_user_table_schema(KDB_TableSchema **out_schema,
                                     KDB_ErrorInfo *error) {
  kadedb_clear_error(error);

  if (!out_schema) {
    KADEDB_SET_ERROR(error, KDB_ERROR_INVALID_ARGUMENT,
                     "Output schema pointer is null");
    return 0;
  }

  // Create schema
  KDB_TableSchema *schema = KadeDB_TableSchema_Create();
  if (!schema) {
    KADEDB_SET_ERROR(error, KDB_ERROR_MEMORY_ALLOCATION,
                     "Failed to create table schema");
    return 0;
  }

  // Add user_id column (integer, not nullable, unique)
  KDB_TableColumnEx user_id_col = {
      "user_id", KDB_COL_INTEGER,
      0,      // not nullable
      1,      // unique
      nullptr // no additional constraints
  };

  if (!KadeDB_TableSchema_AddColumn(schema, &user_id_col)) {
    KADEDB_SET_ERROR(error, KDB_ERROR_DUPLICATE_NAME,
                     "Failed to add user_id column");
    KadeDB_TableSchema_Destroy(schema);
    return 0;
  }

  // Add username column (string, not nullable, unique, with length constraints)
  KDB_ColumnConstraints username_constraints = {
      3,       // min_len
      50,      // max_len
      nullptr, // one_of
      0,       // one_of_count
      NAN,     // min_value (not applicable for strings)
      NAN      // max_value (not applicable for strings)
  };

  KDB_TableColumnEx username_col = {"username", KDB_COL_STRING,
                                    0, // not nullable
                                    1, // unique
                                    &username_constraints};

  if (!KadeDB_TableSchema_AddColumn(schema, &username_col)) {
    KADEDB_SET_ERROR(error, KDB_ERROR_DUPLICATE_NAME,
                     "Failed to add username column");
    KadeDB_TableSchema_Destroy(schema);
    return 0;
  }

  // Add email column (string, nullable, unique)
  KDB_TableColumnEx email_col = {"email", KDB_COL_STRING,
                                 1, // nullable
                                 1, // unique
                                 nullptr};

  if (!KadeDB_TableSchema_AddColumn(schema, &email_col)) {
    KADEDB_SET_ERROR(error, KDB_ERROR_DUPLICATE_NAME,
                     "Failed to add email column");
    KadeDB_TableSchema_Destroy(schema);
    return 0;
  }

  // Add balance column (float, nullable, with numeric constraints)
  KDB_ColumnConstraints balance_constraints = {
      -1,       // min_len (not applicable)
      -1,       // max_len (not applicable)
      nullptr,  // one_of
      0,        // one_of_count
      0.0,      // min_value
      1000000.0 // max_value
  };

  KDB_TableColumnEx balance_col = {"balance", KDB_COL_FLOAT,
                                   1, // nullable
                                   0, // not unique
                                   &balance_constraints};

  if (!KadeDB_TableSchema_AddColumn(schema, &balance_col)) {
    KADEDB_SET_ERROR(error, KDB_ERROR_DUPLICATE_NAME,
                     "Failed to add balance column");
    KadeDB_TableSchema_Destroy(schema);
    return 0;
  }

  // Add is_active column (boolean, not nullable)
  KDB_TableColumnEx is_active_col = {"is_active", KDB_COL_BOOLEAN,
                                     0, // not nullable
                                     0, // not unique
                                     nullptr};

  if (!KadeDB_TableSchema_AddColumn(schema, &is_active_col)) {
    KADEDB_SET_ERROR(error, KDB_ERROR_DUPLICATE_NAME,
                     "Failed to add is_active column");
    KadeDB_TableSchema_Destroy(schema);
    return 0;
  }

  // Set primary key
  if (!KadeDB_TableSchema_SetPrimaryKey(schema, "user_id")) {
    KADEDB_SET_ERROR(error, KDB_ERROR_NOT_FOUND, "Failed to set primary key");
    KadeDB_TableSchema_Destroy(schema);
    return 0;
  }

  *out_schema = schema;
  return 1;
}

int example_validate_user_data(const KDB_TableSchema *schema,
                               const KDB_RowView *users,
                               unsigned long long user_count,
                               KDB_ErrorInfo *error) {
  kadedb_clear_error(error);

  if (!schema) {
    KADEDB_SET_ERROR(error, KDB_ERROR_INVALID_ARGUMENT, "Schema is null");
    return 0;
  }

  if (!users && user_count > 0) {
    KADEDB_SET_ERROR(error, KDB_ERROR_INVALID_ARGUMENT,
                     "Users array is null but count > 0");
    return 0;
  }

  // Validate each user row individually
  char validation_error[512];
  for (unsigned long long i = 0; i < user_count; ++i) {
    if (!KadeDB_TableSchema_ValidateRow(schema, &users[i], validation_error,
                                        sizeof(validation_error))) {
      char full_error[768];
      snprintf(full_error, sizeof(full_error), "Row %llu validation failed: %s",
               i, validation_error);
      KADEDB_SET_ERROR(error, KDB_ERROR_VALIDATION_FAILED, full_error);
      return 0;
    }
  }

  // Validate uniqueness constraints across all rows
  if (!KadeDB_TableSchema_ValidateUniqueRows(schema, users, user_count, 1,
                                             validation_error,
                                             sizeof(validation_error))) {
    char full_error[768];
    snprintf(full_error, sizeof(full_error), "Uniqueness validation failed: %s",
             validation_error);
    KADEDB_SET_ERROR(error, KDB_ERROR_CONSTRAINT_VIOLATION, full_error);
    return 0;
  }

  return 1;
}

// =============================================================================
// EXAMPLE: VALUE HANDLE MANIPULATION
// =============================================================================

int example_value_operations(KDB_ErrorInfo *error) {
  kadedb_clear_error(error);

  // Create various value types
  KDB_ValueHandle *null_val = KadeDB_Value_CreateNull();
  KDB_ValueHandle *int_val = KadeDB_Value_CreateInteger(42);
  KDB_ValueHandle *float_val = KadeDB_Value_CreateFloat(3.14159);
  KDB_ValueHandle *string_val = KadeDB_Value_CreateString("Hello, KadeDB!");
  KDB_ValueHandle *bool_val = KadeDB_Value_CreateBoolean(1);

  if (!null_val || !int_val || !float_val || !string_val || !bool_val) {
    KADEDB_SET_ERROR(error, KDB_ERROR_MEMORY_ALLOCATION,
                     "Failed to create values");
    goto cleanup;
  }

  // Test value type queries
  if (KadeDB_Value_GetType(null_val) != KDB_VAL_NULL) {
    KADEDB_SET_ERROR(error, KDB_ERROR_TYPE_MISMATCH,
                     "Null value type mismatch");
    goto cleanup;
  }

  if (KadeDB_Value_GetType(int_val) != KDB_VAL_INTEGER) {
    KADEDB_SET_ERROR(error, KDB_ERROR_TYPE_MISMATCH,
                     "Integer value type mismatch");
    goto cleanup;
  }

  // Test value extraction - declare variables at the top
  KDB_ErrorInfo extraction_error;
  long long extracted_int;
  double extracted_float;
  const char *extracted_string;
  KDB_ValueHandle *cloned_int;
  char *int_string;

  kadedb_clear_error(&extraction_error);

  extracted_int = KadeDB_Value_AsInteger(int_val, &extraction_error);
  if (kadedb_has_error(&extraction_error)) {
    KADEDB_SET_ERROR(error, KDB_ERROR_TYPE_MISMATCH,
                     "Failed to extract integer value");
    goto cleanup;
  }

  if (extracted_int != 42) {
    KADEDB_SET_ERROR(error, KDB_ERROR_VALIDATION_FAILED,
                     "Integer value mismatch");
    goto cleanup;
  }

  extracted_float = KadeDB_Value_AsFloat(float_val, &extraction_error);
  if (kadedb_has_error(&extraction_error)) {
    KADEDB_SET_ERROR(error, KDB_ERROR_TYPE_MISMATCH,
                     "Failed to extract float value");
    goto cleanup;
  }

  extracted_string = KadeDB_Value_AsString(string_val, &extraction_error);
  if (kadedb_has_error(&extraction_error)) {
    KADEDB_SET_ERROR(error, KDB_ERROR_TYPE_MISMATCH,
                     "Failed to extract string value");
    goto cleanup;
  }

  if (strcmp(extracted_string, "Hello, KadeDB!") != 0) {
    KADEDB_SET_ERROR(error, KDB_ERROR_VALIDATION_FAILED,
                     "String value mismatch");
    goto cleanup;
  }

  // Test value cloning
  cloned_int = KadeDB_Value_Clone(int_val);
  if (!cloned_int) {
    KADEDB_SET_ERROR(error, KDB_ERROR_MEMORY_ALLOCATION,
                     "Failed to clone integer value");
    goto cleanup;
  }

  // Test value equality
  if (!KadeDB_Value_Equals(int_val, cloned_int)) {
    KADEDB_SET_ERROR(error, KDB_ERROR_VALIDATION_FAILED,
                     "Cloned values are not equal");
    KadeDB_Value_Destroy(cloned_int);
    goto cleanup;
  }

  // Test string representation
  int_string = KadeDB_Value_ToString(int_val);
  if (!int_string) {
    KADEDB_SET_ERROR(error, KDB_ERROR_MEMORY_ALLOCATION,
                     "Failed to convert value to string");
    KadeDB_Value_Destroy(cloned_int);
    goto cleanup;
  }

  if (strcmp(int_string, "42") != 0) {
    KADEDB_SET_ERROR(error, KDB_ERROR_VALIDATION_FAILED,
                     "String representation mismatch");
    KadeDB_String_Free(int_string);
    KadeDB_Value_Destroy(cloned_int);
    goto cleanup;
  }

  KadeDB_String_Free(int_string);
  KadeDB_Value_Destroy(cloned_int);

cleanup:
  // Clean up all values
  KadeDB_Value_Destroy(null_val);
  KadeDB_Value_Destroy(int_val);
  KadeDB_Value_Destroy(float_val);
  KadeDB_Value_Destroy(string_val);
  KadeDB_Value_Destroy(bool_val);

  return kadedb_has_error(error) ? 0 : 1;
}

KDB_Row *example_create_mixed_row(KDB_ErrorInfo *error) {
  kadedb_clear_error(error);

  // Create a row with 5 columns matching our user table schema
  KDB_Row *row = KadeDB_Row_Create(5);
  if (!row) {
    KADEDB_SET_ERROR(error, KDB_ERROR_MEMORY_ALLOCATION,
                     "Failed to create row");
    return nullptr;
  }

  // Create values for each column
  KDB_ValueHandle *user_id = KadeDB_Value_CreateInteger(1001);
  KDB_ValueHandle *username = KadeDB_Value_CreateString("john_doe");
  KDB_ValueHandle *email = KadeDB_Value_CreateString("john@example.com");
  KDB_ValueHandle *balance = KadeDB_Value_CreateFloat(1234.56);
  KDB_ValueHandle *is_active = KadeDB_Value_CreateBoolean(1);

  if (!user_id || !username || !email || !balance || !is_active) {
    KADEDB_SET_ERROR(error, KDB_ERROR_MEMORY_ALLOCATION,
                     "Failed to create values");
    goto cleanup;
  }

  // Set values in row
  KDB_ErrorInfo set_error;
  kadedb_clear_error(&set_error);

  if (!KadeDB_Row_Set(row, 0, user_id, &set_error)) {
    KADEDB_SET_ERROR(error, set_error.code, "Failed to set user_id");
    goto cleanup;
  }

  if (!KadeDB_Row_Set(row, 1, username, &set_error)) {
    KADEDB_SET_ERROR(error, set_error.code, "Failed to set username");
    goto cleanup;
  }

  if (!KadeDB_Row_Set(row, 2, email, &set_error)) {
    KADEDB_SET_ERROR(error, set_error.code, "Failed to set email");
    goto cleanup;
  }

  if (!KadeDB_Row_Set(row, 3, balance, &set_error)) {
    KADEDB_SET_ERROR(error, set_error.code, "Failed to set balance");
    goto cleanup;
  }

  if (!KadeDB_Row_Set(row, 4, is_active, &set_error)) {
    KADEDB_SET_ERROR(error, set_error.code, "Failed to set is_active");
    goto cleanup;
  }

  // Clean up value handles (row now owns the values)
  KadeDB_Value_Destroy(user_id);
  KadeDB_Value_Destroy(username);
  KadeDB_Value_Destroy(email);
  KadeDB_Value_Destroy(balance);
  KadeDB_Value_Destroy(is_active);

  return row;

cleanup:
  KadeDB_Value_Destroy(user_id);
  KadeDB_Value_Destroy(username);
  KadeDB_Value_Destroy(email);
  KadeDB_Value_Destroy(balance);
  KadeDB_Value_Destroy(is_active);
  KadeDB_Row_Destroy(row);
  return nullptr;
}

// =============================================================================
// EXAMPLE: RESOURCE MANAGEMENT PATTERNS
// =============================================================================

int example_automatic_cleanup(KDB_ErrorInfo *error) {
  kadedb_clear_error(error);

  // Initialize resource manager
  KDB_ResourceManager manager;
  if (!kadedb_resource_manager_init(&manager, 10)) {
    KADEDB_SET_ERROR(error, KDB_ERROR_MEMORY_ALLOCATION,
                     "Failed to init resource manager");
    return 0;
  }

  // Create resources that will be automatically cleaned up
  KDB_TableSchema *schema = KadeDB_TableSchema_Create();
  if (!schema) {
    KADEDB_SET_ERROR(error, KDB_ERROR_MEMORY_ALLOCATION,
                     "Failed to create schema");
    kadedb_resource_manager_cleanup(&manager);
    return 0;
  }

  // Add schema to resource manager
  if (!kadedb_resource_manager_add(
          &manager, schema, (void (*)(void *))KadeDB_TableSchema_Destroy)) {
    KADEDB_SET_ERROR(error, KDB_ERROR_MEMORY_ALLOCATION,
                     "Failed to add schema to manager");
    KadeDB_TableSchema_Destroy(schema);
    kadedb_resource_manager_cleanup(&manager);
    return 0;
  }

  // Create and add multiple rows
  for (int i = 0; i < 5; ++i) {
    KDB_Row *row = KadeDB_Row_Create(3);
    if (!row) {
      KADEDB_SET_ERROR(error, KDB_ERROR_MEMORY_ALLOCATION,
                       "Failed to create row");
      kadedb_resource_manager_cleanup(&manager); // This cleans up everything
      return 0;
    }

    if (!kadedb_resource_manager_add(&manager, row,
                                     (void (*)(void *))KadeDB_Row_Destroy)) {
      KADEDB_SET_ERROR(error, KDB_ERROR_MEMORY_ALLOCATION,
                       "Failed to add row to manager");
      KadeDB_Row_Destroy(row);
      kadedb_resource_manager_cleanup(&manager);
      return 0;
    }
  }

  // Simulate some work that might fail
  // If any error occurs, the cleanup call below will handle all resources

  // All resources are automatically cleaned up here
  kadedb_resource_manager_cleanup(&manager);

  return 1;
}

int example_manual_cleanup(KDB_ErrorInfo *error) {
  kadedb_clear_error(error);

  KDB_TableSchema *schema = nullptr;
  KDB_Row *row1 = nullptr;
  KDB_Row *row2 = nullptr;
  KDB_ValueHandle *value = nullptr;

  // Create resources with manual cleanup
  schema = KadeDB_TableSchema_Create();
  if (!schema) {
    KADEDB_SET_ERROR(error, KDB_ERROR_MEMORY_ALLOCATION,
                     "Failed to create schema");
    goto cleanup;
  }

  row1 = KadeDB_Row_Create(3);
  if (!row1) {
    KADEDB_SET_ERROR(error, KDB_ERROR_MEMORY_ALLOCATION,
                     "Failed to create row1");
    goto cleanup;
  }

  row2 = KadeDB_Row_Create(3);
  if (!row2) {
    KADEDB_SET_ERROR(error, KDB_ERROR_MEMORY_ALLOCATION,
                     "Failed to create row2");
    goto cleanup;
  }

  value = KadeDB_Value_CreateString("test");
  if (!value) {
    KADEDB_SET_ERROR(error, KDB_ERROR_MEMORY_ALLOCATION,
                     "Failed to create value");
    goto cleanup;
  }

  // Do some work...
  // If any operation fails, we jump to cleanup

  // Simulate a potential failure point
  if (rand() % 100 < 10) { // 10% chance of simulated failure
    KADEDB_SET_ERROR(error, KDB_ERROR_UNKNOWN, "Simulated random failure");
    goto cleanup;
  }

cleanup:
  // Safe cleanup using macros (they handle null pointers)
  KADEDB_SAFE_DESTROY(TableSchema, schema);
  KADEDB_SAFE_DESTROY(Row, row1);
  KADEDB_SAFE_DESTROY(Row, row2);
  KADEDB_SAFE_DESTROY(Value, value);

  return kadedb_has_error(error) ? 0 : 1;
}

// =============================================================================
// EXAMPLE: BULK DATA OPERATIONS
// =============================================================================

int example_bulk_data_processing(const KDB_RowView *rows,
                                 unsigned long long row_count, char *csv_output,
                                 unsigned long long output_size,
                                 KDB_ErrorInfo *error) {
  kadedb_clear_error(error);

  if (!rows && row_count > 0) {
    KADEDB_SET_ERROR(error, KDB_ERROR_INVALID_ARGUMENT, "Rows array is null");
    return 0;
  }

  if (!csv_output || output_size == 0) {
    KADEDB_SET_ERROR(error, KDB_ERROR_INVALID_ARGUMENT,
                     "Output buffer is null or zero size");
    return 0;
  }

  // Define column names for CSV output
  const char *column_names[] = {"user_id", "username", "email", "balance",
                                "is_active"};
  const KDB_ColumnType column_types[] = {KDB_COL_INTEGER, KDB_COL_STRING,
                                         KDB_COL_STRING, KDB_COL_FLOAT,
                                         KDB_COL_BOOLEAN};

  unsigned long long required_len = 0;

  // Convert to CSV with header
  int result =
      KadeDB_Result_ToCSV(column_names, column_types, 5, rows, row_count, ',',
                          1, csv_output, output_size, &required_len);

  if (!result) {
    KADEDB_SET_ERROR(error, KDB_ERROR_SERIALIZATION,
                     "Failed to convert to CSV");
    return 0;
  }

  if (required_len > output_size) {
    char err_msg[256];
    snprintf(err_msg, sizeof(err_msg),
             "Output buffer too small: need %llu bytes, have %llu",
             required_len, output_size);
    KADEDB_SET_ERROR(error, KDB_ERROR_OUT_OF_RANGE, err_msg);
    return 0;
  }

  return 1;
}

// =============================================================================
// EXAMPLE: PYTHON COMPATIBILITY HELPERS
// =============================================================================

int example_python_value_conversion(const KDB_ValueHandle *handle,
                                    PythonCompatibleValue *out_value,
                                    KDB_ErrorInfo *error) {
  kadedb_clear_error(error);

  if (!handle || !out_value) {
    KADEDB_SET_ERROR(error, KDB_ERROR_INVALID_ARGUMENT,
                     "Handle or output value is null");
    return 0;
  }

  // Clear the output value
  memset(out_value, 0, sizeof(PythonCompatibleValue));

  KDB_ValueType type = KadeDB_Value_GetType(handle);
  out_value->type = static_cast<int>(type);

  KDB_ErrorInfo extraction_error;
  kadedb_clear_error(&extraction_error);

  switch (type) {
  case KDB_VAL_NULL:
    // Nothing to set for null
    break;

  case KDB_VAL_INTEGER:
    out_value->data.i64_val = KadeDB_Value_AsInteger(handle, &extraction_error);
    if (kadedb_has_error(&extraction_error)) {
      KADEDB_SET_ERROR(error, extraction_error.code,
                       "Failed to extract integer");
      return 0;
    }
    break;

  case KDB_VAL_FLOAT:
    out_value->data.f64_val = KadeDB_Value_AsFloat(handle, &extraction_error);
    if (kadedb_has_error(&extraction_error)) {
      KADEDB_SET_ERROR(error, extraction_error.code, "Failed to extract float");
      return 0;
    }
    break;

  case KDB_VAL_STRING: {
    const char *str = KadeDB_Value_AsString(handle, &extraction_error);
    if (kadedb_has_error(&extraction_error)) {
      KADEDB_SET_ERROR(error, extraction_error.code,
                       "Failed to extract string");
      return 0;
    }

    // Allocate a copy for Python (Python will need to free this)
    out_value->data.str_val = KadeDB_String_Duplicate(str);
    if (!out_value->data.str_val) {
      KADEDB_SET_ERROR(error, KDB_ERROR_MEMORY_ALLOCATION,
                       "Failed to duplicate string");
      return 0;
    }
    break;
  }

  case KDB_VAL_BOOLEAN:
    out_value->data.bool_val =
        KadeDB_Value_AsBoolean(handle, &extraction_error);
    if (kadedb_has_error(&extraction_error)) {
      KADEDB_SET_ERROR(error, extraction_error.code,
                       "Failed to extract boolean");
      return 0;
    }
    break;

  default:
    KADEDB_SET_ERROR(error, KDB_ERROR_TYPE_MISMATCH, "Unknown value type");
    return 0;
  }

  return 1;
}

void example_free_python_value(PythonCompatibleValue *value) {
  if (value && value->type == static_cast<int>(KDB_VAL_STRING)) {
    KadeDB_String_Free(value->data.str_val);
    value->data.str_val = nullptr;
  }
}

} // extern "C"
