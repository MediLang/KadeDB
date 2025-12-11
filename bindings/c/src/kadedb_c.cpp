#include "kadedb/kadedb.h"
#include "kadedb/version.h"

#include "kadedb/result.h"
#include "kadedb/schema.h"
#include "kadedb/storage.h"
#include "kadedb/value.h"

#include <cmath>
#include <cstring>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

using namespace kadedb;

// Forward declarations for helpers used before their definitions
static ColumnType to_cpp_column_type(KDB_ColumnType t);
static std::unique_ptr<Value> from_c_value(const KDB_Value &v);
static Column make_cpp_column_from_c_ex(const KDB_TableColumnEx &cex);
static Column make_cpp_column_from_c(const KDB_TableColumn &c);
static Column make_cpp_column_from_ex_ptr(const KDB_TableColumnEx *cex);
static Predicate::Op to_cpp_op(KDB_CompareOp op);
static std::optional<Predicate> to_cpp_predicate(const KDB_Predicate *p);

// Define wrapper structs before use
struct KDB_DocumentSchema {
  DocumentSchema impl;
};

struct KDB_TableSchema {
  TableSchema impl;
};

extern "C" KDB_TableSchema *KadeDB_TableSchema_Create() {
  try {
    return new KDB_TableSchema{};
  } catch (...) {
    return nullptr;
  }
}

extern "C" void KadeDB_TableSchema_Destroy(KDB_TableSchema *schema) {
  delete schema;
}

static Column make_cpp_column_from_ex_ptr(const KDB_TableColumnEx *cex) {
  return make_cpp_column_from_c_ex(*cex);
}

extern "C" int KadeDB_TableSchema_AddColumn(KDB_TableSchema *schema,
                                            const KDB_TableColumnEx *column) {
  if (!schema || !column || !column->name)
    return 0;
  Column col = make_cpp_column_from_ex_ptr(column);
  return schema->impl.addColumn(col) ? 1 : 0;
}

extern "C" int KadeDB_TableSchema_RemoveColumn(KDB_TableSchema *schema,
                                               const char *name) {
  if (!schema || !name)
    return 0;
  return schema->impl.removeColumn(name) ? 1 : 0;
}

extern "C" int KadeDB_TableSchema_SetColumnFlags(KDB_TableSchema *schema,
                                                 const char *name, int nullable,
                                                 int unique) {
  if (!schema || !name)
    return 0;
  Column col;
  if (!schema->impl.getColumn(name, col))
    return 0;
  col.nullable = (nullable != 0);
  col.unique = (unique != 0);
  return schema->impl.updateColumn(col) ? 1 : 0;
}

extern "C" int KadeDB_TableSchema_SetStringConstraints(
    KDB_TableSchema *schema, const char *name, long long min_len,
    long long max_len, const char *const *one_of,
    unsigned long long one_of_count) {
  if (!schema || !name)
    return 0;
  Column col;
  if (!schema->impl.getColumn(name, col))
    return 0;
  if (min_len >= 0)
    col.constraints.minLength = static_cast<size_t>(min_len);
  else
    col.constraints.minLength.reset();
  if (max_len >= 0)
    col.constraints.maxLength = static_cast<size_t>(max_len);
  else
    col.constraints.maxLength.reset();
  col.constraints.oneOf.clear();
  if (one_of && one_of_count > 0) {
    col.constraints.oneOf.reserve(static_cast<size_t>(one_of_count));
    for (unsigned long long i = 0; i < one_of_count; ++i) {
      col.constraints.oneOf.emplace_back(one_of[i] ? std::string(one_of[i])
                                                   : std::string());
    }
  }
  return schema->impl.updateColumn(col) ? 1 : 0;
}

extern "C" int KadeDB_TableSchema_SetNumericConstraints(KDB_TableSchema *schema,
                                                        const char *name,
                                                        double min_value,
                                                        double max_value) {
  if (!schema || !name)
    return 0;
  Column col;
  if (!schema->impl.getColumn(name, col))
    return 0;
  if (std::isnan(min_value))
    col.constraints.minValue.reset();
  else
    col.constraints.minValue = min_value;
  if (std::isnan(max_value))
    col.constraints.maxValue.reset();
  else
    col.constraints.maxValue = max_value;
  return schema->impl.updateColumn(col) ? 1 : 0;
}

extern "C" int KadeDB_TableSchema_SetPrimaryKey(KDB_TableSchema *schema,
                                                const char *name) {
  if (!schema)
    return 0;
  try {
    if (name)
      schema->impl.setPrimaryKey(std::string{name});
    else
      schema->impl.setPrimaryKey(std::nullopt);
    return 1;
  } catch (...) {
    return 0;
  }
}

