#ifndef KADEDB_LITE_SYNC_H
#define KADEDB_LITE_SYNC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "kadedb_lite.h"

typedef struct kadedb_lite_sync_config_t {
  const char *remote_url;
  const char *auth_token;
  int sync_interval_seconds;
} kadedb_lite_sync_config_t;

int kadedb_lite_sync_init(kadedb_lite_t *db, kadedb_lite_sync_config_t *config);

int kadedb_lite_sync_start(kadedb_lite_t *db);

int kadedb_lite_sync_stop(kadedb_lite_t *db);

int kadedb_lite_sync_status(kadedb_lite_t *db, char **status_out);

#ifdef __cplusplus
}
#endif

#endif // KADEDB_LITE_SYNC_H
