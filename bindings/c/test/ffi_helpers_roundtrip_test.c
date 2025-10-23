#include "kadedb/kadedb.h"
#include "kadedb/kadedb_ffi_helpers.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_rowshallow_roundtrip() {
  KDB_ErrorInfo err;
  kadedb_clear_error(&err);

  // Create deep row with 3 columns
  KDB_Row *row = KadeDB_Row_Create(3);
  assert(row);

  KDB_ValueHandle *v0 = KadeDB_Value_CreateInteger(42);
  KDB_ValueHandle *v1 = KadeDB_Value_CreateString("hello");
  KDB_ValueHandle *v2 = KadeDB_Value_CreateBoolean(1);
  assert(v0 && v1 && v2);

  assert(KadeDB_Row_Set(row, 0, v0, &err));
  assert(KadeDB_Row_Set(row, 1, v1, &err));
  assert(KadeDB_Row_Set(row, 2, v2, &err));

  // Create shallow from deep, then back to deep
  KDB_RowShallow *sh = KadeDB_RowShallow_FromRow(row);
  assert(sh);
  assert(KadeDB_RowShallow_Size(sh) == 3);

  // Verify get on shallow returns equal content
  KDB_ValueHandle *g1 = KadeDB_RowShallow_Get(sh, 1, &err);
  assert(g1);
  const char *s = KadeDB_Value_AsString(g1, &err);
  assert(s && strcmp(s, "hello") == 0);
  KadeDB_Value_Destroy(g1);

  // Set a new value in shallow and convert back to deep
  KDB_ValueHandle *v1b = KadeDB_Value_CreateString("world");
  assert(v1b);
  assert(KadeDB_RowShallow_Set(sh, 1, v1b, &err));

  KDB_Row *row2 = KadeDB_RowShallow_ToRow(sh);
  assert(row2);
  KDB_ValueHandle *g1b = KadeDB_Row_Get(row2, 1, &err);
  assert(g1b);
  const char *s2 = KadeDB_Value_AsString(g1b, &err);
  assert(s2 && strcmp(s2, "world") == 0);
  KadeDB_Value_Destroy(g1b);

  // Cleanup
  KadeDB_Row_Destroy(row);
  KadeDB_Row_Destroy(row2);
  KadeDB_RowShallow_Destroy(sh);
  KadeDB_Value_Destroy(v0);
  KadeDB_Value_Destroy(v1);
  KadeDB_Value_Destroy(v2);
  KadeDB_Value_Destroy(v1b);
}

static void test_value_handle_roundtrip() {
  KDB_ErrorInfo err;
  kadedb_clear_error(&err);

  // C value -> handle -> C value
  KDB_Value cval;
  cval.type = KDB_VAL_FLOAT;
  cval.as.f64 = 3.5;

  KDB_ValueHandle *h = kadedb_value_to_handle(&cval, &err);
  assert(h);

  KDB_Value out;
  int ok = kadedb_handle_to_value(h, &out, &err);
  assert(ok);
  assert(out.type == KDB_VAL_FLOAT);
  assert(out.as.f64 == 3.5);

  KadeDB_Value_Destroy(h);
}

static void test_document_helpers() {
  KDB_ErrorInfo err;
  kadedb_clear_error(&err);

  const char *keys[] = {"id", "name", "active"};
  KDB_Value vals[3];
  vals[0].type = KDB_VAL_INTEGER;
  vals[0].as.i64 = 7;
  vals[1].type = KDB_VAL_STRING;
  vals[1].as.str = "dana";
  vals[2].type = KDB_VAL_BOOLEAN;
  vals[2].as.boolean = 1;

  KDB_KeyValue *doc = NULL;
  int ok = kadedb_create_document(keys, vals, 3, &doc, &err);
  assert(ok && doc);

  // validate deep-copied content
  assert(strcmp(doc[1].key, "name") == 0);
  assert(doc[1].value.type == KDB_VAL_STRING);
  assert(strcmp(doc[1].value.as.str, "dana") == 0);

  kadedb_free_document(doc, 3);
}

int main() {
  printf("C FFI helpers roundtrip tests\n");
  test_rowshallow_roundtrip();
  test_value_handle_roundtrip();
  test_document_helpers();
  printf("All FFI helper tests passed.\n");
  return 0;
}
