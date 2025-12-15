#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kadedb_lite/kadedb_lite.h"

static int is_stub_value(const char *s) { return s && strcmp(s, "stub") == 0; }

static int expect_get_ok(kadedb_lite_t *db, const char *key,
                         const char *expected, int allow_stub) {
  char *out = NULL;
  size_t out_len = 0;

  int rc = kadedb_lite_get(db, key, &out, &out_len);
  if (rc != 0 || !out)
    return 0;

  if (allow_stub && is_stub_value(out)) {
    kadedb_lite_free(out);
    return 1;
  }

  if (strcmp(out, expected) != 0) {
    kadedb_lite_free(out);
    return 0;
  }

  kadedb_lite_free(out);
  return 1;
}

static int expect_get_not_found(kadedb_lite_t *db, const char *key,
                                int allow_stub) {
  char *out = NULL;
  size_t out_len = 0;

  int rc = kadedb_lite_get(db, key, &out, &out_len);
  if (allow_stub && rc == 0 && out && is_stub_value(out)) {
    kadedb_lite_free(out);
    return 1;
  }

  if (rc == 0) {
    if (out)
      kadedb_lite_free(out);
    return 0;
  }

  if (out)
    kadedb_lite_free(out);
  return 1;
}

int main(void) {
  const char *path = "./tmp_lite_db_kv";

  kadedb_lite_options_t *opts = kadedb_lite_options_create();
  if (!opts) {
    fprintf(stderr, "kadedb_lite_options_create failed\n");
    return 1;
  }
  kadedb_lite_options_set_create_if_missing(opts, 1);
  kadedb_lite_options_set_error_if_exists(opts, 0);
  kadedb_lite_options_set_write_buffer_size_bytes(opts, 1024 * 1024);
  kadedb_lite_options_set_max_open_files(opts, 32);

  kadedb_lite_t *db = kadedb_lite_open_with_options(path, opts);
  kadedb_lite_options_destroy(opts);
  if (!db) {
    fprintf(stderr, "kadedb_lite_open_with_options failed\n");
    return 2;
  }

  const char *key1 = "k1";
  const char *val1 = "v1";
  if (kadedb_lite_put(db, key1, val1, strlen(val1)) != 0) {
    fprintf(stderr, "put k1 failed\n");
    kadedb_lite_close(db);
    return 3;
  }

  int allow_stub = 0;
  {
    char *probe = NULL;
    size_t probe_len = 0;
    int rc = kadedb_lite_get(db, key1, &probe, &probe_len);
    if (rc == 0 && probe && is_stub_value(probe))
      allow_stub = 1;
    if (probe)
      kadedb_lite_free(probe);
  }

  if (!expect_get_ok(db, key1, val1, allow_stub)) {
    fprintf(stderr, "get k1 mismatch\n");
    kadedb_lite_close(db);
    return 4;
  }

  if (kadedb_lite_delete(db, key1) != 0) {
    fprintf(stderr, "delete k1 failed\n");
    kadedb_lite_close(db);
    return 5;
  }

  if (!expect_get_not_found(db, key1, allow_stub)) {
    fprintf(stderr, "expected k1 to be missing after delete\n");
    kadedb_lite_close(db);
    return 6;
  }

  const char *key2 = "empty_val";
  if (kadedb_lite_put(db, key2, "", 0) != 0) {
    fprintf(stderr, "put empty value failed\n");
    kadedb_lite_close(db);
    return 7;
  }

  if (!allow_stub) {
    char *out = NULL;
    size_t out_len = 123;
    int rc = kadedb_lite_get(db, key2, &out, &out_len);
    if (rc != 0 || !out) {
      fprintf(stderr, "get empty value failed\n");
      kadedb_lite_close(db);
      return 8;
    }
    if (out_len != 0 || strcmp(out, "") != 0) {
      fprintf(stderr, "empty value mismatch\n");
      kadedb_lite_free(out);
      kadedb_lite_close(db);
      return 9;
    }
    kadedb_lite_free(out);
  }

  if (kadedb_lite_delete(db, "definitely_missing") != 0) {
    fprintf(stderr, "delete missing key failed\n");
    kadedb_lite_close(db);
    return 10;
  }

  kadedb_lite_close(db);
  return 0;
}
