#include <stdio.h>
#include <string.h>

#include "kadedb/kadedb.h"

static void print_value(const char *s) {
  if (!s)
    printf("<null>");
  else
    printf("%s", s);
}

int main(void) {
  printf("KadeDB C ABI example\n");

  KadeDB_Storage *storage = KadeDB_CreateStorage();
  if (!storage) {
    fprintf(stderr, "KadeDB_CreateStorage failed\n");
    return 1;
  }

  KDB_TableSchema *schema = KadeDB_TableSchema_Create();
  if (!schema) {
    fprintf(stderr, "KadeDB_TableSchema_Create failed\n");
    KadeDB_DestroyStorage(storage);
    return 2;
  }

  KDB_TableColumnEx cols[3];
  memset(cols, 0, sizeof(cols));

  cols[0].name = "id";
  cols[0].type = KDB_COL_INTEGER;
  cols[0].nullable = 0;
  cols[0].unique = 1;
  cols[0].constraints = NULL;

  cols[1].name = "name";
  cols[1].type = KDB_COL_STRING;
  cols[1].nullable = 0;
  cols[1].unique = 0;
  cols[1].constraints = NULL;

  cols[2].name = "active";
  cols[2].type = KDB_COL_BOOLEAN;
  cols[2].nullable = 0;
  cols[2].unique = 0;
  cols[2].constraints = NULL;

  for (int i = 0; i < 3; i++) {
    if (!KadeDB_TableSchema_AddColumn(schema, &cols[i])) {
      fprintf(stderr, "KadeDB_TableSchema_AddColumn failed (col=%s)\n",
              cols[i].name);
      KadeDB_TableSchema_Destroy(schema);
      KadeDB_DestroyStorage(storage);
      return 3;
    }
  }

  if (!KadeDB_TableSchema_SetPrimaryKey(schema, "id")) {
    fprintf(stderr, "KadeDB_TableSchema_SetPrimaryKey failed\n");
    KadeDB_TableSchema_Destroy(schema);
    KadeDB_DestroyStorage(storage);
    return 4;
  }

  if (!KadeDB_CreateTable(storage, "users", schema)) {
    fprintf(stderr, "KadeDB_CreateTable failed\n");
    KadeDB_TableSchema_Destroy(schema);
    KadeDB_DestroyStorage(storage);
    return 5;
  }

  {
    KDB_Value values[3];
    values[0].type = KDB_VAL_INTEGER;
    values[0].as.i64 = 1;
    values[1].type = KDB_VAL_STRING;
    values[1].as.str = "alice";
    values[2].type = KDB_VAL_BOOLEAN;
    values[2].as.boolean = 1;

    KDB_RowView row;
    row.values = values;
    row.count = 3;

    if (!KadeDB_InsertRow(storage, "users", &row)) {
      fprintf(stderr, "KadeDB_InsertRow failed\n");
      KadeDB_TableSchema_Destroy(schema);
      KadeDB_DestroyStorage(storage);
      return 6;
    }
  }

  {
    KadeDB_ResultSet *rs = KadeDB_ExecuteQuery(storage, "SELECT * FROM users");
    if (!rs) {
      fprintf(stderr, "KadeDB_ExecuteQuery failed\n");
      KadeDB_TableSchema_Destroy(schema);
      KadeDB_DestroyStorage(storage);
      return 7;
    }

    int col_count = KadeDB_ResultSet_ColumnCount(rs);
    if (col_count <= 0) {
      const char *err = KadeDB_ResultSet_GetLastError(rs);
      fprintf(stderr, "ResultSet_ColumnCount failed: %s\n", err ? err : "");
      KadeDB_DestroyResultSet(rs);
      KadeDB_TableSchema_Destroy(schema);
      KadeDB_DestroyStorage(storage);
      return 8;
    }

    printf("Columns: %d\n", col_count);
    for (int c = 0; c < col_count; c++) {
      const char *name = KadeDB_ResultSet_GetColumnName(rs, c);
      printf("  [%d] ", c);
      print_value(name);
      printf("\n");
    }

    printf("Rows:\n");
    while (KadeDB_ResultSet_NextRow(rs)) {
      printf("  ");
      for (int c = 0; c < col_count; c++) {
        const char *v = KadeDB_ResultSet_GetString(rs, c);
        print_value(v);
        if (c + 1 < col_count)
          printf(", ");
      }
      printf("\n");
    }

    KadeDB_DestroyResultSet(rs);
  }

  KadeDB_TableSchema_Destroy(schema);
  KadeDB_DestroyStorage(storage);
  printf("done\n");
  return 0;
}
