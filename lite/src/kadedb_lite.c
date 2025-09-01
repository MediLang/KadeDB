#include "kadedb_lite/kadedb_lite.h"
#include <stdlib.h>
#include <string.h>

#ifdef KADEDB_LITE_HAS_ROCKSDB
#include <rocksdb/c.h>
#endif

struct kadedb_lite_t {
#ifdef KADEDB_LITE_HAS_ROCKSDB
  rocksdb_t *db;
  rocksdb_options_t *options;
  rocksdb_readoptions_t *ropts;
  rocksdb_writeoptions_t *wopts;
#else
  int placeholder;
#endif
};

kadedb_lite_t *kadedb_lite_open(const char *path) {
#ifdef KADEDB_LITE_HAS_ROCKSDB
  char *err = NULL;
  kadedb_lite_t *h = (kadedb_lite_t *)calloc(1, sizeof(kadedb_lite_t));
  if (!h)
    return NULL;
  h->options = rocksdb_options_create();
  if (!h->options) {
    free(h);
    return NULL;
  }
  rocksdb_options_set_create_if_missing(h->options, 1);
  h->db = rocksdb_open(h->options, path, &err);
  if (err != NULL || h->db == NULL) {
    if (err) {
      rocksdb_free(err);
    }
    rocksdb_options_destroy(h->options);
    free(h);
    return NULL;
  }
  h->ropts = rocksdb_readoptions_create();
  h->wopts = rocksdb_writeoptions_create();
  if (!h->ropts || !h->wopts) {
    if (h->ropts)
      rocksdb_readoptions_destroy(h->ropts);
    if (h->wopts)
      rocksdb_writeoptions_destroy(h->wopts);
    rocksdb_close(h->db);
    rocksdb_options_destroy(h->options);
    free(h);
    return NULL;
  }
  return h;
#else
  (void)path; // unused for stub
  kadedb_lite_t *db = (kadedb_lite_t *)malloc(sizeof(kadedb_lite_t));
  if (db)
    db->placeholder = 0;
  return db;
#endif
}

void kadedb_lite_close(kadedb_lite_t *db) {
  if (!db)
    return;
#ifdef KADEDB_LITE_HAS_ROCKSDB
  if (db->ropts)
    rocksdb_readoptions_destroy(db->ropts);
  if (db->wopts)
    rocksdb_writeoptions_destroy(db->wopts);
  if (db->db)
    rocksdb_close(db->db);
  if (db->options)
    rocksdb_options_destroy(db->options);
  free(db);
#else
  free(db);
#endif
}

int kadedb_lite_put(kadedb_lite_t *db, const char *key, const char *value,
                    size_t value_len) {
#ifdef KADEDB_LITE_HAS_ROCKSDB
  if (!db || !db->db || !key || !value)
    return -1;
  char *err = NULL;
  rocksdb_put(db->db, db->wopts, key, strlen(key), value, value_len, &err);
  if (err != NULL) {
    rocksdb_free(err);
    return -1;
  }
  return 0;
#else
  (void)db;
  (void)key;
  (void)value;
  (void)value_len;
  // Stub success
  return 0;
#endif
}

int kadedb_lite_get(kadedb_lite_t *db, const char *key, char **value_out,
                    size_t *value_len_out) {
#ifdef KADEDB_LITE_HAS_ROCKSDB
  if (!db || !db->db || !key || !value_out)
    return -1;
  char *err = NULL;
  size_t outlen = 0;
  char *val = rocksdb_get(db->db, db->ropts, key, strlen(key), &outlen, &err);
  if (err != NULL) {
    rocksdb_free(err);
    return -1;
  }
  if (val == NULL) {
    // Not found
    return -1;
  }
  // Allocate using malloc so caller can free via kadedb_lite_free()
  char *buf = (char *)malloc(outlen + 1);
  if (!buf) {
    rocksdb_free(val);
    return -1;
  }
  memcpy(buf, val, outlen);
  buf[outlen] = '\0';
  rocksdb_free(val);
  *value_out = buf;
  if (value_len_out)
    *value_len_out = outlen;
  return 0;
#else
  (void)db;
  (void)key;
  const char *msg = "stub";
  size_t n = strlen(msg);
  char *buf = (char *)malloc(n + 1);
  if (!buf)
    return -1;
  memcpy(buf, msg, n + 1);
  *value_out = buf;
  if (value_len_out)
    *value_len_out = n;
  return 0;
#endif
}

void kadedb_lite_free(char *p) {
#ifdef KADEDB_LITE_HAS_ROCKSDB
  // We allocate with malloc before returning to callers; free with free()
  free(p);
#else
  free(p);
#endif
}