extern "C" int KadeDB_TableSchema_ValidateRow(const KDB_TableSchema *schema,
                                              const KDB_RowView *row,
                                              char *err_buf,
                                              unsigned long long err_buf_len) {
  if (!schema || !row)
    return 0;
  Row cppRow(static_cast<size_t>(row->count));
  for (unsigned long long i = 0; i < row->count; ++i) {
    const KDB_Value &v = row->values[i];
    if (v.type == KDB_VAL_NULL)
      cppRow.set(static_cast<size_t>(i), nullptr);
    else
      cppRow.set(static_cast<size_t>(i), from_c_value(v));
  }
  std::string err = SchemaValidator::validateRow(schema->impl, cppRow);
  if (!err.empty()) {
    if (err_buf && err_buf_len > 0) {
      std::strncpy(err_buf, err.c_str(), static_cast<size_t>(err_buf_len - 1));
      err_buf[err_buf_len - 1] = '\0';
    }
    return 0;
  }
  return 1;
}

extern "C" int KadeDB_TableSchema_ValidateUniqueRows(
    const KDB_TableSchema *schema, const KDB_RowView *rows,
    unsigned long long row_count, int ignore_nulls, char *err_buf,
    unsigned long long err_buf_len) {
  if (!schema || (!rows && row_count > 0))
    return 0;
  std::vector<Row> cppRows;
  cppRows.reserve(static_cast<size_t>(row_count));
  const size_t colCount = schema->impl.columns().size();
  for (unsigned long long r = 0; r < row_count; ++r) {
    Row cppRow(colCount);
    const KDB_RowView &rv = rows[r];
    for (unsigned long long i = 0; i < rv.count && i < colCount; ++i) {
      if (rv.values[i].type == KDB_VAL_NULL)
        cppRow.set(static_cast<size_t>(i), nullptr);
      else
        cppRow.set(static_cast<size_t>(i), from_c_value(rv.values[i]));
    }
    cppRows.emplace_back(std::move(cppRow));
  }
  std::string err =
      SchemaValidator::validateUnique(schema->impl, cppRows, ignore_nulls != 0);
  if (!err.empty()) {
    if (err_buf && err_buf_len > 0) {
      std::strncpy(err_buf, err.c_str(), static_cast<size_t>(err_buf_len - 1));
      err_buf[err_buf_len - 1] = '\0';
    }
    return 0;
  }
  return 1;
}

// Helper definitions (C++ linkage)
static ColumnType to_cpp_column_type(KDB_ColumnType t) {
  switch (t) {
  case KDB_COL_NULL:
    return ColumnType::Null;
  case KDB_COL_INTEGER:
    return ColumnType::Integer;
  case KDB_COL_FLOAT:
    return ColumnType::Float;
  case KDB_COL_STRING:
    return ColumnType::String;
  case KDB_COL_BOOLEAN:
    return ColumnType::Boolean;
  }
  return ColumnType::Null;
}

static std::unique_ptr<Value> from_c_value(const KDB_Value &v) {
  switch (v.type) {
  case KDB_VAL_NULL:
    return ValueFactory::createNull();
  case KDB_VAL_INTEGER:
    return ValueFactory::createInteger(static_cast<int64_t>(v.as.i64));
  case KDB_VAL_FLOAT:
    return ValueFactory::createFloat(v.as.f64);
  case KDB_VAL_STRING:
    return ValueFactory::createString(v.as.str ? std::string(v.as.str)
                                               : std::string());
  case KDB_VAL_BOOLEAN:
    return ValueFactory::createBoolean(v.as.boolean != 0);
  }
  return ValueFactory::createNull();
}

static Predicate::Op to_cpp_op(KDB_CompareOp op) {
  switch (op) {
  case KDB_OP_EQ:
    return Predicate::Op::Eq;
  case KDB_OP_NE:
    return Predicate::Op::Ne;
  case KDB_OP_LT:
    return Predicate::Op::Lt;
  case KDB_OP_LE:
    return Predicate::Op::Le;
  case KDB_OP_GT:
    return Predicate::Op::Gt;
  case KDB_OP_GE:
    return Predicate::Op::Ge;
  }
  return Predicate::Op::Eq;
}

static std::optional<Predicate> to_cpp_predicate(const KDB_Predicate *p) {
  if (!p || !p->column)
    return std::nullopt;
  Predicate pred;
  pred.kind = Predicate::Kind::Comparison;
  pred.column = std::string{p->column};
  pred.op = to_cpp_op(p->op);
  pred.rhs = from_c_value(p->rhs);
  return pred;
}

// Expose make_cpp_column_from_c_ex with C++ linkage (used by other helpers)
static Column make_cpp_column_from_c_ex(const KDB_TableColumnEx &cex) {
  Column col;
  col.name = cex.name ? std::string(cex.name) : std::string();
  col.type = to_cpp_column_type(cex.type);
  col.nullable = (cex.nullable != 0);
  col.unique = (cex.unique != 0);
  if (cex.constraints) {
    const auto *cc = cex.constraints;
    if (cc->min_len >= 0)
      col.constraints.minLength = static_cast<size_t>(cc->min_len);
    if (cc->max_len >= 0)
      col.constraints.maxLength = static_cast<size_t>(cc->max_len);
    if (cc->one_of && cc->one_of_count > 0) {
      col.constraints.oneOf.reserve(static_cast<size_t>(cc->one_of_count));
      for (unsigned long long i = 0; i < cc->one_of_count; ++i) {
        col.constraints.oneOf.emplace_back(
            cc->one_of[i] ? std::string(cc->one_of[i]) : std::string());
      }
    }
    if (!std::isnan(cc->min_value))
      col.constraints.minValue = cc->min_value;
    if (!std::isnan(cc->max_value))
      col.constraints.maxValue = cc->max_value;
  }
  return col;
}

