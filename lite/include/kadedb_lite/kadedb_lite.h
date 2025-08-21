#ifndef KADEDB_LITE_H
#define KADEDB_LITE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

typedef struct kadedb_lite_t kadedb_lite_t;

kadedb_lite_t* kadedb_lite_open(const char* path);
void kadedb_lite_close(kadedb_lite_t* db);

int kadedb_lite_put(kadedb_lite_t* db, const char* key, const char* value, size_t value_len);
int kadedb_lite_get(kadedb_lite_t* db, const char* key, char** value_out, size_t* value_len_out);
void kadedb_lite_free(char* p);

#ifdef __cplusplus
}
#endif

#endif // KADEDB_LITE_H
