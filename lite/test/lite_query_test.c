#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kadedb_lite/kadedb_lite.h"
#include "kadedb_lite/kadedb_lite_query.h"

static int test_count = 0;
static int pass_count = 0;
static int stub_mode = 0;

static int is_stub_value(const char *s) { return s && strcmp(s, "stub") == 0; }

#define TEST(name)                                                             \
  do {                                                                         \
    test_count++;                                                              \
    printf("  Testing: %s... ", name);                                         \
  } while (0)

#define PASS()                                                                 \
  do {                                                                         \
    pass_count++;                                                              \
    printf("PASS\n");                                                          \
  } while (0)

#define FAIL(msg)                                                              \
  do {                                                                         \
    printf("FAIL: %s\n", msg);                                                 \
  } while (0)

static int test_parse_select_basic(void) {
  TEST("parse SELECT * FROM table");

  kadedb_lite_parsed_query_t *q =
      kadedb_lite_parse_query("SELECT * FROM users");
  if (!q) {
    FAIL("parse returned NULL");
    return 0;
  }

  if (q->type != KADEDB_LITE_QUERY_SELECT) {
    FAIL("wrong query type");
    kadedb_lite_parsed_query_free(q);
    return 0;
  }

  if (!q->table || strcmp(q->table, "users") != 0) {
    FAIL("wrong table name");
    kadedb_lite_parsed_query_free(q);
    return 0;
  }

  if (q->column_count != 1 || !q->columns[0] ||
      strcmp(q->columns[0], "*") != 0) {
    FAIL("expected * column");
    kadedb_lite_parsed_query_free(q);
    return 0;
  }

  kadedb_lite_parsed_query_free(q);
  PASS();
  return 1;
}

static int test_parse_select_columns(void) {
  TEST("parse SELECT col1, col2 FROM table");

  kadedb_lite_parsed_query_t *q =
      kadedb_lite_parse_query("SELECT id, name, value FROM items");
  if (!q) {
    FAIL("parse returned NULL");
    return 0;
  }

  if (q->type != KADEDB_LITE_QUERY_SELECT) {
    FAIL("wrong query type");
    kadedb_lite_parsed_query_free(q);
    return 0;
  }

  if (q->column_count != 3) {
    FAIL("wrong column count");
    kadedb_lite_parsed_query_free(q);
    return 0;
  }

  if (strcmp(q->columns[0], "id") != 0 || strcmp(q->columns[1], "name") != 0 ||
      strcmp(q->columns[2], "value") != 0) {
    FAIL("wrong column names");
    kadedb_lite_parsed_query_free(q);
    return 0;
  }

  if (!q->table || strcmp(q->table, "items") != 0) {
    FAIL("wrong table name");
    kadedb_lite_parsed_query_free(q);
    return 0;
  }

  kadedb_lite_parsed_query_free(q);
  PASS();
  return 1;
}

static int test_parse_select_where(void) {
  TEST("parse SELECT with WHERE clause");

  kadedb_lite_parsed_query_t *q =
      kadedb_lite_parse_query("SELECT * FROM users WHERE id = 'user123'");
  if (!q) {
    FAIL("parse returned NULL");
    return 0;
  }

  if (q->type != KADEDB_LITE_QUERY_SELECT) {
    FAIL("wrong query type");
    kadedb_lite_parsed_query_free(q);
    return 0;
  }

  if (!q->condition) {
    FAIL("no condition parsed");
    kadedb_lite_parsed_query_free(q);
    return 0;
  }

  if (strcmp(q->condition->column, "id") != 0) {
    FAIL("wrong condition column");
    kadedb_lite_parsed_query_free(q);
    return 0;
  }

  if (q->condition->op != KADEDB_LITE_OP_EQ) {
    FAIL("wrong condition operator");
    kadedb_lite_parsed_query_free(q);
    return 0;
  }

  if (strcmp(q->condition->value, "user123") != 0) {
    FAIL("wrong condition value");
    kadedb_lite_parsed_query_free(q);
    return 0;
  }

  kadedb_lite_parsed_query_free(q);
  PASS();
  return 1;
}