// C++ linkage implementation of make_cpp_column_from_c
static Column make_cpp_column_from_c(const KDB_TableColumn &c) {
  Column col;
  col.name = c.name ? std::string(c.name) : std::string();
  col.type = to_cpp_column_type(c.type);
  col.nullable = (c.nullable != 0);
  col.unique = (c.unique != 0);
  return col;
}

extern "C" {

const char *KadeDB_GetVersion() {
  // Safe to return string literal defined by CMake at configure time
  return KADEDB_VERSION;
}

int KadeDB_GetMajorVersion() { return KADEDB_VERSION_MAJOR; }
int KadeDB_GetMinorVersion() { return KADEDB_VERSION_MINOR; }
int KadeDB_GetPatchVersion() { return KADEDB_VERSION_PATCH; }

int KadeDB_Initialize() { return 1; }
void KadeDB_Shutdown() {}

int KadeDB_DocumentSchema_SetFieldFlags(KDB_DocumentSchema *schema,
                                        const char *field_name, int nullable,
                                        int unique) {
  if (!schema || !field_name)
    return 0;
  Column col;
  if (!schema->impl.getField(field_name, col))
    return 0;
  col.nullable = (nullable != 0);
  col.unique = (unique != 0);
  schema->impl.addField(std::move(col));
  return 1;
}

// (moved make_cpp_column_from_c_ex above, outside extern "C")

int KadeDB_ValidateRow(const KDB_TableColumnEx *columns,
                       unsigned long long column_count, const KDB_RowView *row,
                       char *err_buf, unsigned long long err_buf_len) {
  if (!columns || !row)
    return 0;
  // Build schema
  std::vector<Column> cols;
  cols.reserve(static_cast<size_t>(column_count));
  for (unsigned long long i = 0; i < column_count; ++i) {
    cols.emplace_back(make_cpp_column_from_c_ex(columns[i]));
  }
  TableSchema schema(std::move(cols));

  // Build row
  Row cppRow(static_cast<size_t>(row->count));
  for (unsigned long long i = 0; i < row->count; ++i) {
    const KDB_Value &v = row->values[i];
    if (v.type == KDB_VAL_NULL) {
      cppRow.set(static_cast<size_t>(i), nullptr);
    } else {
      cppRow.set(static_cast<size_t>(i), from_c_value(v));
    }
  }

  std::string err = SchemaValidator::validateRow(schema, cppRow);
  if (!err.empty()) {
    if (err_buf && err_buf_len > 0) {
      std::strncpy(err_buf, err.c_str(), static_cast<size_t>(err_buf_len - 1));
      err_buf[err_buf_len - 1] = '\0';
    }
    return 0;
  }
  return 1;
}

KDB_DocumentSchema *KadeDB_DocumentSchema_Create() {
  try {
    return new KDB_DocumentSchema{};
  } catch (...) {
    return nullptr;
  }
}

void KadeDB_DocumentSchema_Destroy(KDB_DocumentSchema *schema) {
  delete schema;
}

int KadeDB_DocumentSchema_AddField(KDB_DocumentSchema *schema, const char *name,
                                   KDB_ColumnType type, int nullable,
                                   int unique) {
  if (!schema || !name)
    return 0;
  Column c;
  c.name = std::string{name};
  c.type = to_cpp_column_type(type);
  c.nullable = (nullable != 0);
  c.unique = (unique != 0);
  // Return 0 on duplicate; our addField replaces, so emulate duplicate check
  if (schema->impl.hasField(c.name))
    return 0;
  schema->impl.addField(std::move(c));
  return 1;
}

int KadeDB_ValidateDocument(const KDB_DocumentSchema *schema,
                            const KDB_KeyValue *items, unsigned long long count,
                            char *err_buf, unsigned long long err_buf_len) {
  if (!schema)
    return 0;
  Document doc;
  doc.reserve(static_cast<size_t>(count));
  for (unsigned long long i = 0; i < count; ++i) {
    const auto &kv = items[i];
    if (!kv.key)
      continue;
    doc.emplace(std::string(kv.key), from_c_value(kv.value));
  }
  std::string err = SchemaValidator::validateDocument(schema->impl, doc);
  if (!err.empty()) {
    if (err_buf && err_buf_len > 0) {
      std::strncpy(err_buf, err.c_str(), static_cast<size_t>(err_buf_len - 1));
      err_buf[err_buf_len - 1] = '\0';
    }
    return 0;
  }
  return 1;
}

int KadeDB_ValidateUniqueDocuments(const KDB_DocumentSchema *schema,
                                   const KDB_DocumentView *docs,
                                   unsigned long long doc_count,
                                   int ignore_nulls, char *err_buf,
                                   unsigned long long err_buf_len) {
  if (!schema)
    return 0;
  std::vector<Document> cppDocs;
  cppDocs.reserve(static_cast<size_t>(doc_count));
  for (unsigned long long i = 0; i < doc_count; ++i) {
    Document d;
    const auto &dv = docs[i];
    for (unsigned long long j = 0; j < dv.count; ++j) {
      const auto &kv = dv.items[j];
      if (!kv.key)
        continue;
      d.emplace(std::string(kv.key), from_c_value(kv.value));
    }
    cppDocs.emplace_back(std::move(d));
  }
  std::string err =
      SchemaValidator::validateUnique(schema->impl, cppDocs, ignore_nulls != 0);
  if (!err.empty()) {
    if (err_buf && err_buf_len > 0) {
      std::strncpy(err_buf, err.c_str(), static_cast<size_t>(err_buf_len - 1));
      err_buf[err_buf_len - 1] = '\0';
    }
    return 0;
  }
  return 1;
}

int KadeDB_ValidateUniqueRows(const KDB_TableColumn *columns,
                              unsigned long long column_count,
                              const KDB_RowView *rows,
                              unsigned long long row_count, int ignore_nulls,
                              char *err_buf, unsigned long long err_buf_len) {
  if (!columns && column_count > 0)
    return 0;
  // Build TableSchema
  std::vector<Column> cols;
  cols.reserve(static_cast<size_t>(column_count));
  for (unsigned long long i = 0; i < column_count; ++i) {
    cols.emplace_back(make_cpp_column_from_c(columns[i]));
  }
  TableSchema schema(std::move(cols));

  // Convert rows
  std::vector<Row> cppRows;
  cppRows.reserve(static_cast<size_t>(row_count));
  for (unsigned long long r = 0; r < row_count; ++r) {
    const auto &rv = rows[r];
    Row row(static_cast<size_t>(rv.count));
    for (unsigned long long i = 0; i < rv.count; ++i) {
      // If value type is null, leave as nullptr to represent null
      if (rv.values[i].type == KDB_VAL_NULL) {
        row.set(static_cast<size_t>(i), nullptr);
      } else {
        row.set(static_cast<size_t>(i), from_c_value(rv.values[i]));
      }
    }
    cppRows.emplace_back(std::move(row));
  }

  std::string err =
      SchemaValidator::validateUnique(schema, cppRows, ignore_nulls != 0);
  if (!err.empty()) {
    if (err_buf && err_buf_len > 0) {
      std::strncpy(err_buf, err.c_str(), static_cast<size_t>(err_buf_len - 1));
      err_buf[err_buf_len - 1] = '\0';
    }
    return 0;
  }
  return 1;
}

int KadeDB_DocumentSchema_SetStringConstraints(
    KDB_DocumentSchema *schema, const char *field_name, long long min_len,
    long long max_len, const char *const *one_of,
    unsigned long long one_of_count) {
  if (!schema || !field_name)
    return 0;
  Column col;
  if (!schema->impl.getField(field_name, col))
    return 0;
  // Apply constraints
  if (min_len >= 0)
    col.constraints.minLength = static_cast<size_t>(min_len);
  else
    col.constraints.minLength.reset();
  if (max_len >= 0)
    col.constraints.maxLength = static_cast<size_t>(max_len);
  else
    col.constraints.maxLength.reset();
  col.constraints.oneOf.clear();
  if (one_of && one_of_count > 0) {
    col.constraints.oneOf.reserve(static_cast<size_t>(one_of_count));
    for (unsigned long long i = 0; i < one_of_count; ++i) {
      col.constraints.oneOf.emplace_back(one_of[i] ? std::string(one_of[i])
                                                   : std::string());
    }
  }
  schema->impl.addField(std::move(col));
  return 1;
}

int KadeDB_DocumentSchema_SetNumericConstraints(KDB_DocumentSchema *schema,
                                                const char *field_name,
                                                double min_value,
                                                double max_value) {
  if (!schema || !field_name)
    return 0;
  Column col;
  if (!schema->impl.getField(field_name, col))
    return 0;
  if (std::isnan(min_value))
    col.constraints.minValue.reset();
  else
    col.constraints.minValue = min_value;
  if (std::isnan(max_value))
    col.constraints.maxValue.reset();
  else
    col.constraints.maxValue = max_value;
  schema->impl.addField(std::move(col));
  return 1;
}
}

