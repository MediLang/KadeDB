#ifndef KADEDB_C_API_H
#define KADEDB_C_API_H

#ifdef __cplusplus
extern "C" {
#endif

// Returns a string literal with the KadeDB version (e.g., "0.1.0").
const char *KadeDB_GetVersion();
// Version helpers (numeric components)
int KadeDB_GetMajorVersion();
int KadeDB_GetMinorVersion();
int KadeDB_GetPatchVersion();

// Optional library init/cleanup (currently no-op; reserved for future use)
int KadeDB_Initialize();
void KadeDB_Shutdown();

// ---------- Schema/Document minimal C API ----------

// Opaque handle to a document schema
typedef struct KDB_DocumentSchema KDB_DocumentSchema;

// Column type mirror
typedef enum KDB_ColumnType {
  KDB_COL_NULL = 0,
  KDB_COL_INTEGER = 1,
  KDB_COL_FLOAT = 2,
  KDB_COL_STRING = 3,
  KDB_COL_BOOLEAN = 4,
} KDB_ColumnType;

// Value container for passing document values from C
typedef enum KDB_ValueType {
  KDB_VAL_NULL = 0,
  KDB_VAL_INTEGER = 1,
  KDB_VAL_FLOAT = 2,
  KDB_VAL_STRING = 3,
  KDB_VAL_BOOLEAN = 4,
} KDB_ValueType;

typedef struct KDB_Value {
  KDB_ValueType type;
  union {
    long long i64;   // for INTEGER
    double f64;      // for FLOAT
    const char *str; // for STRING (UTF-8, NUL-terminated)
    int boolean;     // for BOOLEAN (0 or 1)
  } as;
} KDB_Value;

typedef struct KDB_KeyValue {
  const char *key; // UTF-8, NUL-terminated
  KDB_Value value;
} KDB_KeyValue;

typedef struct KDB_DocumentView {
  const KDB_KeyValue *items;
  unsigned long long count;
} KDB_DocumentView;

// Simple table column descriptor for row validation APIs
typedef struct KDB_TableColumn {
  const char *name; // UTF-8, NUL-terminated
  KDB_ColumnType type;
  int nullable; // 0/1
  int unique;   // 0/1
} KDB_TableColumn;

// Row view: array of values matching table columns
typedef struct KDB_RowView {
  const KDB_Value *values;
  unsigned long long count;
} KDB_RowView;

// Simple comparison operators for predicates
typedef enum KDB_CompareOp {
  KDB_OP_EQ = 0,
  KDB_OP_NE = 1,
  KDB_OP_LT = 2,
  KDB_OP_LE = 3,
  KDB_OP_GT = 4,
  KDB_OP_GE = 5,
} KDB_CompareOp;

// Minimal predicate: single-column comparison against a constant value
typedef struct KDB_Predicate {
  const char *column; // column name (UTF-8)
  KDB_CompareOp op;   // comparison operator
  KDB_Value rhs;      // right-hand side value
} KDB_Predicate;

// Assignment for UPDATE: either a constant value or copy from another column
typedef struct KDB_Assignment {
  const char *column;     // target column name
  int is_column_ref;      // 0 = use constant; 1 = copy from column_ref
  const char *column_ref; // when is_column_ref==1, source column name
  KDB_Value constant;     // when is_column_ref==0, constant value
} KDB_Assignment;

// Column constraints for table columns (unset: min_len/max_len = -1,
// min_value/max_value = NaN)
typedef struct KDB_ColumnConstraints {
  long long min_len; // string; -1 for unset
  long long max_len; // string; -1 for unset
  const char *const
      *one_of; // array of allowed string values; NULL or count=0 to ignore
  unsigned long long one_of_count;
  double min_value; // numeric; NaN for unset
  double max_value; // numeric; NaN for unset
} KDB_ColumnConstraints;

// Optional constraints pointer may be NULL
typedef struct KDB_TableColumnEx {
  const char *name;
  KDB_ColumnType type;
  int nullable;
  int unique;
  const KDB_ColumnConstraints *constraints; // optional
} KDB_TableColumnEx;

// ---- Persistent TableSchema wrapper ----
typedef struct KDB_TableSchema KDB_TableSchema;

KDB_TableSchema *KadeDB_TableSchema_Create();
void KadeDB_TableSchema_Destroy(KDB_TableSchema *schema);

// Add/Remove columns
// Returns 1 on success, 0 on duplicate name
int KadeDB_TableSchema_AddColumn(KDB_TableSchema *schema,
                                 const KDB_TableColumnEx *column);
// Returns 1 on success, 0 if not found
int KadeDB_TableSchema_RemoveColumn(KDB_TableSchema *schema, const char *name);

// Column updates by name
int KadeDB_TableSchema_SetColumnFlags(KDB_TableSchema *schema, const char *name,
                                      int nullable, int unique);
int KadeDB_TableSchema_SetStringConstraints(KDB_TableSchema *schema,
                                            const char *name, long long min_len,
                                            long long max_len,
                                            const char *const *one_of,
                                            unsigned long long one_of_count);
int KadeDB_TableSchema_SetNumericConstraints(KDB_TableSchema *schema,
                                             const char *name, double min_value,
                                             double max_value);

// Primary key: pass NULL to clear. Returns 1 on success, 0 on error (e.g.,
// column missing)
int KadeDB_TableSchema_SetPrimaryKey(KDB_TableSchema *schema, const char *name);

// Validation
int KadeDB_TableSchema_ValidateRow(const KDB_TableSchema *schema,
                                   const KDB_RowView *row, char *err_buf,
                                   unsigned long long err_buf_len);
int KadeDB_TableSchema_ValidateUniqueRows(const KDB_TableSchema *schema,
                                          const KDB_RowView *rows,
                                          unsigned long long row_count,
                                          int ignore_nulls, char *err_buf,
                                          unsigned long long err_buf_len);

// Create/destroy document schema
KDB_DocumentSchema *KadeDB_DocumentSchema_Create();
void KadeDB_DocumentSchema_Destroy(KDB_DocumentSchema *schema);

// Add a field definition to the schema. Returns 1 on success, 0 on duplicate
// name.
int KadeDB_DocumentSchema_AddField(KDB_DocumentSchema *schema, const char *name,
                                   KDB_ColumnType type, int nullable,
                                   int unique);

// Validate a document given as an array of key/value pairs.
// Returns 1 if valid, 0 if invalid. If invalid and err_buf is provided, writes
// an error message.
int KadeDB_ValidateDocument(const KDB_DocumentSchema *schema,
                            const KDB_KeyValue *items, unsigned long long count,
                            char *err_buf, unsigned long long err_buf_len);

// Validate uniqueness constraints across an array of documents. Returns 1 if
// valid, 0 otherwise.
int KadeDB_ValidateUniqueDocuments(const KDB_DocumentSchema *schema,
                                   const KDB_DocumentView *docs,
                                   unsigned long long doc_count,
                                   int ignore_nulls, char *err_buf,
                                   unsigned long long err_buf_len);

// Validate uniqueness constraints across rows based on provided column
// descriptors.
int KadeDB_ValidateUniqueRows(const KDB_TableColumn *columns,
                              unsigned long long column_count,
                              const KDB_RowView *rows,
                              unsigned long long row_count, int ignore_nulls,
                              char *err_buf, unsigned long long err_buf_len);

// Validate a single row against table column descriptors with optional
// constraints
int KadeDB_ValidateRow(const KDB_TableColumnEx *columns,
                       unsigned long long column_count, const KDB_RowView *row,
                       char *err_buf, unsigned long long err_buf_len);

// ---- Constraint setters for document schema fields ----
// Set string constraints; pass -1 for min_len/max_len to indicate "unset".
// one_of may be NULL or count=0 to clear the allowed set.
int KadeDB_DocumentSchema_SetStringConstraints(KDB_DocumentSchema *schema,
                                               const char *field_name,
                                               long long min_len,
                                               long long max_len,
                                               const char *const *one_of,
                                               unsigned long long one_of_count);

// Set numeric constraints; pass NaN to indicate "unset".
int KadeDB_DocumentSchema_SetNumericConstraints(KDB_DocumentSchema *schema,
                                                const char *field_name,
                                                double min_value,
                                                double max_value);

// Set document field flags post-creation
int KadeDB_DocumentSchema_SetFieldFlags(KDB_DocumentSchema *schema,
                                        const char *field_name, int nullable,
                                        int unique);

// ---------- Result conversion & pagination utilities ----------

// Convert a tabular result (columns + rows) to CSV.
// - column_names: array of UTF-8, NUL-terminated strings; may be NULL if
// include_header==0
// - types: array of KDB_ColumnType of length column_count (may be NULL; used
// only for metadata consistency)
// - rows: array of KDB_RowView of length row_count; each row.values must have
// at least column_count items
// - delimiter: CSV delimiter character (e.g., ',')
// - include_header: 0/1 to include header row from column_names
// - out_buf: optional output buffer to receive CSV (may be NULL to query
// required length)
// - out_buf_len: size in bytes of out_buf
// - out_required_len: if non-NULL, set to required byte length (including
// terminating NUL) Returns 1 on success (possibly truncated if out_buf too
// small), 0 on error (e.g., invalid args).
int KadeDB_Result_ToCSV(const char *const *column_names,
                        const KDB_ColumnType *types,
                        unsigned long long column_count,
                        const KDB_RowView *rows, unsigned long long row_count,
                        char delimiter, int include_header, char *out_buf,
                        unsigned long long out_buf_len,
                        unsigned long long *out_required_len);

// Convert a tabular result (columns + rows) to JSON.
// - include_metadata: 0 to emit an array of row objects; 1 to emit an object
// with {columns, types, rows} Buffer semantics are identical to
// KadeDB_Result_ToCSV.
int KadeDB_Result_ToJSON(const char *const *column_names,
                         const KDB_ColumnType *types,
                         unsigned long long column_count,
                         const KDB_RowView *rows, unsigned long long row_count,
                         int include_metadata, char *out_buf,
                         unsigned long long out_buf_len,
                         unsigned long long *out_required_len);

// Extended CSV: control quoting behavior
int KadeDB_Result_ToCSVEx(const char *const *column_names,
                          const KDB_ColumnType *types,
                          unsigned long long column_count,
                          const KDB_RowView *rows, unsigned long long row_count,
                          char delimiter, int include_header, int always_quote,
                          char quote_char, char *out_buf,
                          unsigned long long out_buf_len,
                          unsigned long long *out_required_len);

// Extended JSON: pretty-print by setting indent (spaces per level, 0 for
// compact)
int KadeDB_Result_ToJSONEx(const char *const *column_names,
                           const KDB_ColumnType *types,
                           unsigned long long column_count,
                           const KDB_RowView *rows,
                           unsigned long long row_count, int include_metadata,
                           int indent, char *out_buf,
                           unsigned long long out_buf_len,
                           unsigned long long *out_required_len);

// Compute pagination bounds.
// - total_rows: total number of rows
// - page_size: 0 means all rows in a single page; otherwise positive page size
// - page_index: zero-based page index
// Outputs [start, end) bounds and total_pages. Returns 1 on success, 0 on error
// (e.g., page_index out of range).
int KadeDB_Paginate(unsigned long long total_rows, unsigned long long page_size,
                    unsigned long long page_index,
                    unsigned long long *out_start, unsigned long long *out_end,
                    unsigned long long *out_total_pages);

// Convenience: compute only total pages
int KadeDB_Paginate_TotalPages(unsigned long long total_rows,
                               unsigned long long page_size,
                               unsigned long long *out_total_pages);

// Convenience: compute bounds for a page; returns 1 on success, 0 on error
int KadeDB_Paginate_Bounds(unsigned long long total_rows,
                           unsigned long long page_size,
                           unsigned long long page_index,
                           unsigned long long *out_start,
                           unsigned long long *out_end);

// ---------- Relational Storage minimal C API ----------

// Opaque storage type wrapping an in-memory relational engine
typedef struct KadeDB_Storage KadeDB_Storage;

// Opaque ResultSet cursor for simple row iteration
typedef struct KadeDB_ResultSet KadeDB_ResultSet;

// Create/destroy storage instance
KadeDB_Storage *KadeDB_CreateStorage();
void KadeDB_DestroyStorage(KadeDB_Storage *storage);

// Create a table with a provided schema
// Returns 1 on success; 0 on error
int KadeDB_CreateTable(KadeDB_Storage *storage, const char *table,
                       const KDB_TableSchema *schema);

// Insert a row; row is provided as a view (values array)
// Returns 1 on success; 0 on error
int KadeDB_InsertRow(KadeDB_Storage *storage, const char *table,
                     const KDB_RowView *row);

// Execute a very basic query string. Supported form:
//   "SELECT * FROM <table>"
// Returns a result set cursor or NULL on error/unsupported query
KadeDB_ResultSet *KadeDB_ExecuteQuery(KadeDB_Storage *storage,
                                      const char *query);

// ResultSet iteration utilities
// Move to next row; returns 1 when a row is available, 0 when no more rows
int KadeDB_ResultSet_NextRow(KadeDB_ResultSet *rs);
// Get a string representation of the current row's column; returns NULL on
// error
const char *KadeDB_ResultSet_GetString(KadeDB_ResultSet *rs, int column);
// Destroy the result set and free resources
void KadeDB_DestroyResultSet(KadeDB_ResultSet *rs);

// ---- Additional CRUD and schema utilities ----

// Update rows by assigning to one or more columns, optionally filtered by a
// predicate.
// - assignments: array of KDB_Assignment entries (>=1)
// - where_predicate: optional; pass NULL to update all rows
// - out_updated: optional; set to number of rows updated
// Returns 1 on success; 0 on error.
int KadeDB_UpdateRows(KadeDB_Storage *storage, const char *table,
                      const KDB_Assignment *assignments,
                      unsigned long long assignment_count,
                      const KDB_Predicate *where_predicate,
                      unsigned long long *out_updated);

// Delete rows optionally filtered by a predicate. If where_predicate is NULL,
// deletes all rows. Returns 1 on success; 0 on error. Writes deleted count to
// out_deleted when non-NULL.
int KadeDB_DeleteRows(KadeDB_Storage *storage, const char *table,
                      const KDB_Predicate *where_predicate,
                      unsigned long long *out_deleted);

// Drop an entire table (schema and data). Returns 1 on success; 0 on error.
int KadeDB_DropTable(KadeDB_Storage *storage, const char *table);

// Truncate a table (delete all rows, keep schema). Returns 1 on success; 0 on
// error.
int KadeDB_TruncateTable(KadeDB_Storage *storage, const char *table);

// List tables as a delimited single-line string (e.g., comma-separated).
// - delimiter: character to separate names (e.g., ',')
// - out_buf/out_buf_len: optional output buffer; may be NULL to query required
// length
// - out_required_len: when non-NULL, set to required byte length including NUL
// Returns 1 on success.
int KadeDB_ListTables_ToCSV(KadeDB_Storage *storage, char delimiter,
                            char *out_buf, unsigned long long out_buf_len,
                            unsigned long long *out_required_len);

#ifdef __cplusplus
}
#endif

#endif // KADEDB_C_API_H
