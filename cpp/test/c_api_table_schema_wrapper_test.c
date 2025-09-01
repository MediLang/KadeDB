#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "kadedb/kadedb.h"

static KDB_Value V_NULL() {
  KDB_Value v;
  v.type = KDB_VAL_NULL;
  return v;
}
static KDB_Value V_INT(long long x) {
  KDB_Value v;
  v.type = KDB_VAL_INTEGER;
  v.as.i64 = x;
  return v;
}
static KDB_Value V_STR(const char *s) {
  KDB_Value v;
  v.type = KDB_VAL_STRING;
  v.as.str = s;
  return v;
}

int main() {
  char buf[256];

  // Create schema
  KDB_TableSchema *s = KadeDB_TableSchema_Create();
  assert(s);

  // Add columns via ColumnEx
  KDB_ColumnConstraints name_cc = {.min_len = 2,
                                   .max_len = 10,
                                   .one_of = NULL,
                                   .one_of_count = 0,
                                   .min_value = NAN,
                                   .max_value = NAN};
  KDB_TableColumnEx c_id = {"id", KDB_COL_INTEGER, 0, 1, NULL};
  KDB_TableColumnEx c_name = {"name", KDB_COL_STRING, 0, 0, &name_cc};
  KDB_TableColumnEx c_age = {"age", KDB_COL_INTEGER, 1, 0, NULL};
  assert(KadeDB_TableSchema_AddColumn(s, &c_id) == 1);
  assert(KadeDB_TableSchema_AddColumn(s, &c_name) == 1);
  assert(KadeDB_TableSchema_AddColumn(s, &c_age) == 1);

  // Set numeric constraints on age
  assert(KadeDB_TableSchema_SetNumericConstraints(s, "age", 18.0, 65.0) == 1);

  // Set primary key to id
  assert(KadeDB_TableSchema_SetPrimaryKey(s, "id") == 1);

  // Validate valid row
  KDB_Value r1_vals[] = {V_INT(1), V_STR("Tom"), V_INT(30)};
  KDB_RowView r1 = {r1_vals, 3};
  assert(KadeDB_TableSchema_ValidateRow(s, &r1, buf, sizeof(buf)) == 1);

  // Violations: name too short, age too high, null id (non-nullable)
  KDB_Value r2_vals[] = {V_NULL(), V_STR("T"), V_INT(70)};
  KDB_RowView r2 = {r2_vals, 3};
  assert(KadeDB_TableSchema_ValidateRow(s, &r2, buf, sizeof(buf)) == 0);

  // Uniqueness checks (id unique)
  KDB_RowView rows_ignore[] = {
      {(KDB_Value[]){V_NULL(), V_STR("Joe"), V_INT(25)}, 3},
      {(KDB_Value[]){V_NULL(), V_STR("Anna"), V_INT(26)}, 3},
  };
  assert(KadeDB_TableSchema_ValidateUniqueRows(s, rows_ignore, 2, 1, buf,
                                               sizeof(buf)) == 1);
  assert(KadeDB_TableSchema_ValidateUniqueRows(s, rows_ignore, 2, 0, buf,
                                               sizeof(buf)) == 0);

  KadeDB_TableSchema_Destroy(s);
  return 0;
}