// ---------------- Result conversion & pagination (C API) ----------------

using namespace kadedb;

static ColumnType to_cpp_column_type(KDB_ColumnType t);

extern "C" int KadeDB_Result_ToCSVEx(
    const char *const *column_names, const KDB_ColumnType *types,
    unsigned long long column_count, const KDB_RowView *rows,
    unsigned long long row_count, char delimiter, int include_header,
    int always_quote, char quote_char, char *out_buf,
    unsigned long long out_buf_len, unsigned long long *out_required_len) {
  try {
    // Build ResultSet
    std::vector<std::string> cols;
    cols.reserve(static_cast<size_t>(column_count));
    for (unsigned long long i = 0; i < column_count; ++i) {
      cols.emplace_back(column_names && column_names[i]
                            ? std::string(column_names[i])
                            : std::string());
    }
    std::vector<ColumnType> ctypes;
    ctypes.reserve(static_cast<size_t>(column_count));
    for (unsigned long long i = 0; i < column_count; ++i) {
      ctypes.emplace_back(types ? to_cpp_column_type(types[i])
                                : ColumnType::Null);
    }
    ResultSet rs(std::move(cols), std::move(ctypes));
    for (unsigned long long r = 0; r < row_count; ++r) {
      const auto &rv = rows[r];
      std::vector<std::unique_ptr<Value>> vals;
      vals.reserve(static_cast<size_t>(column_count));
      for (unsigned long long c = 0; c < column_count; ++c) {
        if (c < rv.count)
          vals.emplace_back(from_c_value(rv.values[c]));
        else
          vals.emplace_back(ValueFactory::createNull());
      }
      rs.addRow(ResultRow(std::move(vals)));
    }

    std::string s =
        rs.toCSV(delimiter, include_header != 0, always_quote != 0, quote_char);
    unsigned long long need = static_cast<unsigned long long>(s.size()) + 1ULL;
    if (out_required_len)
      *out_required_len = need;
    if (!out_buf || out_buf_len == 0)
      return 1;
    // Write up to out_buf_len - 1 and NUL-terminate
    unsigned long long ncopy =
        (need <= out_buf_len) ? (need - 1ULL) : (out_buf_len - 1ULL);
    std::memcpy(out_buf, s.data(), static_cast<size_t>(ncopy));
    out_buf[ncopy] = '\0';
    return 1;
  } catch (...) {
    return 0;
  }
}

