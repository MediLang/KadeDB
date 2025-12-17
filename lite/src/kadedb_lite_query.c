#include "kadedb_lite/kadedb_lite_query.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

typedef struct token_t {
  char *value;
  size_t length;
} token_t;

typedef struct tokenizer_t {
  const char *input;
  size_t pos;
  size_t len;
} tokenizer_t;

static void tokenizer_init(tokenizer_t *t, const char *input) {
  t->input = input;
  t->pos = 0;
  t->len = input ? strlen(input) : 0;
}

static void skip_whitespace(tokenizer_t *t) {
  while (t->pos < t->len && isspace((unsigned char)t->input[t->pos])) {
    t->pos++;
  }
}

static int is_identifier_char(char c) {
  return isalnum((unsigned char)c) || c == '_';
}

static char *read_identifier(tokenizer_t *t) {
  skip_whitespace(t);
  if (t->pos >= t->len)
    return NULL;

  size_t start = t->pos;
  while (t->pos < t->len && is_identifier_char(t->input[t->pos])) {
    t->pos++;
  }

  if (t->pos == start)
    return NULL;

  size_t len = t->pos - start;
  char *id = (char *)malloc(len + 1);
  if (!id)
    return NULL;
  memcpy(id, t->input + start, len);
  id[len] = '\0';
  return id;
}

static char *read_string_literal(tokenizer_t *t) {
  skip_whitespace(t);
  if (t->pos >= t->len)
    return NULL;

  char quote = t->input[t->pos];
  if (quote != '\'' && quote != '"')
    return NULL;

  t->pos++;
  size_t start = t->pos;

  while (t->pos < t->len && t->input[t->pos] != quote) {
    if (t->input[t->pos] == '\\' && t->pos + 1 < t->len) {
      t->pos += 2;
    } else {
      t->pos++;
    }
  }

  if (t->pos >= t->len)
    return NULL;

  size_t len = t->pos - start;
  char *str = (char *)malloc(len + 1);
  if (!str)
    return NULL;
  memcpy(str, t->input + start, len);
  str[len] = '\0';
  t->pos++;
  return str;
}

static char *read_value(tokenizer_t *t) {
  skip_whitespace(t);
  if (t->pos >= t->len)
    return NULL;

  if (t->input[t->pos] == '\'' || t->input[t->pos] == '"') {
    return read_string_literal(t);
  }

  return read_identifier(t);
}

static int expect_char(tokenizer_t *t, char c) {
  skip_whitespace(t);
  if (t->pos < t->len && t->input[t->pos] == c) {
    t->pos++;
    return 1;
  }
  return 0;
}

static int str_case_eq(const char *a, const char *b) {
  if (!a || !b)
    return 0;
  while (*a && *b) {
    if (tolower((unsigned char)*a) != tolower((unsigned char)*b))
      return 0;
    a++;
    b++;
  }
  return *a == '\0' && *b == '\0';
}

static void free_string_array(char **arr, size_t count) {
  if (!arr)
    return;
  for (size_t i = 0; i < count; i++) {
    free(arr[i]);
  }
  free(arr);
}

static kadedb_lite_condition_op_t parse_operator(tokenizer_t *t) {
  skip_whitespace(t);
  if (t->pos >= t->len)
    return KADEDB_LITE_OP_EQ;

  if (t->pos + 1 < t->len) {
    if (t->input[t->pos] == '!' && t->input[t->pos + 1] == '=') {
      t->pos += 2;
      return KADEDB_LITE_OP_NE;
    }
    if (t->input[t->pos] == '<' && t->input[t->pos + 1] == '=') {
      t->pos += 2;
      return KADEDB_LITE_OP_LE;
    }
    if (t->input[t->pos] == '>' && t->input[t->pos + 1] == '=') {
      t->pos += 2;
      return KADEDB_LITE_OP_GE;
    }
    if (t->input[t->pos] == '<' && t->input[t->pos + 1] == '>') {
      t->pos += 2;
      return KADEDB_LITE_OP_NE;
    }
  }

  if (t->input[t->pos] == '=') {
    t->pos++;
    return KADEDB_LITE_OP_EQ;
  }
  if (t->input[t->pos] == '<') {
    t->pos++;
    return KADEDB_LITE_OP_LT;
  }
  if (t->input[t->pos] == '>') {
    t->pos++;
    return KADEDB_LITE_OP_GT;
  }

  return KADEDB_LITE_OP_EQ;
}