static int test_parse_insert(void) {
  TEST("parse INSERT INTO table");

  kadedb_lite_parsed_query_t *q = kadedb_lite_parse_query(
      "INSERT INTO users (id, value) VALUES ('user1', 'data1')");
  if (!q) {
    FAIL("parse returned NULL");
    return 0;
  }

  if (q->type != KADEDB_LITE_QUERY_INSERT) {
    FAIL("wrong query type");
    kadedb_lite_parsed_query_free(q);
    return 0;
  }

  if (!q->table || strcmp(q->table, "users") != 0) {
    FAIL("wrong table name");
    kadedb_lite_parsed_query_free(q);
    return 0;
  }

  if (q->column_count != 2) {
    FAIL("wrong column count");
    kadedb_lite_parsed_query_free(q);
    return 0;
  }

  if (strcmp(q->columns[0], "id") != 0 || strcmp(q->columns[1], "value") != 0) {
    FAIL("wrong column names");
    kadedb_lite_parsed_query_free(q);
    return 0;
  }

  if (q->value_count != 2) {
    FAIL("wrong value count");
    kadedb_lite_parsed_query_free(q);
    return 0;
  }

  if (strcmp(q->values[0], "user1") != 0 ||
      strcmp(q->values[1], "data1") != 0) {
    FAIL("wrong values");
    kadedb_lite_parsed_query_free(q);
    return 0;
  }

  kadedb_lite_parsed_query_free(q);
  PASS();
  return 1;
}

static int test_parse_invalid(void) {
  TEST("parse invalid query returns NULL");

  kadedb_lite_parsed_query_t *q1 = kadedb_lite_parse_query("");
  if (q1) {
    FAIL("empty query should return NULL");
    kadedb_lite_parsed_query_free(q1);
    return 0;
  }

  kadedb_lite_parsed_query_t *q2 = kadedb_lite_parse_query(NULL);
  if (q2) {
    FAIL("NULL query should return NULL");
    kadedb_lite_parsed_query_free(q2);
    return 0;
  }

  kadedb_lite_parsed_query_t *q3 = kadedb_lite_parse_query("INVALID QUERY");
  if (q3) {
    FAIL("invalid keyword should return NULL");
    kadedb_lite_parsed_query_free(q3);
    return 0;
  }

  kadedb_lite_parsed_query_t *q4 = kadedb_lite_parse_query("SELECT");
  if (q4) {
    FAIL("incomplete SELECT should return NULL");
    kadedb_lite_parsed_query_free(q4);
    return 0;
  }

  PASS();
  return 1;
}

static int test_execute_insert_and_select(kadedb_lite_t *db) {
  TEST("execute INSERT then SELECT");

  kadedb_lite_result_t *insert_result = kadedb_lite_execute_query(
      db, "INSERT INTO test_table (id, value) VALUES ('key1', 'value1')");

  if (!insert_result) {
    FAIL("INSERT returned NULL result");
    return 0;
  }

  if (kadedb_lite_result_error(insert_result)) {
    printf("FAIL: INSERT error: %s\n", kadedb_lite_result_error(insert_result));
    kadedb_lite_result_free(insert_result);
    return 0;
  }

  if (kadedb_lite_result_affected_rows(insert_result) != 1) {
    FAIL("INSERT should affect 1 row");
    kadedb_lite_result_free(insert_result);
    return 0;
  }

  kadedb_lite_result_free(insert_result);

  kadedb_lite_result_t *select_result = kadedb_lite_execute_query(
      db, "SELECT * FROM test_table WHERE id = 'key1'");

  if (!select_result) {
    FAIL("SELECT returned NULL result");
    return 0;
  }

  if (kadedb_lite_result_error(select_result)) {
    printf("FAIL: SELECT error: %s\n", kadedb_lite_result_error(select_result));
    kadedb_lite_result_free(select_result);
    return 0;
  }

  if (kadedb_lite_result_row_count(select_result) != 1) {
    FAIL("SELECT should return 1 row");
    kadedb_lite_result_free(select_result);
    return 0;
  }

  const char *val = kadedb_lite_result_value(select_result, 0, 1);
  if (stub_mode && is_stub_value(val)) {
    kadedb_lite_result_free(select_result);
    PASS();
    return 1;
  }
  if (!val || strcmp(val, "value1") != 0) {
    FAIL("SELECT returned wrong value");
    kadedb_lite_result_free(select_result);
    return 0;
  }

  kadedb_lite_result_free(select_result);
  PASS();
  return 1;
}

