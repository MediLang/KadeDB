#include "kadedb_lite/kadedb_lite.h"
#include <stdlib.h>
#include <string.h>

#ifdef KADEDB_LITE_HAS_ROCKSDB
#include <rocksdb/c.h>
#endif

struct kadedb_lite_options_t {
  int create_if_missing;
  int error_if_exists;
  kadedb_lite_compression_t compression;
  size_t cache_size_bytes;
  size_t write_buffer_size_bytes;
  int max_open_files;
};

struct kadedb_lite_t {
#ifdef KADEDB_LITE_HAS_ROCKSDB
  rocksdb_t *db;
  rocksdb_options_t *options;
  rocksdb_readoptions_t *ropts;
  rocksdb_writeoptions_t *wopts;
  rocksdb_cache_t *cache;
  rocksdb_block_based_table_options_t *bbt_opts;
#else
  int placeholder;
#endif
};

kadedb_lite_options_t *kadedb_lite_options_create(void) {
  kadedb_lite_options_t *opts =
      (kadedb_lite_options_t *)calloc(1, sizeof(*opts));
  if (!opts)
    return NULL;
  opts->create_if_missing = 1;
  opts->error_if_exists = 0;
  opts->compression = KADEDB_LITE_COMPRESSION_NONE;
  opts->cache_size_bytes = 0;
  opts->write_buffer_size_bytes = 0;
  opts->max_open_files = 0;
  return opts;
}

void kadedb_lite_options_destroy(kadedb_lite_options_t *opts) { free(opts); }

void kadedb_lite_options_set_create_if_missing(kadedb_lite_options_t *opts,
                                               int create_if_missing) {
  if (!opts)
    return;
  opts->create_if_missing = create_if_missing ? 1 : 0;
}

void kadedb_lite_options_set_error_if_exists(kadedb_lite_options_t *opts,
                                             int error_if_exists) {
  if (!opts)
    return;
  opts->error_if_exists = error_if_exists ? 1 : 0;
}

void kadedb_lite_options_set_compression(
    kadedb_lite_options_t *opts, kadedb_lite_compression_t compression) {
  if (!opts)
    return;
  opts->compression = compression;
}

void kadedb_lite_options_set_cache_size_bytes(kadedb_lite_options_t *opts,
                                              size_t cache_size_bytes) {
  if (!opts)
    return;
  opts->cache_size_bytes = cache_size_bytes;
}

void kadedb_lite_options_set_write_buffer_size_bytes(
    kadedb_lite_options_t *opts, size_t write_buffer_size_bytes) {
  if (!opts)
    return;
  opts->write_buffer_size_bytes = write_buffer_size_bytes;
}

void kadedb_lite_options_set_max_open_files(kadedb_lite_options_t *opts,
                                            int max_open_files) {
  if (!opts)
    return;
  opts->max_open_files = max_open_files;
}

#ifdef KADEDB_LITE_HAS_ROCKSDB
static rocksdb_compression_type
kadedb_lite_map_compression(kadedb_lite_compression_t compression) {
  switch (compression) {
  case KADEDB_LITE_COMPRESSION_NONE:
    return rocksdb_no_compression;
  case KADEDB_LITE_COMPRESSION_SNAPPY:
    return rocksdb_snappy_compression;
  case KADEDB_LITE_COMPRESSION_ZLIB:
    return rocksdb_zlib_compression;
  case KADEDB_LITE_COMPRESSION_BZ2:
    return rocksdb_bz2_compression;
  case KADEDB_LITE_COMPRESSION_LZ4:
    return rocksdb_lz4_compression;
  case KADEDB_LITE_COMPRESSION_LZ4HC:
    return rocksdb_lz4hc_compression;
  case KADEDB_LITE_COMPRESSION_ZSTD:
    return rocksdb_zstd_compression;
  default:
    return rocksdb_no_compression;
  }
}
#endif

kadedb_lite_t *kadedb_lite_open(const char *path) {
  return kadedb_lite_open_with_options(path, NULL);
}

kadedb_lite_t *
kadedb_lite_open_with_options(const char *path,
                              const kadedb_lite_options_t *opts) {
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
  if (opts) {
    rocksdb_options_set_create_if_missing(h->options, opts->create_if_missing);
    rocksdb_options_set_error_if_exists(h->options, opts->error_if_exists);
    rocksdb_options_set_compression(
        h->options, kadedb_lite_map_compression(opts->compression));
    if (opts->write_buffer_size_bytes > 0) {
      rocksdb_options_set_write_buffer_size(h->options,
                                            opts->write_buffer_size_bytes);
    }
    if (opts->max_open_files != 0) {
      rocksdb_options_set_max_open_files(h->options, opts->max_open_files);
    }
    if (opts->cache_size_bytes > 0) {
      h->cache = rocksdb_cache_create_lru(opts->cache_size_bytes);
      if (!h->cache) {
        rocksdb_options_destroy(h->options);
        free(h);
        return NULL;
      }
      h->bbt_opts = rocksdb_block_based_options_create();
      if (!h->bbt_opts) {
        rocksdb_cache_destroy(h->cache);
        rocksdb_options_destroy(h->options);
        free(h);
        return NULL;
      }
      rocksdb_block_based_options_set_block_cache(h->bbt_opts, h->cache);
      rocksdb_options_set_block_based_table_factory(h->options, h->bbt_opts);
    }
  } else {
    rocksdb_options_set_create_if_missing(h->options, 1);
  }
  h->db = rocksdb_open(h->options, path, &err);
  if (err != NULL || h->db == NULL) {
    if (err) {
      rocksdb_free(err);
    }
    if (h->bbt_opts)
      rocksdb_block_based_options_destroy(h->bbt_opts);
    if (h->cache)
      rocksdb_cache_destroy(h->cache);
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
    if (h->bbt_opts)
      rocksdb_block_based_options_destroy(h->bbt_opts);
    if (h->cache)
      rocksdb_cache_destroy(h->cache);
    rocksdb_options_destroy(h->options);
    free(h);
    return NULL;
  }
  return h;
#else
  (void)opts;
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
  if (db->bbt_opts)
    rocksdb_block_based_options_destroy(db->bbt_opts);
  if (db->cache)
    rocksdb_cache_destroy(db->cache);
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

int kadedb_lite_delete(kadedb_lite_t *db, const char *key) {
#ifdef KADEDB_LITE_HAS_ROCKSDB
  if (!db || !db->db || !key)
    return -1;
  char *err = NULL;
  rocksdb_delete(db->db, db->wopts, key, strlen(key), &err);
  if (err != NULL) {
    rocksdb_free(err);
    return -1;
  }
  return 0;
#else
  (void)db;
  (void)key;
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
