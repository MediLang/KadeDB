#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kadedb_lite/kadedb_lite.h"
#include "kadedb_lite/kadedb_lite_query.h"

#define MAX_LINE 4096

typedef struct history_t {
  char **items;
  size_t count;
  size_t cap;
} history_t;

static void history_free(history_t *h) {
  if (!h)
    return;
  for (size_t i = 0; i < h->count; i++)
    free(h->items[i]);
  free(h->items);
  h->items = NULL;
  h->count = 0;
  h->cap = 0;
}

static int history_push(history_t *h, const char *line) {
  if (!h || !line)
    return 0;
  if (h->count == h->cap) {
    size_t new_cap = h->cap ? h->cap * 2 : 16;
    char **n = (char **)realloc(h->items, new_cap * sizeof(char *));
    if (!n)
      return 0;
    h->items = n;
    h->cap = new_cap;
  }
  size_t len = strlen(line);
  char *copy = (char *)malloc(len + 1);
  if (!copy)
    return 0;
  memcpy(copy, line, len + 1);
  h->items[h->count++] = copy;
  return 1;
}

static void rstrip(char *s) {
  if (!s)
    return;
  size_t n = strlen(s);
  while (n > 0 && (s[n - 1] == '\n' || s[n - 1] == '\r' || s[n - 1] == ' ' ||
                   s[n - 1] == '\t')) {
    s[n - 1] = '\0';
    n--;
  }
}