extern "C" int KadeDB_Result_ToCSV(const char *const *column_names,
                                   const KDB_ColumnType *types,
                                   unsigned long long column_count,
                                   const KDB_RowView *rows,
                                   unsigned long long row_count, char delimiter,
                                   int include_header, char *out_buf,
                                   unsigned long long out_buf_len,
                                   unsigned long long *out_required_len) {
  return KadeDB_Result_ToCSVEx(column_names, types, column_count, rows,
                               row_count, delimiter, include_header,
                               /*always_quote*/ 0, /*quote_char*/ '"', out_buf,
                               out_buf_len, out_required_len);
}

extern "C" int KadeDB_Result_ToJSONEx(
    const char *const *column_names, const KDB_ColumnType *types,
    unsigned long long column_count, const KDB_RowView *rows,
    unsigned long long row_count, int include_metadata, int indent,
    char *out_buf, unsigned long long out_buf_len,
    unsigned long long *out_required_len) {
  try {
    // Build ResultSet
    std::vector<std::string> cols;
    cols.reserve(static_cast<size_t>(column_count));
    for (unsigned long long i = 0; i < column_count; ++i) {
      cols.emplace_back(column_names && column_names[i]
                            ? std::string(column_names[i])
                            : std::string());
    }
    std::vector<ColumnType> ctypes;
    ctypes.reserve(static_cast<size_t>(column_count));
    for (unsigned long long i = 0; i < column_count; ++i) {
      ctypes.emplace_back(types ? to_cpp_column_type(types[i])
                                : ColumnType::Null);
    }
    ResultSet rs(std::move(cols), std::move(ctypes));
    for (unsigned long long r = 0; r < row_count; ++r) {
      const auto &rv = rows[r];
      std::vector<std::unique_ptr<Value>> vals;
      vals.reserve(static_cast<size_t>(column_count));
      for (unsigned long long c = 0; c < column_count; ++c) {
        if (c < rv.count)
          vals.emplace_back(from_c_value(rv.values[c]));
        else
          vals.emplace_back(ValueFactory::createNull());
      }
      rs.addRow(ResultRow(std::move(vals)));
    }
    if (indent < 0)
      indent = 0;
    std::string s = rs.toJSON(include_metadata != 0, indent);
    unsigned long long need = static_cast<unsigned long long>(s.size()) + 1ULL;
    if (out_required_len)
      *out_required_len = need;
    if (!out_buf || out_buf_len == 0)
      return 1;
    unsigned long long ncopy =
        (need <= out_buf_len) ? (need - 1ULL) : (out_buf_len - 1ULL);
    std::memcpy(out_buf, s.data(), static_cast<size_t>(ncopy));
    out_buf[ncopy] = '\0';
    return 1;
  } catch (...) {
    return 0;
  }
}

extern "C" int KadeDB_Result_ToJSON(
    const char *const *column_names, const KDB_ColumnType *types,
    unsigned long long column_count, const KDB_RowView *rows,
    unsigned long long row_count, int include_metadata, char *out_buf,
    unsigned long long out_buf_len, unsigned long long *out_required_len) {
  return KadeDB_Result_ToJSONEx(column_names, types, column_count, rows,
                                row_count, include_metadata, /*indent*/ 0,
                                out_buf, out_buf_len, out_required_len);
}