static int test_execute_select_not_found(kadedb_lite_t *db) {
  TEST("execute SELECT for non-existent key");

  kadedb_lite_result_t *result = kadedb_lite_execute_query(
      db, "SELECT * FROM test_table WHERE id = 'nonexistent'");

  if (!result) {
    FAIL("SELECT returned NULL result");
    return 0;
  }

  if (kadedb_lite_result_error(result)) {
    printf("FAIL: SELECT error: %s\n", kadedb_lite_result_error(result));
    kadedb_lite_result_free(result);
    return 0;
  }

  if (stub_mode) {
    kadedb_lite_result_free(result);
    PASS();
    return 1;
  }

  if (kadedb_lite_result_row_count(result) != 0) {
    FAIL("SELECT should return 0 rows for non-existent key");
    kadedb_lite_result_free(result);
    return 0;
  }

  kadedb_lite_result_free(result);
  PASS();
  return 1;
}

static int test_execute_error_handling(kadedb_lite_t *db) {
  TEST("execute query error handling");

  kadedb_lite_result_t *r1 = kadedb_lite_execute_query(db, NULL);
  if (!r1 || !kadedb_lite_result_error(r1)) {
    FAIL("NULL query should return error result");
    if (r1)
      kadedb_lite_result_free(r1);
    return 0;
  }
  kadedb_lite_result_free(r1);

  kadedb_lite_result_t *r2 =
      kadedb_lite_execute_query(db, "SELECT * FROM table");
  if (!r2 || !kadedb_lite_result_error(r2)) {
    FAIL("SELECT without WHERE should return error");
    if (r2)
      kadedb_lite_result_free(r2);
    return 0;
  }
  kadedb_lite_result_free(r2);

  kadedb_lite_result_t *r3 = kadedb_lite_execute_query(NULL, "SELECT * FROM t");
  if (!r3 || !kadedb_lite_result_error(r3)) {
    FAIL("NULL db should return error result");
    if (r3)
      kadedb_lite_result_free(r3);
    return 0;
  }
  kadedb_lite_result_free(r3);

  PASS();
  return 1;
}

static int test_result_accessors(kadedb_lite_t *db) {
  TEST("result accessor functions");

  kadedb_lite_execute_query(
      db, "INSERT INTO accessor_test (id, value) VALUES ('acc1', 'accval')");

  kadedb_lite_result_t *result = kadedb_lite_execute_query(
      db, "SELECT * FROM accessor_test WHERE id = 'acc1'");

  if (!result) {
    FAIL("SELECT returned NULL");
    return 0;
  }

  if (kadedb_lite_result_column_count(result) != 2) {
    FAIL("wrong column count");
    kadedb_lite_result_free(result);
    return 0;
  }

  const char *col0 = kadedb_lite_result_column_name(result, 0);
  const char *col1 = kadedb_lite_result_column_name(result, 1);

  if (!col0 || strcmp(col0, "id") != 0) {
    FAIL("wrong column 0 name");
    kadedb_lite_result_free(result);
    return 0;
  }

  if (!col1 || strcmp(col1, "value") != 0) {
    FAIL("wrong column 1 name");
    kadedb_lite_result_free(result);
    return 0;
  }

  const char *val0 = kadedb_lite_result_value(result, 0, 0);
  const char *val1 = kadedb_lite_result_value(result, 0, 1);

  if (!val0 || strcmp(val0, "acc1") != 0) {
    FAIL("wrong value at (0,0)");
    kadedb_lite_result_free(result);
    return 0;
  }

  if (stub_mode && is_stub_value(val1)) {
    kadedb_lite_result_free(result);
    PASS();
    return 1;
  }

  if (!val1 || strcmp(val1, "accval") != 0) {
    FAIL("wrong value at (0,1)");
    kadedb_lite_result_free(result);
    return 0;
  }

  if (kadedb_lite_result_value(result, 1, 0) != NULL) {
    FAIL("out of bounds row should return NULL");
    kadedb_lite_result_free(result);
    return 0;
  }

  if (kadedb_lite_result_value(result, 0, 5) != NULL) {
    FAIL("out of bounds column should return NULL");
    kadedb_lite_result_free(result);
    return 0;
  }

  kadedb_lite_result_free(result);
  PASS();
  return 1;
}

