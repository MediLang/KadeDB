#include "kadedb/kadedb.h"
#include <assert.h>
#include <stdio.h>

int main() {
  assert(KadeDB_Initialize() == 1);

  // Create storage and a simple table
  KadeDB_Storage *st = KadeDB_CreateStorage();
  assert(st);
  KDB_TableSchema *schema = KadeDB_TableSchema_Create();
  assert(schema);
  KDB_TableColumnEx idcol = {"id", KDB_COL_INTEGER, 0, 1, NULL};
  assert(KadeDB_TableSchema_AddColumn(schema, &idcol) == 1);
  assert(KadeDB_CreateTable(st, "t", schema) == 1);

  // Query and test invalid column index access
  KadeDB_ResultSet *rs = KadeDB_ExecuteQuery(st, "SELECT * FROM t");
  assert(rs);
  // No row yet; NextRow should be 0
  assert(KadeDB_ResultSet_NextRow(rs) == 0);

  // Column metadata: out-of-range
  assert(KadeDB_ResultSet_GetColumnName(rs, 5) == NULL);
  assert(KadeDB_ResultSet_GetColumnType(rs, -1) == -1);

  // Typed getters on invalid state should fail and set ok=0
  int ok = 1;
  (void)KadeDB_ResultSet_GetInt64(rs, 0, &ok);
  assert(ok == 0);

  // Add a row and access invalid column
  KDB_Value v;
  v.type = KDB_VAL_INTEGER;
  v.as.i64 = 7;
  KDB_RowView row = {&v, 1};
  assert(KadeDB_InsertRow(st, "t", &row) == 1);

  rs = KadeDB_ExecuteQuery(st, "SELECT * FROM t");
  assert(rs);
  assert(KadeDB_ResultSet_NextRow(rs) == 1);
  ok = 1;
  (void)KadeDB_ResultSet_GetInt64(rs, 2, &ok); // invalid column
  assert(ok == 0);
  const char *err = KadeDB_ResultSet_GetLastError(rs);
  // Error message may or may not be set, but call must be safe
  (void)err;

  KadeDB_DestroyResultSet(rs);
  KadeDB_TableSchema_Destroy(schema);
  KadeDB_DestroyStorage(st);
  return 0;
}
