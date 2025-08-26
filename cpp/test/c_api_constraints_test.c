#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "kadedb/kadedb.h"

static KDB_Value V_NULL() {
  KDB_Value v; v.type = KDB_VAL_NULL; return v;
}
static KDB_Value V_INT(long long x) {
  KDB_Value v; v.type = KDB_VAL_INTEGER; v.as.i64 = x; return v;
}
static KDB_Value V_STR(const char* s) {
  KDB_Value v; v.type = KDB_VAL_STRING; v.as.str = s; return v;
}

int main() {
  char buf[256];

  // Create document schema and add fields
  KDB_DocumentSchema* ds = KadeDB_DocumentSchema_Create();
  assert(ds);
  assert(KadeDB_DocumentSchema_AddField(ds, "status", KDB_COL_STRING, 0, 0) == 1);
  assert(KadeDB_DocumentSchema_AddField(ds, "age", KDB_COL_INTEGER, 0, 0) == 1);
  assert(KadeDB_DocumentSchema_AddField(ds, "id", KDB_COL_INTEGER, 1, 1) == 1);

  // Set string constraints for status: len [2,4], oneOf {ok, warn}
  const char* allowed[] = {"ok", "warn"};
  assert(KadeDB_DocumentSchema_SetStringConstraints(ds, "status", 2, 4, allowed, 2) == 1);

  // Set numeric constraints for age: [18,65]
  assert(KadeDB_DocumentSchema_SetNumericConstraints(ds, "age", 18.0, 65.0) == 1);

  // Valid document
  KDB_KeyValue d1_items[] = {
    {"status", V_STR("ok")},
    {"age", V_INT(30)},
    {"id", V_INT(1)}
  };
  assert(KadeDB_ValidateDocument(ds, d1_items, 3, buf, sizeof(buf)) == 1);

  // Too long status
  KDB_KeyValue d2_items[] = {
    {"status", V_STR("toolong")},
    {"age", V_INT(30)},
    {"id", V_INT(2)}
  };
  assert(KadeDB_ValidateDocument(ds, d2_items, 3, buf, sizeof(buf)) == 0);

  // Status not in oneOf
  KDB_KeyValue d3_items[] = {
    {"status", V_STR("no")},
    {"age", V_INT(30)},
    {"id", V_INT(3)}
  };
  assert(KadeDB_ValidateDocument(ds, d3_items, 3, buf, sizeof(buf)) == 0);

  // Age too low
  KDB_KeyValue d4_items[] = {
    {"status", V_STR("ok")},
    {"age", V_INT(10)},
    {"id", V_INT(4)}
  };
  assert(KadeDB_ValidateDocument(ds, d4_items, 3, buf, sizeof(buf)) == 0);

  // Age too high
  KDB_KeyValue d5_items[] = {
    {"status", V_STR("ok")},
    {"age", V_INT(80)},
    {"id", V_INT(5)}
  };
  assert(KadeDB_ValidateDocument(ds, d5_items, 3, buf, sizeof(buf)) == 0);

  // Uniqueness ignoreNulls
  KDB_KeyValue d6_items[] = {
    {"status", V_STR("ok")},
    {"age", V_INT(22)},
    {"id", V_NULL()}
  };
  KDB_KeyValue d7_items[] = {
    {"status", V_STR("warn")},
    {"age", V_INT(23)},
    {"id", V_NULL()}
  };

  KDB_DocumentView docs[] = {
    {d1_items, 3},
    {d6_items, 3},
    {d7_items, 3}
  };

  // ignore_nulls = 1 -> OK
  assert(KadeDB_ValidateUniqueDocuments(ds, docs, 3, 1, buf, sizeof(buf)) == 1);
  // ignore_nulls = 0 -> FAIL due to two null ids
  assert(KadeDB_ValidateUniqueDocuments(ds, docs, 3, 0, buf, sizeof(buf)) == 0);

  // Flip flags: make id non-nullable and non-unique, then null duplicates allowed but null invalid
  assert(KadeDB_DocumentSchema_SetFieldFlags(ds, "id", 0, 0) == 1);
  // Now validation of docs with null id must fail regardless
  assert(KadeDB_ValidateDocument(ds, d6_items, 3, buf, sizeof(buf)) == 0);

  KadeDB_DocumentSchema_Destroy(ds);
  return 0;
}