extern "C" int KadeDB_Paginate(unsigned long long total_rows,
                               unsigned long long page_size,
                               unsigned long long page_index,
                               unsigned long long *out_start,
                               unsigned long long *out_end,
                               unsigned long long *out_total_pages) {
  // Compute total pages
  unsigned long long tp = 0ULL;
  if (page_size == 0ULL) {
    tp = (total_rows == 0ULL) ? 0ULL : 1ULL;
  } else {
    tp = (total_rows + page_size - 1ULL) / page_size;
  }
  if (out_total_pages)
    *out_total_pages = tp;
  if (page_index >= tp)
    return 0;
  unsigned long long start = 0ULL, end = 0ULL;
  if (page_size == 0ULL) {
    start = 0ULL;
    end = total_rows;
  } else {
    start = page_index * page_size;
    unsigned long long maxEnd = start + page_size;
    end = maxEnd > total_rows ? total_rows : maxEnd;
  }
  if (out_start)
    *out_start = start;
  if (out_end)
    *out_end = end;
  return 1;
}

extern "C" int KadeDB_Paginate_TotalPages(unsigned long long total_rows,
                                          unsigned long long page_size,
                                          unsigned long long *out_total_pages) {
  if (!out_total_pages)
    return 0;
  unsigned long long tp = 0ULL;
  if (page_size == 0ULL)
    tp = (total_rows == 0ULL) ? 0ULL : 1ULL;
  else
    tp = (total_rows + page_size - 1ULL) / page_size;
  *out_total_pages = tp;
  return 1;
}

extern "C" int KadeDB_Paginate_Bounds(unsigned long long total_rows,
                                      unsigned long long page_size,
                                      unsigned long long page_index,
                                      unsigned long long *out_start,
                                      unsigned long long *out_end) {
  unsigned long long tp = 0ULL;
  if (page_size == 0ULL)
    tp = (total_rows == 0ULL) ? 0ULL : 1ULL;
  else
    tp = (total_rows + page_size - 1ULL) / page_size;
  if (page_index >= tp)
    return 0;
  unsigned long long start = 0ULL, end = 0ULL;
  if (page_size == 0ULL) {
    start = 0ULL;
    end = total_rows;
  } else {
    start = page_index * page_size;
    unsigned long long maxEnd = start + page_size;
    end = maxEnd > total_rows ? total_rows : maxEnd;
  }
  if (out_start)
    *out_start = start;
  if (out_end)
    *out_end = end;
  return 1;
}

// ---------------- Minimal Relational Storage C ABI ----------------

struct KadeDB_Storage {
  InMemoryRelationalStorage impl;
};

struct KadeDB_ResultSet {
  std::unique_ptr<ResultSet> impl;
  size_t cursor = static_cast<size_t>(-1);
  std::string scratch;
  std::string last_error;
};

extern "C" KadeDB_Storage *KadeDB_CreateStorage() {
  try {
    return new KadeDB_Storage{};
  } catch (...) {
    return nullptr;
  }
}

extern "C" void KadeDB_DestroyStorage(KadeDB_Storage *storage) {
  delete storage;
}

extern "C" int KadeDB_CreateTable(KadeDB_Storage *storage, const char *table,
                                  const KDB_TableSchema *schema) {
  if (!storage || !table || !schema)
    return 0;
  Status st = storage->impl.createTable(std::string{table}, schema->impl);
  return st.ok() ? 1 : 0;
}

extern "C" int KadeDB_InsertRow(KadeDB_Storage *storage, const char *table,
                                const KDB_RowView *row) {
  if (!storage || !table || !row)
    return 0;
  Row r(static_cast<size_t>(row->count));
  for (unsigned long long i = 0; i < row->count; ++i) {
    const KDB_Value &v = row->values[i];
    if (v.type == KDB_VAL_NULL)
      r.set(static_cast<size_t>(i), nullptr);
    else
      r.set(static_cast<size_t>(i), from_c_value(v));
  }
  Status st = storage->impl.insertRow(std::string{table}, r);
  return st.ok() ? 1 : 0;
}

// very small SELECT parser: supports only "SELECT * FROM <table>"
// (case-insensitive)
static std::string parse_select_star_from(const char *query) {
  if (!query)
    return {};
  // trim leading spaces
  const char *p = query;
  while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
    ++p;
  auto tolower_str = [](const std::string &s) {
    std::string o;
    o.reserve(s.size());
    for (char c : s)
      o.push_back(static_cast<char>(::tolower(static_cast<unsigned char>(c))));
    return o;
  };
  std::string s = tolower_str(p);
  const std::string prefix = "select * from ";
  if (s.rfind(prefix, 0) != 0)
    return {};
  std::string rest = s.substr(prefix.size());
  // trim trailing semicolon/spaces
  while (!rest.empty() &&
         (rest.back() == ' ' || rest.back() == '\t' || rest.back() == '\n' ||
          rest.back() == '\r' || rest.back() == ';'))
    rest.pop_back();
  // original case table name: extract from original query at equivalent
  // position
  size_t off = static_cast<size_t>(p - query) + prefix.size();
  std::string table = std::string(query + off, query + off + rest.size());
  // trim trailing whitespace in original
  while (!table.empty() &&
         (table.back() == ' ' || table.back() == '\t' || table.back() == '\n' ||
          table.back() == '\r' || table.back() == ';'))
    table.pop_back();
  return table;
}