static const char *lskip(const char *s) {
  while (s && (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r'))
    s++;
  return s;
}

static int streq_ci_prefix(const char *s, const char *prefix) {
  if (!s || !prefix)
    return 0;
  while (*prefix) {
    if (!*s)
      return 0;
    if (tolower((unsigned char)*s) != tolower((unsigned char)*prefix))
      return 0;
    s++;
    prefix++;
  }
  return 1;
}

static void print_help(void) {
  printf("Commands:\n");
  printf("  help\n");
  printf("  quit | exit\n");
  printf("  history\n");
  printf("  !N              (re-run history entry N, 1-based)\n");
  printf("  import <table> <csv_path>   (CSV rows of id,value)\n");
  printf("  export <query> <csv_path>   (run query and write CSV)\n");
  printf("\n");
  printf("Queries:\n");
  printf("  Lite supports a small SQL-ish subset (SELECT/INSERT).\n");
  printf("  Example: INSERT INTO users (id, value) VALUES (1, alice)\n");
  printf("  Example: SELECT id, value FROM users WHERE id=1\n");
}

static void print_result(const kadedb_lite_result_t *r) {
  if (!r) {
    printf("(null result)\n");
    return;
  }

  const char *err = kadedb_lite_result_error(r);
  if (err && err[0] != '\0') {
    printf("Error: %s\n", err);
    return;
  }

  size_t cols = kadedb_lite_result_column_count(r);
  size_t rows = kadedb_lite_result_row_count(r);
  int affected = kadedb_lite_result_affected_rows(r);

  if (affected > 0 && rows == 0) {
    printf("OK (%d affected)\n", affected);
    return;
  }

  if (cols == 0) {
    printf("OK\n");
    return;
  }

  for (size_t c = 0; c < cols; c++) {
    const char *name = kadedb_lite_result_column_name(r, c);
    printf("%s%s", name ? name : "", (c + 1 < cols) ? "\t" : "\n");
  }

  for (size_t i = 0; i < rows; i++) {
    for (size_t c = 0; c < cols; c++) {
      const char *v = kadedb_lite_result_value(r, i, c);
      printf("%s%s", v ? v : "", (c + 1 < cols) ? "\t" : "\n");
    }
  }

  printf("(%zu row(s))\n", rows);
}

static int csv_needs_quote(const char *s) {
  if (!s)
    return 0;
  for (const char *p = s; *p; p++) {
    if (*p == ',' || *p == '"' || *p == '\n' || *p == '\r')
      return 1;
  }
  return 0;
}

static void csv_write_field(FILE *f, const char *s) {
  if (!s)
    s = "";
  if (!csv_needs_quote(s)) {
    fputs(s, f);
    return;
  }
  fputc('"', f);
  for (const char *p = s; *p; p++) {
    if (*p == '"')
      fputc('"', f);
    fputc(*p, f);
  }
  fputc('"', f);
}

static char *sql_quote_value(const char *s) {
  if (!s)
    s = "";

  size_t extra = 0;
  for (const char *p = s; *p; p++) {
    if (*p == '\\' || *p == '\'')
      extra++;
  }

  size_t n = strlen(s);
  char *out = (char *)malloc(n + extra + 3);
  if (!out)
    return NULL;

  char *w = out;
  *w++ = '\'';
  for (const char *p = s; *p; p++) {
    if (*p == '\\' || *p == '\'')
      *w++ = '\\';
    *w++ = *p;
  }
  *w++ = '\'';
  *w++ = '\0';
  return out;
}

static int cmd_export(kadedb_lite_t *db, const char *args) {
  const char *p = lskip(args);
  if (!p || !*p) {
    printf("usage: export <query> <csv_path>\n");
    return 0;
  }

  const char *last_space = strrchr(p, ' ');
  if (!last_space) {
    printf("usage: export <query> <csv_path>\n");
    return 0;
  }

  while (last_space > p && last_space[-1] == ' ')
    last_space--;

  size_t qlen = (size_t)(last_space - p);
  char *query = (char *)malloc(qlen + 1);
  if (!query)
    return 0;
  memcpy(query, p, qlen);
  query[qlen] = '\0';

  const char *path = lskip(last_space);
  if (!path || !*path) {
    free(query);
    printf("usage: export <query> <csv_path>\n");
    return 0;
  }

  kadedb_lite_result_t *r = kadedb_lite_execute_query(db, query);
  free(query);
  if (!r) {
    printf("export: query failed (NULL result)\n");
    return 0;
  }

  const char *err = kadedb_lite_result_error(r);
  if (err && err[0] != '\0') {
    printf("export: query error: %s\n", err);
    kadedb_lite_result_free(r);
    return 0;
  }

  FILE *f = fopen(path, "w");
  if (!f) {
    printf("export: failed to open file: %s\n", path);
    kadedb_lite_result_free(r);
    return 0;
  }

  size_t cols = kadedb_lite_result_column_count(r);
  size_t rows = kadedb_lite_result_row_count(r);

  for (size_t c = 0; c < cols; c++) {
    const char *name = kadedb_lite_result_column_name(r, c);
    csv_write_field(f, name);
    if (c + 1 < cols)
      fputc(',', f);
  }
  fputc('\n', f);

  for (size_t i = 0; i < rows; i++) {
    for (size_t c = 0; c < cols; c++) {
      const char *v = kadedb_lite_result_value(r, i, c);
      csv_write_field(f, v);
      if (c + 1 < cols)
        fputc(',', f);
    }
    fputc('\n', f);
  }

  fclose(f);
  kadedb_lite_result_free(r);
  printf("exported %zu row(s) to %s\n", rows, path);
  return 1;
}

static int cmd_import(kadedb_lite_t *db, const char *args) {
  const char *p = lskip(args);
  if (!p || !*p) {
    printf("usage: import <table> <csv_path>\n");
    return 0;
  }

  const char *space = strchr(p, ' ');
  if (!space) {
    printf("usage: import <table> <csv_path>\n");
    return 0;
  }

  size_t tlen = (size_t)(space - p);
  char *table = (char *)malloc(tlen + 1);
  if (!table)
    return 0;
  memcpy(table, p, tlen);
  table[tlen] = '\0';

  const char *path = lskip(space);
  if (!path || !*path) {
    free(table);
    printf("usage: import <table> <csv_path>\n");
    return 0;
  }

  FILE *f = fopen(path, "r");
  if (!f) {
    printf("import: failed to open file: %s\n", path);
    free(table);
    return 0;
  }

  char line[MAX_LINE];
  size_t imported = 0;

  while (fgets(line, sizeof(line), f)) {
    rstrip(line);
    const char *ln = lskip(line);
    if (!*ln)
      continue;

    const char *comma = strchr(ln, ',');
    if (!comma)
      continue;

    size_t id_len = (size_t)(comma - ln);
    char *id = (char *)malloc(id_len + 1);
    if (!id)
      break;
    memcpy(id, ln, id_len);
    id[id_len] = '\0';

    const char *val_raw = comma + 1;

    char *id_q = sql_quote_value(id);
    char *val_q = sql_quote_value(val_raw);
    free(id);

    if (!id_q || !val_q) {
      free(id_q);
      free(val_q);
      break;
    }

    size_t qlen = strlen("INSERT INTO  (id, value) VALUES (, )") +
                  strlen(table) + strlen(id_q) + strlen(val_q) + 1;
    char *query = (char *)malloc(qlen);
    if (!query) {
      free(id_q);
      free(val_q);
      break;
    }

    snprintf(query, qlen, "INSERT INTO %s (id, value) VALUES (%s, %s)", table,
             id_q, val_q);

    free(id_q);
    free(val_q);

    kadedb_lite_result_t *r = kadedb_lite_execute_query(db, query);
    free(query);

    if (!r)
      continue;

    const char *err = kadedb_lite_result_error(r);
    if (err && err[0] != '\0') {
      printf("import: row error: %s\n", err);
      kadedb_lite_result_free(r);
      continue;
    }

    imported++;
    kadedb_lite_result_free(r);
  }

  fclose(f);
  free(table);
  printf("imported %zu row(s)\n", imported);
  return 1;
}

static int run_query_line(kadedb_lite_t *db, const char *line) {
  if (!db || !line)
    return 0;
  kadedb_lite_result_t *r = kadedb_lite_execute_query(db, line);
  if (!r) {
    printf("(no result)\n");
    return 0;
  }
  print_result(r);
  kadedb_lite_result_free(r);
  return 1;
}

static int parse_history_index(const char *s, size_t *out_index) {
  if (!s || !out_index || *s != '!')
    return 0;
  s++;
  if (!*s)
    return 0;
  char *end = NULL;
  long v = strtol(s, &end, 10);
  if (!end || *end != '\0')
    return 0;
  if (v <= 0)
    return 0;
  *out_index = (size_t)(v - 1);
  return 1;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    printf("Usage: %s <db_path>\n", argv[0]);
    return 1;
  }

  const char *db_path = argv[1];
  kadedb_lite_t *db = kadedb_lite_open(db_path);
  if (!db) {
    fprintf(stderr, "Failed to open Lite DB at: %s\n", db_path);
    return 2;
  }

  printf("KadeDB-Lite CLI\n");
  printf("Type 'help' for commands.\n\n");

  history_t hist;
  memset(&hist, 0, sizeof(hist));

  char line[MAX_LINE];
  while (1) {
    printf("Lite> ");
    if (!fgets(line, sizeof(line), stdin)) {
      printf("\n(EOF)\n");
      break;
    }

    rstrip(line);
    const char *cmd = lskip(line);
    if (!cmd || !*cmd)
      continue;

    if (strcmp(cmd, "exit") == 0 || strcmp(cmd, "quit") == 0)
      break;

    if (strcmp(cmd, "help") == 0) {
      print_help();
      continue;
    }

    if (strcmp(cmd, "history") == 0) {
      for (size_t i = 0; i < hist.count; i++) {
        printf("%zu  %s\n", i + 1, hist.items[i]);
      }
      continue;
    }

    size_t recall_idx = 0;
    if (parse_history_index(cmd, &recall_idx)) {
      if (recall_idx >= hist.count) {
        printf("history index out of range\n");
        continue;
      }
      cmd = hist.items[recall_idx];
      printf("%s\n", cmd);
    } else {
      if (!history_push(&hist, cmd)) {
        printf("warning: failed to store history\n");
      }
    }

    if (streq_ci_prefix(cmd, "import ")) {
      cmd_import(db, cmd + strlen("import"));
      continue;
    }

    if (streq_ci_prefix(cmd, "export ")) {
      cmd_export(db, cmd + strlen("export"));
      continue;
    }

    run_query_line(db, cmd);
  }

  history_free(&hist);
  kadedb_lite_close(db);
  return 0;
}