static kadedb_lite_condition_t *parse_condition(tokenizer_t *t) {
  char *column = read_identifier(t);
  if (!column)
    return NULL;

  kadedb_lite_condition_op_t op = parse_operator(t);

  char *value = read_value(t);
  if (!value) {
    free(column);
    return NULL;
  }

  kadedb_lite_condition_t *cond =
      (kadedb_lite_condition_t *)malloc(sizeof(kadedb_lite_condition_t));
  if (!cond) {
    free(column);
    free(value);
    return NULL;
  }

  cond->column = column;
  cond->op = op;
  cond->value = value;
  return cond;
}

static kadedb_lite_parsed_query_t *parse_select(tokenizer_t *t) {
  kadedb_lite_parsed_query_t *q = (kadedb_lite_parsed_query_t *)calloc(
      1, sizeof(kadedb_lite_parsed_query_t));
  if (!q)
    return NULL;

  q->type = KADEDB_LITE_QUERY_SELECT;

  size_t col_cap = 8;
  q->columns = (char **)malloc(col_cap * sizeof(char *));
  if (!q->columns) {
    free(q);
    return NULL;
  }
  q->column_count = 0;

  skip_whitespace(t);
  if (t->pos < t->len && t->input[t->pos] == '*') {
    t->pos++;
    char *star = (char *)malloc(2);
    if (!star) {
      free(q->columns);
      free(q);
      return NULL;
    }
    star[0] = '*';
    star[1] = '\0';
    q->columns[0] = star;
    q->column_count = 1;
  } else {
    while (1) {
      char *col = read_identifier(t);
      if (!col)
        break;

      if (q->column_count >= col_cap) {
        col_cap *= 2;
        char **new_cols =
            (char **)realloc(q->columns, col_cap * sizeof(char *));
        if (!new_cols) {
          free(col);
          break;
        }
        q->columns = new_cols;
      }
      q->columns[q->column_count++] = col;

      if (!expect_char(t, ','))
        break;
    }
  }

  char *from_kw = read_identifier(t);
  if (!from_kw || !str_case_eq(from_kw, "FROM")) {
    free(from_kw);
    free_string_array(q->columns, q->column_count);
    free(q);
    return NULL;
  }
  free(from_kw);

  q->table = read_identifier(t);
  if (!q->table) {
    free_string_array(q->columns, q->column_count);
    free(q);
    return NULL;
  }

  char *where_kw = read_identifier(t);
  if (where_kw && str_case_eq(where_kw, "WHERE")) {
    free(where_kw);
    q->condition = parse_condition(t);
  } else {
    free(where_kw);
  }

  return q;
}

static kadedb_lite_parsed_query_t *parse_insert(tokenizer_t *t) {
  char *into_kw = read_identifier(t);
  if (!into_kw || !str_case_eq(into_kw, "INTO")) {
    free(into_kw);
    return NULL;
  }
  free(into_kw);

  kadedb_lite_parsed_query_t *q = (kadedb_lite_parsed_query_t *)calloc(
      1, sizeof(kadedb_lite_parsed_query_t));
  if (!q)
    return NULL;

  q->type = KADEDB_LITE_QUERY_INSERT;

  q->table = read_identifier(t);
  if (!q->table) {
    free(q);
    return NULL;
  }

  if (!expect_char(t, '(')) {
    free(q->table);
    free(q);
    return NULL;
  }

  size_t col_cap = 8;
  q->columns = (char **)malloc(col_cap * sizeof(char *));
  if (!q->columns) {
    free(q->table);
    free(q);
    return NULL;
  }
  q->column_count = 0;

  while (1) {
    char *col = read_identifier(t);
    if (!col)
      break;

    if (q->column_count >= col_cap) {
      col_cap *= 2;
      char **new_cols = (char **)realloc(q->columns, col_cap * sizeof(char *));
      if (!new_cols) {
        free(col);
        break;
      }
      q->columns = new_cols;
    }
    q->columns[q->column_count++] = col;

    if (!expect_char(t, ','))
      break;
  }

  if (!expect_char(t, ')')) {
    free_string_array(q->columns, q->column_count);
    free(q->table);
    free(q);
    return NULL;
  }

  char *values_kw = read_identifier(t);
  if (!values_kw || !str_case_eq(values_kw, "VALUES")) {
    free(values_kw);
    free_string_array(q->columns, q->column_count);
    free(q->table);
    free(q);
    return NULL;
  }
  free(values_kw);

  if (!expect_char(t, '(')) {
    free_string_array(q->columns, q->column_count);
    free(q->table);
    free(q);
    return NULL;
  }

  size_t val_cap = 8;
  q->values = (char **)malloc(val_cap * sizeof(char *));
  if (!q->values) {
    free_string_array(q->columns, q->column_count);
    free(q->table);
    free(q);
    return NULL;
  }
  q->value_count = 0;

  while (1) {
    char *val = read_value(t);
    if (!val)
      break;

    if (q->value_count >= val_cap) {
      val_cap *= 2;
      char **new_vals = (char **)realloc(q->values, val_cap * sizeof(char *));
      if (!new_vals) {
        free(val);
        break;
      }
      q->values = new_vals;
    }
    q->values[q->value_count++] = val;

    if (!expect_char(t, ','))
      break;
  }

  if (!expect_char(t, ')')) {
    free_string_array(q->values, q->value_count);
    free_string_array(q->columns, q->column_count);
    free(q->table);
    free(q);
    return NULL;
  }

  return q;
}

