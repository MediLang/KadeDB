#ifndef KADEDB_LITE_H
#define KADEDB_LITE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

typedef struct kadedb_lite_t kadedb_lite_t;

typedef struct kadedb_lite_options_t kadedb_lite_options_t;

typedef enum kadedb_lite_compression_t {
  KADEDB_LITE_COMPRESSION_NONE = 0,
  KADEDB_LITE_COMPRESSION_SNAPPY = 1,
  KADEDB_LITE_COMPRESSION_ZLIB = 2,
  KADEDB_LITE_COMPRESSION_BZ2 = 3,
  KADEDB_LITE_COMPRESSION_LZ4 = 4,
  KADEDB_LITE_COMPRESSION_LZ4HC = 5,
  KADEDB_LITE_COMPRESSION_ZSTD = 6
} kadedb_lite_compression_t;

kadedb_lite_options_t *kadedb_lite_options_create(void);
void kadedb_lite_options_destroy(kadedb_lite_options_t *opts);

void kadedb_lite_options_set_create_if_missing(kadedb_lite_options_t *opts,
                                               int create_if_missing);
void kadedb_lite_options_set_error_if_exists(kadedb_lite_options_t *opts,
                                             int error_if_exists);
void kadedb_lite_options_set_compression(kadedb_lite_options_t *opts,
                                         kadedb_lite_compression_t compression);
void kadedb_lite_options_set_cache_size_bytes(kadedb_lite_options_t *opts,
                                              size_t cache_size_bytes);
void kadedb_lite_options_set_write_buffer_size_bytes(
    kadedb_lite_options_t *opts, size_t write_buffer_size_bytes);
void kadedb_lite_options_set_max_open_files(kadedb_lite_options_t *opts,
                                            int max_open_files);

kadedb_lite_t *kadedb_lite_open(const char *path);
kadedb_lite_t *kadedb_lite_open_with_options(const char *path,
                                             const kadedb_lite_options_t *opts);
void kadedb_lite_close(kadedb_lite_t *db);

int kadedb_lite_put(kadedb_lite_t *db, const char *key, const char *value,
                    size_t value_len);
int kadedb_lite_get(kadedb_lite_t *db, const char *key, char **value_out,
                    size_t *value_len_out);
int kadedb_lite_delete(kadedb_lite_t *db, const char *key);
void kadedb_lite_free(char *p);

#ifdef __cplusplus
}
#endif

#endif // KADEDB_LITE_H
