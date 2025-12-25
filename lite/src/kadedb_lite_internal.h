#ifndef KADEDB_LITE_INTERNAL_H
#define KADEDB_LITE_INTERNAL_H

#include "kadedb_lite/kadedb_lite.h"

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

  int sync_initialized;
  int sync_running;
  int sync_interval_seconds;
  char *sync_remote_url;
  char *sync_auth_token;
};

#endif // KADEDB_LITE_INTERNAL_H