extern "C" KadeDB_ResultSet *KadeDB_ExecuteQuery(KadeDB_Storage *storage,
                                                 const char *query) {
  if (!storage || !query)
    return nullptr;
  std::string table = parse_select_star_from(query);
  if (table.empty())
    return nullptr;
  auto res =
      storage->impl.select(table, /*columns*/ {}, /*where*/ std::nullopt);
  if (!res.hasValue())
    return nullptr;
  try {
    auto *out = new KadeDB_ResultSet{};
    out->impl = std::make_unique<ResultSet>(std::move(res.value()));
    out->cursor = static_cast<size_t>(-1);
    return out;
  } catch (...) {
    return nullptr;
  }
}

extern "C" int KadeDB_ResultSet_NextRow(KadeDB_ResultSet *rs) {
  if (!rs || !rs->impl)
    return 0;
  // emulate ResultSet::next over our own cursor
  if (rs->cursor + 1 < rs->impl->rowCount()) {
    ++rs->cursor;
    return 1;
  }
  return 0;
}

extern "C" const char *KadeDB_ResultSet_GetString(KadeDB_ResultSet *rs,
                                                  int column) {
  if (!rs || !rs->impl || rs->cursor >= rs->impl->rowCount() || column < 0)
    return nullptr;
  size_t col = static_cast<size_t>(column);
  if (col >= rs->impl->columnCount())
    return nullptr;
  try {
    rs->scratch = rs->impl->row(rs->cursor).toString(col);
    return rs->scratch.c_str();
  } catch (...) {
    return nullptr;
  }
}

extern "C" void KadeDB_DestroyResultSet(KadeDB_ResultSet *rs) { delete rs; }

extern "C" int KadeDB_ResultSet_Reset(KadeDB_ResultSet *rs) {
  if (!rs || !rs->impl)
    return 0;
  rs->cursor = static_cast<size_t>(-1);
  rs->last_error.clear();
  return 1;
}

extern "C" int KadeDB_ResultSet_ColumnCount(KadeDB_ResultSet *rs) {
  if (!rs || !rs->impl)
    return -1;
  try {
    return static_cast<int>(rs->impl->columnCount());
  } catch (...) {
    return -1;
  }
}

extern "C" const char *KadeDB_ResultSet_GetColumnName(KadeDB_ResultSet *rs,
                                                      int column) {
  if (!rs || !rs->impl || column < 0)
    return nullptr;
  size_t col = static_cast<size_t>(column);
  if (col >= rs->impl->columnCount())
    return nullptr;
  try {
    rs->scratch = rs->impl->columnNames().at(col);
    return rs->scratch.c_str();
  } catch (...) {
    return nullptr;
  }
}

extern "C" int KadeDB_ResultSet_GetColumnType(KadeDB_ResultSet *rs,
                                              int column) {
  if (!rs || !rs->impl || column < 0)
    return -1;
  size_t col = static_cast<size_t>(column);
  if (col >= rs->impl->columnCount())
    return -1;
  try {
    ColumnType ct = rs->impl->columnTypes().at(col);
    return static_cast<int>(ct);
  } catch (...) {
    return -1;
  }
}

extern "C" int KadeDB_ResultSet_FindColumn(KadeDB_ResultSet *rs,
                                           const char *name) {
  if (!rs || !rs->impl || !name)
    return -1;
  try {
    size_t idx = rs->impl->findColumn(std::string{name});
    if (idx == ResultSet::npos)
      return -1;
    return static_cast<int>(idx);
  } catch (...) {
    return -1;
  }
}

extern "C" long long KadeDB_ResultSet_GetInt64(KadeDB_ResultSet *rs, int column,
                                               int *ok) {
  if (ok)
    *ok = 0;
  if (!rs || !rs->impl || rs->cursor >= rs->impl->rowCount() || column < 0)
    return 0;
  size_t col = static_cast<size_t>(column);
  if (col >= rs->impl->columnCount())
    return 0;
  try {
    long long v = rs->impl->at(rs->cursor, col).asInt();
    if (ok)
      *ok = 1;
    return v;
  } catch (const std::exception &e) {
    rs->last_error = e.what();
  } catch (...) {
    rs->last_error = "unknown error";
  }
  return 0;
}

