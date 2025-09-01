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

  // Define table columns with constraints
  KDB_ColumnConstraints name_cc = {
      .min_len = 2,
      .max_len = 4,
      .one_of = NULL,
      .one_of_count = 0,
      .min_value = NAN,
      .max_value = NAN,
  };
  const char *enum_vals[] = {"A", "B"};
  KDB_ColumnConstraints code_cc = {
      .min_len = -1,
      .max_len = -1,
      .one_of = enum_vals,
      .one_of_count = 2,
      .min_value = NAN,
      .max_value = NAN,
  };
  KDB_ColumnConstraints age_cc = {
      .min_len = -1,
      .max_len = -1,
      .one_of = NULL,
      .one_of_count = 0,
      .min_value = 18.0,
      .max_value = 65.0,
  };

  KDB_TableColumnEx cols[] = {
      {"id", KDB_COL_INTEGER, 1, 1, NULL},
      {"name", KDB_COL_STRING, 0, 0, &name_cc},
      {"age", KDB_COL_INTEGER, 0, 0, &age_cc},
      {"code", KDB_COL_STRING, 1, 0, &code_cc},
  };

  // Valid row
  KDB_Value r1_vals[] = {V_INT(1), V_STR("Tom"), V_INT(30), V_STR("A")};
  KDB_RowView r1 = {r1_vals, 4};
  assert(KadeDB_ValidateRow(cols, 4, &r1, buf, sizeof(buf)) == 1);

  // Violations: name too short, age too low, code not allowed
  KDB_Value r2_vals[] = {V_INT(2), V_STR("T"), V_INT(10), V_STR("Z")};
  KDB_RowView r2 = {r2_vals, 4};
  assert(KadeDB_ValidateRow(cols, 4, &r2, buf, sizeof(buf)) == 0);

  // Uniqueness: two rows with null ids
  KDB_RowView rows_ignore[] = {
      {(KDB_Value[]){V_NULL(), V_STR("Joe"), V_INT(25), V_STR("B")}, 4},
      {(KDB_Value[]){V_NULL(), V_STR("Anna"), V_INT(26), V_STR("A")}, 4},
  };
  KDB_TableColumn simple_cols[] = {
      {"id", KDB_COL_INTEGER, 1, 1},
      {"name", KDB_COL_STRING, 1, 0},
      {"age", KDB_COL_INTEGER, 1, 0},
      {"code", KDB_COL_STRING, 1, 0},
  };
  assert(KadeDB_ValidateUniqueRows(simple_cols, 4, rows_ignore, 2, 1, buf,
                                   sizeof(buf)) == 1);
  assert(KadeDB_ValidateUniqueRows(simple_cols, 4, rows_ignore, 2, 0, buf,
                                   sizeof(buf)) == 0);

  return 0;
}
