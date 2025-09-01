#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kadedb_lite/kadedb_lite.h"

int main(void) {
  const char *path = "./tmp_lite_db"; // RocksDB will create it if needed
  const char *key = "hello";
  const char *val = "world";

  kadedb_lite_t *db = kadedb_lite_open(path);
  if (!db) {
    fprintf(stderr, "kadedb_lite_open failed\n");
    return 1;
  }

  int rc = kadedb_lite_put(db, key, val, strlen(val));
  if (rc != 0) {
    fprintf(stderr, "kadedb_lite_put failed\n");
    kadedb_lite_close(db);
    return 2;
  }

  char *out = NULL;
  size_t out_len = 0;
  rc = kadedb_lite_get(db, key, &out, &out_len);
  if (rc != 0 || !out) {
    fprintf(stderr, "kadedb_lite_get failed\n");
    kadedb_lite_close(db);
    return 3;
  }

  // Accept either stub or round-trip value depending on build flags
  if (strcmp(out, "stub") == 0) {
    printf("Lite smoke: stub mode value = %s\n", out);
  } else if (strcmp(out, val) == 0) {
    printf("Lite smoke: rocksdb mode value = %s\n", out);
  } else {
    fprintf(stderr, "Unexpected value: %s\n", out);
    kadedb_lite_free(out);
    kadedb_lite_close(db);
    return 4;
  }

  kadedb_lite_free(out);
  kadedb_lite_close(db);
  return 0;
}
