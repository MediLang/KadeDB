#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kadedb_lite/kadedb_lite.h"
#include "kadedb_lite/kadedb_lite_sync.h"

static int test_count = 0;
static int pass_count = 0;

#define TEST(name)                                                             \
  do {                                                                         \
    test_count++;                                                              \
    printf("  Testing: %s... ", name);                                         \
  } while (0)

#define PASS()                                                                 \
  do {                                                                         \
    pass_count++;                                                              \
    printf("PASS\n");                                                          \
  } while (0)

#define FAIL(msg)                                                              \
  do {                                                                         \
    printf("FAIL: %s\n", msg);                                                 \
  } while (0)

static int expect_rc_nonzero(int rc) { return rc != 0; }
static int expect_rc_zero(int rc) { return rc == 0; }

static int test_invalid_args(void) {
  TEST("sync invalid args");

  kadedb_lite_sync_config_t cfg;
  memset(&cfg, 0, sizeof(cfg));

  if (!expect_rc_nonzero(kadedb_lite_sync_init(NULL, &cfg))) {
    FAIL("init should fail on NULL db");
    return 0;
  }

  if (!expect_rc_nonzero(kadedb_lite_sync_start(NULL))) {
    FAIL("start should fail on NULL db");
    return 0;
  }

  if (!expect_rc_nonzero(kadedb_lite_sync_stop(NULL))) {
    FAIL("stop should fail on NULL db");
    return 0;
  }

  char *status = NULL;
  if (!expect_rc_nonzero(kadedb_lite_sync_status(NULL, &status))) {
    FAIL("status should fail on NULL db");
    return 0;
  }

  if (!expect_rc_nonzero(kadedb_lite_sync_status((kadedb_lite_t *)0x1, NULL))) {
    FAIL("status should fail on NULL status_out");
    return 0;
  }

  PASS();
  return 1;
}

static int test_basic_flow(void) {
  TEST("sync init/start/stop/status");

  kadedb_lite_t *db = kadedb_lite_open("./tmp_lite_db_sync");
  if (!db) {
    FAIL("kadedb_lite_open failed");
    return 0;
  }

  char *status = NULL;
  if (!expect_rc_zero(kadedb_lite_sync_status(db, &status)) || !status) {
    FAIL("status should succeed and allocate string");
    kadedb_lite_close(db);
    return 0;
  }
  kadedb_lite_free(status);

  if (!expect_rc_nonzero(kadedb_lite_sync_start(db))) {
    FAIL("start should fail before init");
    kadedb_lite_close(db);
    return 0;
  }

  kadedb_lite_sync_config_t cfg;
  memset(&cfg, 0, sizeof(cfg));
  cfg.remote_url = "https://example.invalid";
  cfg.auth_token = "token";
  cfg.sync_interval_seconds = 30;

  if (!expect_rc_zero(kadedb_lite_sync_init(db, &cfg))) {
    FAIL("init should succeed");
    kadedb_lite_close(db);
    return 0;
  }

  if (!expect_rc_zero(kadedb_lite_sync_start(db))) {
    FAIL("start should succeed after init");
    kadedb_lite_close(db);
    return 0;
  }

  status = NULL;
  if (!expect_rc_zero(kadedb_lite_sync_status(db, &status)) || !status) {
    FAIL("status should succeed when running");
    kadedb_lite_close(db);
    return 0;
  }
  if (strstr(status, "state=running") == NULL) {
    FAIL("expected running state");
    kadedb_lite_free(status);
    kadedb_lite_close(db);
    return 0;
  }
  kadedb_lite_free(status);

  if (!expect_rc_zero(kadedb_lite_sync_stop(db))) {
    FAIL("stop should succeed");
    kadedb_lite_close(db);
    return 0;
  }

  status = NULL;
  if (!expect_rc_zero(kadedb_lite_sync_status(db, &status)) || !status) {
    FAIL("status should succeed when stopped");
    kadedb_lite_close(db);
    return 0;
  }
  if (strstr(status, "state=stopped") == NULL) {
    FAIL("expected stopped state");
    kadedb_lite_free(status);
    kadedb_lite_close(db);
    return 0;
  }
  kadedb_lite_free(status);

  if (kadedb_lite_put(db, "k", "v", 1) != 0) {
    FAIL("put should still work");
    kadedb_lite_close(db);
    return 0;
  }

  kadedb_lite_close(db);
  PASS();
  return 1;
}

static int test_negative_interval_rejected(void) {
  TEST("sync negative interval rejected");

  kadedb_lite_t *db = kadedb_lite_open("./tmp_lite_db_sync_neg");
  if (!db) {
    FAIL("kadedb_lite_open failed");
    return 0;
  }

  kadedb_lite_sync_config_t cfg;
  memset(&cfg, 0, sizeof(cfg));
  cfg.remote_url = "x";
  cfg.auth_token = "y";
  cfg.sync_interval_seconds = -1;

  if (!expect_rc_nonzero(kadedb_lite_sync_init(db, &cfg))) {
    FAIL("expected init to fail on negative interval");
    kadedb_lite_close(db);
    return 0;
  }

  kadedb_lite_close(db);
  PASS();
  return 1;
}

int main(void) {
  int ok = 1;

  ok &= test_invalid_args();
  ok &= test_basic_flow();
  ok &= test_negative_interval_rejected();

  printf("\nSync tests: %d/%d passed\n", pass_count, test_count);
  return ok ? 0 : 1;
}
