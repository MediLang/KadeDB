#include "kadedb/kadedb.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

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

int main() {
  printf("=== C ABI CRUD Bindings Test ===\n");
  assert(KadeDB_Initialize() == 1);

  KadeDB_Storage *st = KadeDB_CreateStorage();
  assert(st != NULL);

  // Build schema: id (INTEGER, unique), name (STRING)
  KDB_TableSchema *schema = KadeDB_TableSchema_Create();
  assert(schema);

  KDB_TableColumnEx idcol = {"id", KDB_COL_INTEGER, 0, 1, NULL};
  KDB_TableColumnEx namecol = {"name", KDB_COL_STRING, 0, 0, NULL};
  assert(KadeDB_TableSchema_AddColumn(schema, &idcol) == 1);
  assert(KadeDB_TableSchema_AddColumn(schema, &namecol) == 1);

  assert(KadeDB_CreateTable(st, "users", schema) == 1);

  // Verify list tables
  unsigned long long need = 0;
  assert(KadeDB_ListTables_ToCSV(st, ',', NULL, 0, &need) == 1);
  char buf[64];
  assert(need < sizeof(buf));
  assert(KadeDB_ListTables_ToCSV(st, ',', buf, sizeof(buf), NULL) == 1);
  assert(strstr(buf, "users") != NULL);

  // Insert two rows
  {
    KDB_Value vals[2];
    vals[0] = make_int(1);
    vals[1] = make_str("alice");
    KDB_RowView row = {vals, 2};
    assert(KadeDB_InsertRow(st, "users", &row) == 1);
  }
  {
    KDB_Value vals[2];
    vals[0] = make_int(2);
    vals[1] = make_str("carol");
    KDB_RowView row = {vals, 2};
    assert(KadeDB_InsertRow(st, "users", &row) == 1);
  }

  // Update: set name = "bob" where id == 1
  KDB_Assignment asg;
  asg.column = "name";
  asg.is_column_ref = 0;
  asg.column_ref = NULL;
  asg.constant = make_str("bob");
  KDB_Predicate pred;
  pred.column = "id";
  pred.op = KDB_OP_EQ;
  pred.rhs = make_int(1);
  unsigned long long updated = 0;
  assert(KadeDB_UpdateRows(st, "users", &asg, 1, &pred, &updated) == 1);
  assert(updated == 1);

  // Validate update via SELECT: ensure rows are present (content not strictly
  // asserted here)
  KadeDB_ResultSet *rs = KadeDB_ExecuteQuery(st, "SELECT * FROM users");
  assert(rs != NULL);
  int count_rows = 0;
  while (KadeDB_ResultSet_NextRow(rs)) {
    ++count_rows;
    (void)KadeDB_ResultSet_GetString(rs, 0);
    (void)KadeDB_ResultSet_GetString(rs, 1);
  }
  assert(count_rows == 2);
  KadeDB_DestroyResultSet(rs);

  // Delete where id == 2
  KDB_Predicate pred2;
  pred2.column = "id";
  pred2.op = KDB_OP_EQ;
  pred2.rhs = make_int(2);
  unsigned long long deleted = 0;
  assert(KadeDB_DeleteRows(st, "users", &pred2, &deleted) == 1);
  assert(deleted == 1);

  // Truncate table
  assert(KadeDB_TruncateTable(st, "users") == 1);
  // After truncate, query should have no rows
  rs = KadeDB_ExecuteQuery(st, "SELECT * FROM users");
  assert(rs != NULL);
  assert(KadeDB_ResultSet_NextRow(rs) == 0);
  KadeDB_DestroyResultSet(rs);

  // Drop table
  assert(KadeDB_DropTable(st, "users") == 1);

  KadeDB_TableSchema_Destroy(schema);
  KadeDB_DestroyStorage(st);
  KadeDB_Shutdown();
  printf("✓ CRUD Bindings test passed\n");
  return 0;
}
