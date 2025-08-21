#include "kadedb_lite/kadedb_lite.h"
#include <stdlib.h>
#include <string.h>

struct kadedb_lite_t {
  int placeholder;
};

kadedb_lite_t* kadedb_lite_open(const char* path) {
  (void)path; // unused for stub
  kadedb_lite_t* db = (kadedb_lite_t*)malloc(sizeof(kadedb_lite_t));
  if (db) db->placeholder = 0;
  return db;
}

void kadedb_lite_close(kadedb_lite_t* db) {
  if (db) free(db);
}

int kadedb_lite_put(kadedb_lite_t* db, const char* key, const char* value, size_t value_len) {
  (void)db; (void)key; (void)value; (void)value_len;
  // Stub success
  return 0;
}

int kadedb_lite_get(kadedb_lite_t* db, const char* key, char** value_out, size_t* value_len_out) {
  (void)db; (void)key;
  const char* msg = "stub";
  size_t n = strlen(msg);
  char* buf = (char*)malloc(n + 1);
  if (!buf) return -1;
  memcpy(buf, msg, n + 1);
  *value_out = buf;
  if (value_len_out) *value_len_out = n;
  return 0;
}

void kadedb_lite_free(char* p) {
  free(p);
}