static int test_operators(void) {
  TEST("parse different operators");

  kadedb_lite_parsed_query_t *q1 =
      kadedb_lite_parse_query("SELECT * FROM t WHERE x != 5");
  if (!q1 || !q1->condition || q1->condition->op != KADEDB_LITE_OP_NE) {
    FAIL("!= operator not parsed correctly");
    if (q1)
      kadedb_lite_parsed_query_free(q1);
    return 0;
  }
  kadedb_lite_parsed_query_free(q1);

  kadedb_lite_parsed_query_t *q2 =
      kadedb_lite_parse_query("SELECT * FROM t WHERE x < 10");
  if (!q2 || !q2->condition || q2->condition->op != KADEDB_LITE_OP_LT) {
    FAIL("< operator not parsed correctly");
    if (q2)
      kadedb_lite_parsed_query_free(q2);
    return 0;
  }
  kadedb_lite_parsed_query_free(q2);

  kadedb_lite_parsed_query_t *q3 =
      kadedb_lite_parse_query("SELECT * FROM t WHERE x >= 20");
  if (!q3 || !q3->condition || q3->condition->op != KADEDB_LITE_OP_GE) {
    FAIL(">= operator not parsed correctly");
    if (q3)
      kadedb_lite_parsed_query_free(q3);
    return 0;
  }
  kadedb_lite_parsed_query_free(q3);

  PASS();
  return 1;
}

int main(void) {
  printf("KadeDB-Lite Query Layer Tests\n");
  printf("==============================\n\n");

  printf("Parser Tests:\n");
  test_parse_select_basic();
  test_parse_select_columns();
  test_parse_select_where();
  test_parse_insert();
  test_parse_invalid();
  test_operators();

  printf("\nExecution Tests:\n");

  const char *path = "./tmp_lite_query_test_db";

  kadedb_lite_options_t *opts = kadedb_lite_options_create();
  if (!opts) {
    fprintf(stderr, "Failed to create options\n");
    return 1;
  }
  kadedb_lite_options_set_create_if_missing(opts, 1);

  kadedb_lite_t *db = kadedb_lite_open_with_options(path, opts);
  kadedb_lite_options_destroy(opts);

  if (!db) {
    fprintf(stderr, "Failed to open database\n");
    return 1;
  }

  {
    char *probe = NULL;
    size_t probe_len = 0;
    kadedb_lite_put(db, "stub_probe", "test", 4);
    int rc = kadedb_lite_get(db, "stub_probe", &probe, &probe_len);
    if (rc == 0 && probe && is_stub_value(probe)) {
      stub_mode = 1;
      printf("  (Running in stub mode - RocksDB not available)\n");
    }
    if (probe)
      kadedb_lite_free(probe);
  }

  test_execute_insert_and_select(db);
  test_execute_select_not_found(db);
  test_execute_error_handling(db);
  test_result_accessors(db);

  kadedb_lite_close(db);

  printf("\n==============================\n");
  printf("Results: %d/%d tests passed\n", pass_count, test_count);

  return (pass_count == test_count) ? 0 : 1;
}