extern "C" double KadeDB_ResultSet_GetDouble(KadeDB_ResultSet *rs, int column,
                                             int *ok) {
  if (ok)
    *ok = 0;
  if (!rs || !rs->impl || rs->cursor >= rs->impl->rowCount() || column < 0)
    return 0.0;
  size_t col = static_cast<size_t>(column);
  if (col >= rs->impl->columnCount())
    return 0.0;
  try {
    double v = rs->impl->at(rs->cursor, col).asFloat();
    if (ok)
      *ok = 1;
    return v;
  } catch (const std::exception &e) {
    rs->last_error = e.what();
  } catch (...) {
    rs->last_error = "unknown error";
  }
  return 0.0;
}

extern "C" int KadeDB_ResultSet_GetBool(KadeDB_ResultSet *rs, int column,
                                        int *ok) {
  if (ok)
    *ok = 0;
  if (!rs || !rs->impl || rs->cursor >= rs->impl->rowCount() || column < 0)
    return 0;
  size_t col = static_cast<size_t>(column);
  if (col >= rs->impl->columnCount())
    return 0;
  try {
    int v = rs->impl->at(rs->cursor, col).asBool() ? 1 : 0;
    if (ok)
      *ok = 1;
    return v;
  } catch (const std::exception &e) {
    rs->last_error = e.what();
  } catch (...) {
    rs->last_error = "unknown error";
  }
  return 0;
}

extern "C" const char *KadeDB_ResultSet_GetLastError(KadeDB_ResultSet *rs) {
  if (!rs)
    return nullptr;
  return rs->last_error.empty() ? nullptr : rs->last_error.c_str();
}

extern "C" int KadeDB_UpdateRows(KadeDB_Storage *storage, const char *table,
                                 const KDB_Assignment *assignments,
                                 unsigned long long assignment_count,
                                 const KDB_Predicate *where_predicate,
                                 unsigned long long *out_updated) {
  if (!storage || !table || !assignments || assignment_count == 0ULL)
    return 0;
  std::unordered_map<std::string, AssignmentValue> asg;
  asg.reserve(static_cast<size_t>(assignment_count));
  for (unsigned long long i = 0; i < assignment_count; ++i) {
    const KDB_Assignment &a = assignments[i];
    if (!a.column)
      return 0;
    AssignmentValue av;
    if (a.is_column_ref != 0) {
      if (!a.column_ref)
        return 0;
      av.kind = AssignmentValue::Kind::ColumnRef;
      av.column_ref = std::string{a.column_ref};
    } else {
      av.kind = AssignmentValue::Kind::Constant;
      av.constant = from_c_value(a.constant);
    }
    asg.emplace(std::string{a.column}, std::move(av));
  }
  auto where = to_cpp_predicate(where_predicate);
  auto res = storage->impl.updateRows(std::string{table}, asg, where);
  if (!res.hasValue())
    return 0;
  if (out_updated)
    *out_updated = static_cast<unsigned long long>(res.value());
  return 1;
}

extern "C" int KadeDB_DeleteRows(KadeDB_Storage *storage, const char *table,
                                 const KDB_Predicate *where_predicate,
                                 unsigned long long *out_deleted) {
  if (!storage || !table)
    return 0;
  auto where = to_cpp_predicate(where_predicate);
  auto res = storage->impl.deleteRows(std::string{table}, where);
  if (!res.hasValue())
    return 0;
  if (out_deleted)
    *out_deleted = static_cast<unsigned long long>(res.value());
  return 1;
}

extern "C" int KadeDB_DropTable(KadeDB_Storage *storage, const char *table) {
  if (!storage || !table)
    return 0;
  Status st = storage->impl.dropTable(std::string{table});
  return st.ok() ? 1 : 0;
}

extern "C" int KadeDB_TruncateTable(KadeDB_Storage *storage,
                                    const char *table) {
  if (!storage || !table)
    return 0;
  Status st = storage->impl.truncateTable(std::string{table});
  return st.ok() ? 1 : 0;
}

extern "C" int KadeDB_ListTables_ToCSV(KadeDB_Storage *storage, char delimiter,
                                       char *out_buf,
                                       unsigned long long out_buf_len,
                                       unsigned long long *out_required_len) {
  if (!storage)
    return 0;
  std::vector<std::string> names = storage->impl.listTables();
  // Build delimited string
  size_t total = 0;
  for (size_t i = 0; i < names.size(); ++i) {
    total += names[i].size();
    if (i + 1 < names.size())
      total += 1; // delimiter
  }
  unsigned long long need = static_cast<unsigned long long>(total) + 1ULL;
  if (out_required_len)
    *out_required_len = need;
  if (!out_buf || out_buf_len == 0)
    return 1;
  unsigned long long ncopy =
      (need <= out_buf_len) ? (need - 1ULL) : (out_buf_len - 1ULL);
  // Write with truncation if needed
  unsigned long long written = 0;
  for (size_t i = 0; i < names.size(); ++i) {
    const std::string &s = names[i];
    for (char c : s) {
      if (written >= ncopy)
        break;
      out_buf[written++] = c;
    }
    if (i + 1 < names.size() && written < ncopy) {
      out_buf[written++] = delimiter;
    }
    if (written >= ncopy)
      break;
  }
  out_buf[written] = '\0';
  return 1;
}
