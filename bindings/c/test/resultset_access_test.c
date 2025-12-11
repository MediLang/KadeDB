#include "kadedb/kadedb.h"
#include <assert.h>
#include <stdio.h>

static KDB_Value make_int(long long v) {
  KDB_Value x;
  x.type = KDB_VAL_INTEGER;
  x.as.i64 = v;
  return x;
}
static KDB_Value make_str(const char *s) {
  KDB_Value x;
  x.type = KDB_VAL_STRING;
  x.as.str = s;
  return x;
}
static KDB_Value make_bool(int b) {
  KDB_Value x;
  x.type = KDB_VAL_BOOLEAN;
  x.as.boolean = b;
  return x;
}

int main() {
  assert(KadeDB_Initialize() == 1);
  KadeDB_Storage *st = KadeDB_CreateStorage();
  assert(st);

  // Schema: id (INTEGER, unique), name (STRING), active (BOOLEAN)
  KDB_TableSchema *schema = KadeDB_TableSchema_Create();
  assert(schema);
  KDB_TableColumnEx idcol = {"id", KDB_COL_INTEGER, 0, 1, NULL};
  KDB_TableColumnEx namecol = {"name", KDB_COL_STRING, 0, 0, NULL};
  KDB_TableColumnEx activecol = {"active", KDB_COL_BOOLEAN, 0, 0, NULL};
  assert(KadeDB_TableSchema_AddColumn(schema, &idcol) == 1);
  assert(KadeDB_TableSchema_AddColumn(schema, &namecol) == 1);
  assert(KadeDB_TableSchema_AddColumn(schema, &activecol) == 1);

  assert(KadeDB_CreateTable(st, "users", schema) == 1);

  // Insert a row
  {
    KDB_Value vals[3];
    vals[0] = make_int(42);
    vals[1] = make_str("alice");
    vals[2] = make_bool(1);
    KDB_RowView row = {vals, 3};
    assert(KadeDB_InsertRow(st, "users", &row) == 1);
  }

  KadeDB_ResultSet *rs = KadeDB_ExecuteQuery(st, "SELECT * FROM users");
  assert(rs);

  // Metadata
  int ncols = KadeDB_ResultSet_ColumnCount(rs);
  assert(ncols == 3);
  const char *c0 = KadeDB_ResultSet_GetColumnName(rs, 0);
  const char *c1 = KadeDB_ResultSet_GetColumnName(rs, 1);
  const char *c2 = KadeDB_ResultSet_GetColumnName(rs, 2);
  assert(c0 && c1 && c2);
  int t0 = KadeDB_ResultSet_GetColumnType(rs, 0);
  int t1 = KadeDB_ResultSet_GetColumnType(rs, 1);
  int t2 = KadeDB_ResultSet_GetColumnType(rs, 2);
  assert(t0 == KDB_COL_INTEGER);
  assert(t1 == KDB_COL_STRING);
  assert(t2 == KDB_COL_BOOLEAN);
  int idx_name = KadeDB_ResultSet_FindColumn(rs, "name");
  assert(idx_name == 1);

  // Row access
  assert(KadeDB_ResultSet_NextRow(rs) == 1);
  int ok = 0;
  long long id = KadeDB_ResultSet_GetInt64(rs, 0, &ok);
  assert(ok == 1 && id == 42);
  const char *n = KadeDB_ResultSet_GetString(rs, 1);
  assert(n);
  int active = KadeDB_ResultSet_GetBool(rs, 2, &ok);
  assert(ok == 1 && active == 1);

  // Reset and iterate again
  assert(KadeDB_ResultSet_Reset(rs) == 1);
  assert(KadeDB_ResultSet_NextRow(rs) == 1);

  KadeDB_DestroyResultSet(rs);
  KadeDB_TableSchema_Destroy(schema);
  KadeDB_DestroyStorage(st);
  return 0;
}
