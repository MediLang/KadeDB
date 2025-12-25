#include "kadedb_lite/kadedb_lite_sync.h"

#include "kadedb_lite_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *kadedb_lite_sync_strdup(const char *s) {
  if (!s)
    return NULL;
  size_t n = strlen(s);
  char *out = (char *)malloc(n + 1);
  if (!out)
    return NULL;
  memcpy(out, s, n + 1);
  return out;
}

int kadedb_lite_sync_init(kadedb_lite_t *db,
                          kadedb_lite_sync_config_t *config) {
  if (!db || !config)
    return -1;

  if (config->sync_interval_seconds < 0)
    return -1;

  if (db->sync_remote_url) {
    free(db->sync_remote_url);
    db->sync_remote_url = NULL;
  }
  if (db->sync_auth_token) {
    free(db->sync_auth_token);
    db->sync_auth_token = NULL;
  }

  db->sync_remote_url = kadedb_lite_sync_strdup(config->remote_url);
  if (config->remote_url && !db->sync_remote_url)
    return -1;

  db->sync_auth_token = kadedb_lite_sync_strdup(config->auth_token);
  if (config->auth_token && !db->sync_auth_token) {
    free(db->sync_remote_url);
    db->sync_remote_url = NULL;
    return -1;
  }

  db->sync_interval_seconds = config->sync_interval_seconds;
  db->sync_initialized = 1;
  db->sync_running = 0;
  return 0;
}

int kadedb_lite_sync_start(kadedb_lite_t *db) {
  if (!db)
    return -1;
  if (!db->sync_initialized)
    return -1;
  db->sync_running = 1;
  return 0;
}

int kadedb_lite_sync_stop(kadedb_lite_t *db) {
  if (!db)
    return -1;
  if (!db->sync_initialized)
    return -1;
  db->sync_running = 0;
  return 0;
}

int kadedb_lite_sync_status(kadedb_lite_t *db, char **status_out) {
  if (!db || !status_out)
    return -1;

  const char *state = "uninitialized";
  if (db->sync_initialized) {
    state = db->sync_running ? "running" : "stopped";
  }

  const char *remote = db->sync_remote_url ? db->sync_remote_url : "";
  const char *token = db->sync_auth_token ? db->sync_auth_token : "";

  char interval_buf[32];
  interval_buf[0] = '\0';
  {
    int n = snprintf(interval_buf, sizeof(interval_buf), "%d",
                     db->sync_interval_seconds);
    if (n < 0)
      interval_buf[0] = '\0';
  }

  const char *prefix = "kadedb_lite_sync:";
  const char *k_state = " state=";
  const char *k_remote = " remote_url=";
  const char *k_token = " auth_token=";
  const char *k_interval = " interval_seconds=";

  size_t total = strlen(prefix) + strlen(k_state) + strlen(state) +
                 strlen(k_remote) + strlen(remote) + strlen(k_token) +
                 strlen(token) + strlen(k_interval) + strlen(interval_buf) + 1;

  char *buf = (char *)malloc(total);
  if (!buf)
    return -1;

  strcpy(buf, prefix);
  strcat(buf, k_state);
  strcat(buf, state);
  strcat(buf, k_remote);
  strcat(buf, remote);
  strcat(buf, k_token);
  strcat(buf, token);
  strcat(buf, k_interval);
  strcat(buf, interval_buf);

  *status_out = buf;
  return 0;
}