kadedb_lite_parsed_query_t *kadedb_lite_parse_query(const char *query) {
  if (!query)
    return NULL;

  tokenizer_t t;
  tokenizer_init(&t, query);

  char *keyword = read_identifier(&t);
  if (!keyword)
    return NULL;

  kadedb_lite_parsed_query_t *result = NULL;

  if (str_case_eq(keyword, "SELECT")) {
    result = parse_select(&t);
  } else if (str_case_eq(keyword, "INSERT")) {
    result = parse_insert(&t);
  }

  free(keyword);
  return result;
}

void kadedb_lite_parsed_query_free(kadedb_lite_parsed_query_t *parsed) {
  if (!parsed)
    return;

  free(parsed->table);
  free_string_array(parsed->columns, parsed->column_count);
  free_string_array(parsed->values, parsed->value_count);

  if (parsed->condition) {
    free(parsed->condition->column);
    free(parsed->condition->value);
    free(parsed->condition);
  }

  free(parsed);
}

static kadedb_lite_result_t *create_result(void) {
  kadedb_lite_result_t *r =
      (kadedb_lite_result_t *)calloc(1, sizeof(kadedb_lite_result_t));
  return r;
}

static kadedb_lite_result_t *create_error_result(const char *msg) {
  kadedb_lite_result_t *r = create_result();
  if (!r)
    return NULL;
  if (msg) {
    r->error_message = (char *)malloc(strlen(msg) + 1);
    if (r->error_message) {
      strcpy(r->error_message, msg);
    }
  }
  return r;
}

static char *build_key(const char *table, const char *id) {
  size_t tlen = strlen(table);
  size_t ilen = strlen(id);
  char *key = (char *)malloc(tlen + 1 + ilen + 1);
  if (!key)
    return NULL;
  memcpy(key, table, tlen);
  key[tlen] = ':';
  memcpy(key + tlen + 1, id, ilen);
  key[tlen + 1 + ilen] = '\0';
  return key;
}

static kadedb_lite_result_t *
execute_select(kadedb_lite_t *db, kadedb_lite_parsed_query_t *parsed) {
  if (!parsed->condition) {
    return create_error_result(
        "SELECT without WHERE clause not supported in Lite");
  }

  if (!str_case_eq(parsed->condition->column, "id") &&
      !str_case_eq(parsed->condition->column, "key")) {
    return create_error_result(
        "SELECT condition must be on 'id' or 'key' column");
  }

  if (parsed->condition->op != KADEDB_LITE_OP_EQ) {
    return create_error_result("Only equality conditions supported");
  }

  char *key = build_key(parsed->table, parsed->condition->value);
  if (!key) {
    return create_error_result("Memory allocation failed");
  }

  char *value = NULL;
  size_t value_len = 0;
  int rc = kadedb_lite_get(db, key, &value, &value_len);
  free(key);

  kadedb_lite_result_t *result = create_result();
  if (!result) {
    if (value)
      kadedb_lite_free(value);
    return NULL;
  }

  if (rc != 0 || !value) {
    result->row_count = 0;
    result->column_count = 2;
    result->column_names = (char **)malloc(2 * sizeof(char *));
    if (result->column_names) {
      result->column_names[0] = (char *)malloc(3);
      result->column_names[1] = (char *)malloc(6);
      if (result->column_names[0])
        strcpy(result->column_names[0], "id");
      if (result->column_names[1])
        strcpy(result->column_names[1], "value");
    }
    return result;
  }

  result->column_count = 2;
  result->column_names = (char **)malloc(2 * sizeof(char *));
  if (!result->column_names) {
    kadedb_lite_free(value);
    kadedb_lite_result_free(result);
    return create_error_result("Memory allocation failed");
  }
  result->column_names[0] = (char *)malloc(3);
  result->column_names[1] = (char *)malloc(6);
  if (result->column_names[0])
    strcpy(result->column_names[0], "id");
  if (result->column_names[1])
    strcpy(result->column_names[1], "value");

  result->row_count = 1;
  result->rows = (kadedb_lite_row_t *)malloc(sizeof(kadedb_lite_row_t));
  if (!result->rows) {
    kadedb_lite_free(value);
    kadedb_lite_result_free(result);
    return create_error_result("Memory allocation failed");
  }

  result->rows[0].value_count = 2;
  result->rows[0].values = (char **)malloc(2 * sizeof(char *));
  if (!result->rows[0].values) {
    kadedb_lite_free(value);
    kadedb_lite_result_free(result);
    return create_error_result("Memory allocation failed");
  }

  size_t id_len = strlen(parsed->condition->value);
  result->rows[0].values[0] = (char *)malloc(id_len + 1);
  if (result->rows[0].values[0]) {
    strcpy(result->rows[0].values[0], parsed->condition->value);
  }

  result->rows[0].values[1] = (char *)malloc(value_len + 1);
  if (result->rows[0].values[1]) {
    memcpy(result->rows[0].values[1], value, value_len);
    result->rows[0].values[1][value_len] = '\0';
  }

  kadedb_lite_free(value);
  return result;
}

