#include "kadedb/kadedb.h"
#include "kadedb/kadedb_ffi_helpers.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_string_lifetime_with_handle_to_value() {
  KDB_ErrorInfo err;
  kadedb_clear_error(&err);

  KDB_ValueHandle *vh = KadeDB_Value_CreateString("persist");
  assert(vh);

  KDB_Value out;
  int ok = kadedb_handle_to_value(vh, &out, &err);
  assert(ok);
  assert(out.type == KDB_VAL_STRING);

  // Pointer should remain valid while the handle lives
  const char *p1 = out.as.str;
  assert(p1 && strcmp(p1, "persist") == 0);

  // Re-query string view and ensure consistent pointer content (address may
  // differ)
  const char *p2 = KadeDB_Value_AsString(vh, &err);
  assert(p2 && strcmp(p2, "persist") == 0);

  // Destroy handle then pointer becomes invalid (cannot dereference safely).
  // Just ensure we don't use it after.
  KadeDB_Value_Destroy(vh);
}

static void test_rowshallow_error_paths() {
  KDB_ErrorInfo err;
  kadedb_clear_error(&err);

  KDB_RowShallow *sh = KadeDB_RowShallow_Create(1);
  assert(sh);

  // Invalid index on Set
  KDB_ValueHandle *v = KadeDB_Value_CreateInteger(1);
  int ok = KadeDB_RowShallow_Set(sh, 2, v, &err);
  assert(!ok);
  assert(kadedb_has_error(&err));
  assert(err.code == KDB_ERROR_OUT_OF_RANGE);

  // Invalid index on Get
  kadedb_clear_error(&err);
  KDB_ValueHandle *g = KadeDB_RowShallow_Get(sh, 3, &err);
  assert(g == NULL);
  assert(kadedb_has_error(&err));
  assert(err.code == KDB_ERROR_OUT_OF_RANGE);

  KadeDB_Value_Destroy(v);
  KadeDB_RowShallow_Destroy(sh);
}

static void test_multi_type_roundtrips() {
  KDB_ErrorInfo err;
  kadedb_clear_error(&err);

  // Integer
  KDB_Value vi;
  vi.type = KDB_VAL_INTEGER;
  vi.as.i64 = 123;
  KDB_ValueHandle *hi = kadedb_value_to_handle(&vi, &err);
  assert(hi);
  KDB_Value vi2;
  assert(kadedb_handle_to_value(hi, &vi2, &err));
  assert(vi2.type == KDB_VAL_INTEGER && vi2.as.i64 == 123);
  KadeDB_Value_Destroy(hi);

  // Boolean
  KDB_Value vb;
  vb.type = KDB_VAL_BOOLEAN;
  vb.as.boolean = 1;
  KDB_ValueHandle *hb = kadedb_value_to_handle(&vb, &err);
  assert(hb);
  KDB_Value vb2;
  assert(kadedb_handle_to_value(hb, &vb2, &err));
  assert(vb2.type == KDB_VAL_BOOLEAN && vb2.as.boolean == 1);
  KadeDB_Value_Destroy(hb);

  // String
  KDB_Value vs;
  vs.type = KDB_VAL_STRING;
  vs.as.str = "abc";
  KDB_ValueHandle *hs = kadedb_value_to_handle(&vs, &err);
  assert(hs);
  KDB_Value vs2;
  assert(kadedb_handle_to_value(hs, &vs2, &err));
  assert(vs2.type == KDB_VAL_STRING && strcmp(vs2.as.str, "abc") == 0);
  KadeDB_Value_Destroy(hs);
}

int main() {
  printf("C FFI helpers edge-case tests\n");
  test_string_lifetime_with_handle_to_value();
  test_rowshallow_error_paths();
  test_multi_type_roundtrips();
  printf("All FFI edge-case tests passed.\n");
  return 0;
}
