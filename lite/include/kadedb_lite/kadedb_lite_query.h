#ifndef KADEDB_LITE_QUERY_H
#define KADEDB_LITE_QUERY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include "kadedb_lite.h"

typedef enum kadedb_lite_query_type_t {
  KADEDB_LITE_QUERY_SELECT = 0,
  KADEDB_LITE_QUERY_INSERT = 1,
  KADEDB_LITE_QUERY_UNKNOWN = -1
} kadedb_lite_query_type_t;

typedef enum kadedb_lite_condition_op_t {
  KADEDB_LITE_OP_EQ = 0,
  KADEDB_LITE_OP_NE = 1,
  KADEDB_LITE_OP_LT = 2,
  KADEDB_LITE_OP_LE = 3,
  KADEDB_LITE_OP_GT = 4,
  KADEDB_LITE_OP_GE = 5
} kadedb_lite_condition_op_t;

typedef struct kadedb_lite_condition_t {
  char *column;
  kadedb_lite_condition_op_t op;
  char *value;
} kadedb_lite_condition_t;

typedef struct kadedb_lite_parsed_query_t {
  kadedb_lite_query_type_t type;
  char *table;
  char **columns;
  size_t column_count;
  char **values;
  size_t value_count;
  kadedb_lite_condition_t *condition;
} kadedb_lite_parsed_query_t;

typedef struct kadedb_lite_row_t {
  char **values;
  size_t value_count;
} kadedb_lite_row_t;

typedef struct kadedb_lite_result_t {
  char **column_names;
  size_t column_count;
  kadedb_lite_row_t *rows;
  size_t row_count;
  int affected_rows;
  char *error_message;
} kadedb_lite_result_t;

kadedb_lite_parsed_query_t *kadedb_lite_parse_query(const char *query);

void kadedb_lite_parsed_query_free(kadedb_lite_parsed_query_t *parsed);

kadedb_lite_result_t *kadedb_lite_execute_query(kadedb_lite_t *db,
                                                const char *query);

void kadedb_lite_result_free(kadedb_lite_result_t *result);

const char *kadedb_lite_result_error(const kadedb_lite_result_t *result);

size_t kadedb_lite_result_row_count(const kadedb_lite_result_t *result);

size_t kadedb_lite_result_column_count(const kadedb_lite_result_t *result);

const char *kadedb_lite_result_column_name(const kadedb_lite_result_t *result,
                                           size_t col_index);

const char *kadedb_lite_result_value(const kadedb_lite_result_t *result,
                                     size_t row_index, size_t col_index);

int kadedb_lite_result_affected_rows(const kadedb_lite_result_t *result);

#ifdef __cplusplus
}
#endif

#endif // KADEDB_LITE_QUERY_H