static kadedb_lite_result_t *
execute_insert(kadedb_lite_t *db, kadedb_lite_parsed_query_t *parsed) {
  if (parsed->column_count < 2 || parsed->value_count < 2) {
    return create_error_result(
        "INSERT requires at least 'id' and 'value' columns");
  }

  int id_idx = -1;
  int value_idx = -1;

  for (size_t i = 0; i < parsed->column_count; i++) {
    if (str_case_eq(parsed->columns[i], "id") ||
        str_case_eq(parsed->columns[i], "key")) {
      id_idx = (int)i;
    } else if (str_case_eq(parsed->columns[i], "value") ||
               str_case_eq(parsed->columns[i], "data")) {
      value_idx = (int)i;
    }
  }

  if (id_idx < 0 || value_idx < 0) {
    return create_error_result("INSERT must include 'id' and 'value' columns");
  }

  if ((size_t)id_idx >= parsed->value_count ||
      (size_t)value_idx >= parsed->value_count) {
    return create_error_result("Column/value count mismatch");
  }

  char *key = build_key(parsed->table, parsed->values[id_idx]);
  if (!key) {
    return create_error_result("Memory allocation failed");
  }

  const char *val = parsed->values[value_idx];
  size_t val_len = strlen(val);

  int rc = kadedb_lite_put(db, key, val, val_len);
  free(key);

  if (rc != 0) {
    return create_error_result("Failed to insert data");
  }

  kadedb_lite_result_t *result = create_result();
  if (!result)
    return NULL;

  result->affected_rows = 1;
  return result;
}

kadedb_lite_result_t *kadedb_lite_execute_query(kadedb_lite_t *db,
                                                const char *query) {
  if (!db || !query) {
    return create_error_result("Invalid arguments");
  }

  kadedb_lite_parsed_query_t *parsed = kadedb_lite_parse_query(query);
  if (!parsed) {
    return create_error_result("Failed to parse query");
  }

  kadedb_lite_result_t *result = NULL;

  switch (parsed->type) {
  case KADEDB_LITE_QUERY_SELECT:
    result = execute_select(db, parsed);
    break;
  case KADEDB_LITE_QUERY_INSERT:
    result = execute_insert(db, parsed);
    break;
  default:
    result = create_error_result("Unsupported query type");
    break;
  }

  kadedb_lite_parsed_query_free(parsed);
  return result;
}

void kadedb_lite_result_free(kadedb_lite_result_t *result) {
  if (!result)
    return;

  free_string_array(result->column_names, result->column_count);

  if (result->rows) {
    for (size_t i = 0; i < result->row_count; i++) {
      free_string_array(result->rows[i].values, result->rows[i].value_count);
    }
    free(result->rows);
  }

  free(result->error_message);
  free(result);
}

const char *kadedb_lite_result_error(const kadedb_lite_result_t *result) {
  if (!result)
    return NULL;
  return result->error_message;
}

size_t kadedb_lite_result_row_count(const kadedb_lite_result_t *result) {
  if (!result)
    return 0;
  return result->row_count;
}

size_t kadedb_lite_result_column_count(const kadedb_lite_result_t *result) {
  if (!result)
    return 0;
  return result->column_count;
}

const char *kadedb_lite_result_column_name(const kadedb_lite_result_t *result,
                                           size_t col_index) {
  if (!result || !result->column_names || col_index >= result->column_count)
    return NULL;
  return result->column_names[col_index];
}

const char *kadedb_lite_result_value(const kadedb_lite_result_t *result,
                                     size_t row_index, size_t col_index) {
  if (!result || !result->rows || row_index >= result->row_count)
    return NULL;
  if (!result->rows[row_index].values ||
      col_index >= result->rows[row_index].value_count)
    return NULL;
  return result->rows[row_index].values[col_index];
}

int kadedb_lite_result_affected_rows(const kadedb_lite_result_t *result) {
  if (!result)
    return 0;
  return result->affected_rows;
}
