#include "kadedb/version.h"
#include "kadedb/kadedb.h"

#include "kadedb/schema.h"
#include "kadedb/value.h"
#include "kadedb/result.h"

#include <cstring>
#include <cmath>
#include <memory>
#include <string>
#include <unordered_map>

using namespace kadedb;

// Forward declarations for helpers used before their definitions
static ColumnType to_cpp_column_type(KDB_ColumnType t);
static std::unique_ptr<Value> from_c_value(const KDB_Value& v);
static Column make_cpp_column_from_c_ex(const KDB_TableColumnEx& cex);
static Column make_cpp_column_from_c(const KDB_TableColumn& c);
static Column make_cpp_column_from_ex_ptr(const KDB_TableColumnEx* cex);

// Define wrapper structs before use
struct KDB_DocumentSchema {
  DocumentSchema impl;
};

struct KDB_TableSchema {
  TableSchema impl;
};

extern "C" KDB_TableSchema* KadeDB_TableSchema_Create() {
  try {
    return new KDB_TableSchema{};
  } catch (...) {
    return nullptr;
  }
}

extern "C" void KadeDB_TableSchema_Destroy(KDB_TableSchema* schema) {
  delete schema;
}

static Column make_cpp_column_from_ex_ptr(const KDB_TableColumnEx* cex) {
  return make_cpp_column_from_c_ex(*cex);
}

extern "C" int KadeDB_TableSchema_AddColumn(KDB_TableSchema* schema, const KDB_TableColumnEx* column) {
  if (!schema || !column || !column->name) return 0;
  Column col = make_cpp_column_from_ex_ptr(column);
  return schema->impl.addColumn(col) ? 1 : 0;
}

extern "C" int KadeDB_TableSchema_RemoveColumn(KDB_TableSchema* schema, const char* name) {
  if (!schema || !name) return 0;
  return schema->impl.removeColumn(name) ? 1 : 0;
}

extern "C" int KadeDB_TableSchema_SetColumnFlags(KDB_TableSchema* schema, const char* name, int nullable, int unique) {
  if (!schema || !name) return 0;
  Column col;
  if (!schema->impl.getColumn(name, col)) return 0;
  col.nullable = (nullable != 0);
  col.unique = (unique != 0);
  return schema->impl.updateColumn(col) ? 1 : 0;
}

extern "C" int KadeDB_TableSchema_SetStringConstraints(KDB_TableSchema* schema,
                                            const char* name,
                                            long long min_len,
                                            long long max_len,
                                            const char* const* one_of,
                                            unsigned long long one_of_count) {
  if (!schema || !name) return 0;
  Column col;
  if (!schema->impl.getColumn(name, col)) return 0;
  if (min_len >= 0) col.constraints.minLength = static_cast<size_t>(min_len);
  else col.constraints.minLength.reset();
  if (max_len >= 0) col.constraints.maxLength = static_cast<size_t>(max_len);
  else col.constraints.maxLength.reset();
  col.constraints.oneOf.clear();
  if (one_of && one_of_count > 0) {
    col.constraints.oneOf.reserve(static_cast<size_t>(one_of_count));
    for (unsigned long long i = 0; i < one_of_count; ++i) {
      col.constraints.oneOf.emplace_back(one_of[i] ? std::string(one_of[i]) : std::string());
    }
  }
  return schema->impl.updateColumn(col) ? 1 : 0;
}

extern "C" int KadeDB_TableSchema_SetNumericConstraints(KDB_TableSchema* schema,
                                             const char* name,
                                             double min_value,
                                             double max_value) {
  if (!schema || !name) return 0;
  Column col;
  if (!schema->impl.getColumn(name, col)) return 0;
  if (std::isnan(min_value)) col.constraints.minValue.reset();
  else col.constraints.minValue = min_value;
  if (std::isnan(max_value)) col.constraints.maxValue.reset();
  else col.constraints.maxValue = max_value;
  return schema->impl.updateColumn(col) ? 1 : 0;
}

extern "C" int KadeDB_TableSchema_SetPrimaryKey(KDB_TableSchema* schema, const char* name) {
  if (!schema) return 0;
  try {
    if (name) schema->impl.setPrimaryKey(std::string{name});
    else schema->impl.setPrimaryKey(std::nullopt);
    return 1;
  } catch (...) {
    return 0;
  }
}

extern "C" int KadeDB_TableSchema_ValidateRow(const KDB_TableSchema* schema,
                                   const KDB_RowView* row,
                                   char* err_buf,
                                   unsigned long long err_buf_len) {
  if (!schema || !row) return 0;
  Row cppRow(static_cast<size_t>(row->count));
  for (unsigned long long i = 0; i < row->count; ++i) {
    const KDB_Value& v = row->values[i];
    if (v.type == KDB_VAL_NULL) cppRow.set(static_cast<size_t>(i), nullptr);
    else cppRow.set(static_cast<size_t>(i), from_c_value(v));
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

extern "C" int KadeDB_TableSchema_ValidateUniqueRows(const KDB_TableSchema* schema,
                                          const KDB_RowView* rows,
                                          unsigned long long row_count,
                                          int ignore_nulls,
                                          char* err_buf,
                                          unsigned long long err_buf_len) {
  if (!schema || (!rows && row_count > 0)) return 0;
  std::vector<Row> cppRows;
  cppRows.reserve(static_cast<size_t>(row_count));
  const size_t colCount = schema->impl.columns().size();
  for (unsigned long long r = 0; r < row_count; ++r) {
    Row cppRow(colCount);
    const KDB_RowView& rv = rows[r];
    for (unsigned long long i = 0; i < rv.count && i < colCount; ++i) {
      if (rv.values[i].type == KDB_VAL_NULL) cppRow.set(static_cast<size_t>(i), nullptr);
      else cppRow.set(static_cast<size_t>(i), from_c_value(rv.values[i]));
    }
    cppRows.emplace_back(std::move(cppRow));
  }
  std::string err = SchemaValidator::validateUnique(schema->impl, cppRows, ignore_nulls != 0);
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
    case KDB_COL_NULL: return ColumnType::Null;
    case KDB_COL_INTEGER: return ColumnType::Integer;
    case KDB_COL_FLOAT: return ColumnType::Float;
    case KDB_COL_STRING: return ColumnType::String;
    case KDB_COL_BOOLEAN: return ColumnType::Boolean;
  }
  return ColumnType::Null;
}

static std::unique_ptr<Value> from_c_value(const KDB_Value& v) {
  switch (v.type) {
    case KDB_VAL_NULL: return ValueFactory::createNull();
    case KDB_VAL_INTEGER: return ValueFactory::createInteger(static_cast<int64_t>(v.as.i64));
    case KDB_VAL_FLOAT: return ValueFactory::createFloat(v.as.f64);
    case KDB_VAL_STRING: return ValueFactory::createString(v.as.str ? std::string(v.as.str) : std::string());
    case KDB_VAL_BOOLEAN: return ValueFactory::createBoolean(v.as.boolean != 0);
  }
  return ValueFactory::createNull();
}

// Expose make_cpp_column_from_c_ex with C++ linkage (used by other helpers)
static Column make_cpp_column_from_c_ex(const KDB_TableColumnEx& cex) {
  Column col;
  col.name = cex.name ? std::string(cex.name) : std::string();
  col.type = to_cpp_column_type(cex.type);
  col.nullable = (cex.nullable != 0);
  col.unique = (cex.unique != 0);
  if (cex.constraints) {
    const auto* cc = cex.constraints;
    if (cc->min_len >= 0) col.constraints.minLength = static_cast<size_t>(cc->min_len);
    if (cc->max_len >= 0) col.constraints.maxLength = static_cast<size_t>(cc->max_len);
    if (cc->one_of && cc->one_of_count > 0) {
      col.constraints.oneOf.reserve(static_cast<size_t>(cc->one_of_count));
      for (unsigned long long i = 0; i < cc->one_of_count; ++i) {
        col.constraints.oneOf.emplace_back(cc->one_of[i] ? std::string(cc->one_of[i]) : std::string());
      }
    }
    if (!std::isnan(cc->min_value)) col.constraints.minValue = cc->min_value;
    if (!std::isnan(cc->max_value)) col.constraints.maxValue = cc->max_value;
  }
  return col;
}

// C++ linkage implementation of make_cpp_column_from_c
static Column make_cpp_column_from_c(const KDB_TableColumn& c) {
  Column col;
  col.name = c.name ? std::string(c.name) : std::string();
  col.type = to_cpp_column_type(c.type);
  col.nullable = (c.nullable != 0);
  col.unique = (c.unique != 0);
  return col;
}

extern "C" {

const char* KadeDB_GetVersion() {
  // Safe to return string literal defined by CMake at configure time
  return KADEDB_VERSION;
}

int KadeDB_DocumentSchema_SetFieldFlags(KDB_DocumentSchema* schema,
                                        const char* field_name,
                                        int nullable,
                                        int unique) {
  if (!schema || !field_name) return 0;
  Column col;
  if (!schema->impl.getField(field_name, col)) return 0;
  col.nullable = (nullable != 0);
  col.unique = (unique != 0);
  schema->impl.addField(std::move(col));
  return 1;
}

// (moved make_cpp_column_from_c_ex above, outside extern "C")

int KadeDB_ValidateRow(const KDB_TableColumnEx* columns,
                       unsigned long long column_count,
                       const KDB_RowView* row,
                       char* err_buf,
                       unsigned long long err_buf_len) {
  if (!columns || !row) return 0;
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
    const KDB_Value& v = row->values[i];
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

KDB_DocumentSchema* KadeDB_DocumentSchema_Create() {
  try {
    return new KDB_DocumentSchema{};
  } catch (...) {
    return nullptr;
  }
}

void KadeDB_DocumentSchema_Destroy(KDB_DocumentSchema* schema) {
  delete schema;
}

int KadeDB_DocumentSchema_AddField(KDB_DocumentSchema* schema,
                                   const char* name,
                                   KDB_ColumnType type,
                                   int nullable,
                                   int unique) {
  if (!schema || !name) return 0;
  Column c{std::string{name}, to_cpp_column_type(type), nullable != 0, unique != 0};
  // Return 0 on duplicate; our addField replaces, so emulate duplicate check
  if (schema->impl.hasField(c.name)) return 0;
  schema->impl.addField(std::move(c));
  return 1;
}

 

int KadeDB_ValidateDocument(const KDB_DocumentSchema* schema,
                            const KDB_KeyValue* items,
                            unsigned long long count,
                            char* err_buf,
                            unsigned long long err_buf_len) {
  if (!schema) return 0;
  Document doc;
  doc.reserve(static_cast<size_t>(count));
  for (unsigned long long i = 0; i < count; ++i) {
    const auto& kv = items[i];
    if (!kv.key) continue;
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

int KadeDB_ValidateUniqueDocuments(const KDB_DocumentSchema* schema,
                                   const KDB_DocumentView* docs,
                                   unsigned long long doc_count,
                                   int ignore_nulls,
                                   char* err_buf,
                                   unsigned long long err_buf_len) {
  if (!schema) return 0;
  std::vector<Document> cppDocs;
  cppDocs.reserve(static_cast<size_t>(doc_count));
  for (unsigned long long i = 0; i < doc_count; ++i) {
    Document d;
    const auto& dv = docs[i];
    for (unsigned long long j = 0; j < dv.count; ++j) {
      const auto& kv = dv.items[j];
      if (!kv.key) continue;
      d.emplace(std::string(kv.key), from_c_value(kv.value));
    }
    cppDocs.emplace_back(std::move(d));
  }
  std::string err = SchemaValidator::validateUnique(schema->impl, cppDocs, ignore_nulls != 0);
  if (!err.empty()) {
    if (err_buf && err_buf_len > 0) {
      std::strncpy(err_buf, err.c_str(), static_cast<size_t>(err_buf_len - 1));
      err_buf[err_buf_len - 1] = '\0';
    }
    return 0;
  }
  return 1;
}


int KadeDB_ValidateUniqueRows(const KDB_TableColumn* columns,
                              unsigned long long column_count,
                              const KDB_RowView* rows,
                              unsigned long long row_count,
                              int ignore_nulls,
                              char* err_buf,
                              unsigned long long err_buf_len) {
  if (!columns && column_count > 0) return 0;
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
    const auto& rv = rows[r];
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

  std::string err = SchemaValidator::validateUnique(schema, cppRows, ignore_nulls != 0);
  if (!err.empty()) {
    if (err_buf && err_buf_len > 0) {
      std::strncpy(err_buf, err.c_str(), static_cast<size_t>(err_buf_len - 1));
      err_buf[err_buf_len - 1] = '\0';
    }
    return 0;
  }
  return 1;
}

int KadeDB_DocumentSchema_SetStringConstraints(KDB_DocumentSchema* schema,
                                               const char* field_name,
                                               long long min_len,
                                               long long max_len,
                                               const char* const* one_of,
                                               unsigned long long one_of_count) {
  if (!schema || !field_name) return 0;
  Column col;
  if (!schema->impl.getField(field_name, col)) return 0;
  // Apply constraints
  if (min_len >= 0) col.constraints.minLength = static_cast<size_t>(min_len);
  else col.constraints.minLength.reset();
  if (max_len >= 0) col.constraints.maxLength = static_cast<size_t>(max_len);
  else col.constraints.maxLength.reset();
  col.constraints.oneOf.clear();
  if (one_of && one_of_count > 0) {
    col.constraints.oneOf.reserve(static_cast<size_t>(one_of_count));
    for (unsigned long long i = 0; i < one_of_count; ++i) {
      col.constraints.oneOf.emplace_back(one_of[i] ? std::string(one_of[i]) : std::string());
    }
  }
  schema->impl.addField(std::move(col));
  return 1;
}

int KadeDB_DocumentSchema_SetNumericConstraints(KDB_DocumentSchema* schema,
                                                const char* field_name,
                                                double min_value,
                                                double max_value) {
  if (!schema || !field_name) return 0;
  Column col;
  if (!schema->impl.getField(field_name, col)) return 0;
  if (std::isnan(min_value)) col.constraints.minValue.reset();
  else col.constraints.minValue = min_value;
  if (std::isnan(max_value)) col.constraints.maxValue.reset();
  else col.constraints.maxValue = max_value;
  schema->impl.addField(std::move(col));
  return 1;
}

}

// ---------------- Result conversion & pagination (C API) ----------------

using namespace kadedb;

static ColumnType to_cpp_column_type(KDB_ColumnType t);

extern "C" int KadeDB_Result_ToCSVEx(const char* const* column_names,
                                     const KDB_ColumnType* types,
                                     unsigned long long column_count,
                                     const KDB_RowView* rows,
                                     unsigned long long row_count,
                                     char delimiter,
                                     int include_header,
                                     int always_quote,
                                     char quote_char,
                                     char* out_buf,
                                     unsigned long long out_buf_len,
                                     unsigned long long* out_required_len) {
  try {
    // Build ResultSet
    std::vector<std::string> cols;
    cols.reserve(static_cast<size_t>(column_count));
    for (unsigned long long i = 0; i < column_count; ++i) {
      cols.emplace_back(column_names && column_names[i] ? std::string(column_names[i]) : std::string());
    }
    std::vector<ColumnType> ctypes;
    ctypes.reserve(static_cast<size_t>(column_count));
    for (unsigned long long i = 0; i < column_count; ++i) {
      ctypes.emplace_back(types ? to_cpp_column_type(types[i]) : ColumnType::Null);
    }
    ResultSet rs(std::move(cols), std::move(ctypes));
    for (unsigned long long r = 0; r < row_count; ++r) {
      const auto& rv = rows[r];
      std::vector<std::unique_ptr<Value>> vals;
      vals.reserve(static_cast<size_t>(column_count));
      for (unsigned long long c = 0; c < column_count; ++c) {
        if (c < rv.count) vals.emplace_back(from_c_value(rv.values[c]));
        else vals.emplace_back(ValueFactory::createNull());
      }
      rs.addRow(ResultRow(std::move(vals)));
    }

    std::string s = rs.toCSV(delimiter, include_header != 0, always_quote != 0, quote_char);
    unsigned long long need = static_cast<unsigned long long>(s.size()) + 1ULL;
    if (out_required_len) *out_required_len = need;
    if (!out_buf || out_buf_len == 0) return 1;
    // Write up to out_buf_len - 1 and NUL-terminate
    unsigned long long ncopy = (need <= out_buf_len) ? (need - 1ULL) : (out_buf_len - 1ULL);
    std::memcpy(out_buf, s.data(), static_cast<size_t>(ncopy));
    out_buf[ncopy] = '\0';
    return 1;
  } catch (...) {
    return 0;
  }
}

extern "C" int KadeDB_Result_ToCSV(const char* const* column_names,
                                    const KDB_ColumnType* types,
                                    unsigned long long column_count,
                                    const KDB_RowView* rows,
                                    unsigned long long row_count,
                                    char delimiter,
                                    int include_header,
                                    char* out_buf,
                                    unsigned long long out_buf_len,
                                    unsigned long long* out_required_len) {
  return KadeDB_Result_ToCSVEx(column_names, types, column_count, rows, row_count,
                               delimiter, include_header, /*always_quote*/0, /*quote_char*/'"',
                               out_buf, out_buf_len, out_required_len);
}

extern "C" int KadeDB_Result_ToJSONEx(const char* const* column_names,
                                      const KDB_ColumnType* types,
                                      unsigned long long column_count,
                                      const KDB_RowView* rows,
                                      unsigned long long row_count,
                                      int include_metadata,
                                      int indent,
                                      char* out_buf,
                                      unsigned long long out_buf_len,
                                      unsigned long long* out_required_len) {
  try {
    // Build ResultSet
    std::vector<std::string> cols;
    cols.reserve(static_cast<size_t>(column_count));
    for (unsigned long long i = 0; i < column_count; ++i) {
      cols.emplace_back(column_names && column_names[i] ? std::string(column_names[i]) : std::string());
    }
    std::vector<ColumnType> ctypes;
    ctypes.reserve(static_cast<size_t>(column_count));
    for (unsigned long long i = 0; i < column_count; ++i) {
      ctypes.emplace_back(types ? to_cpp_column_type(types[i]) : ColumnType::Null);
    }
    ResultSet rs(std::move(cols), std::move(ctypes));
    for (unsigned long long r = 0; r < row_count; ++r) {
      const auto& rv = rows[r];
      std::vector<std::unique_ptr<Value>> vals;
      vals.reserve(static_cast<size_t>(column_count));
      for (unsigned long long c = 0; c < column_count; ++c) {
        if (c < rv.count) vals.emplace_back(from_c_value(rv.values[c]));
        else vals.emplace_back(ValueFactory::createNull());
      }
      rs.addRow(ResultRow(std::move(vals)));
    }
    if (indent < 0) indent = 0;
    std::string s = rs.toJSON(include_metadata != 0, indent);
    unsigned long long need = static_cast<unsigned long long>(s.size()) + 1ULL;
    if (out_required_len) *out_required_len = need;
    if (!out_buf || out_buf_len == 0) return 1;
    unsigned long long ncopy = (need <= out_buf_len) ? (need - 1ULL) : (out_buf_len - 1ULL);
    std::memcpy(out_buf, s.data(), static_cast<size_t>(ncopy));
    out_buf[ncopy] = '\0';
    return 1;
  } catch (...) {
    return 0;
  }
}

extern "C" int KadeDB_Result_ToJSON(const char* const* column_names,
                                     const KDB_ColumnType* types,
                                     unsigned long long column_count,
                                     const KDB_RowView* rows,
                                     unsigned long long row_count,
                                     int include_metadata,
                                     char* out_buf,
                                     unsigned long long out_buf_len,
                                     unsigned long long* out_required_len) {
  return KadeDB_Result_ToJSONEx(column_names, types, column_count, rows, row_count,
                                include_metadata, /*indent*/0,
                                out_buf, out_buf_len, out_required_len);
}

extern "C" int KadeDB_Paginate(unsigned long long total_rows,
                                 unsigned long long page_size,
                                 unsigned long long page_index,
                                 unsigned long long* out_start,
                                 unsigned long long* out_end,
                                 unsigned long long* out_total_pages) {
  // Compute total pages
  unsigned long long tp = 0ULL;
  if (page_size == 0ULL) {
    tp = (total_rows == 0ULL) ? 0ULL : 1ULL;
  } else {
    tp = (total_rows + page_size - 1ULL) / page_size;
  }
  if (out_total_pages) *out_total_pages = tp;
  if (page_index >= tp) return 0;
  unsigned long long start = 0ULL, end = 0ULL;
  if (page_size == 0ULL) {
    start = 0ULL;
    end = total_rows;
  } else {
    start = page_index * page_size;
    unsigned long long maxEnd = start + page_size;
    end = maxEnd > total_rows ? total_rows : maxEnd;
  }
  if (out_start) *out_start = start;
  if (out_end) *out_end = end;
  return 1;
}

extern "C" int KadeDB_Paginate_TotalPages(unsigned long long total_rows,
                                           unsigned long long page_size,
                                           unsigned long long* out_total_pages) {
  if (!out_total_pages) return 0;
  unsigned long long tp = 0ULL;
  if (page_size == 0ULL) tp = (total_rows == 0ULL) ? 0ULL : 1ULL;
  else tp = (total_rows + page_size - 1ULL) / page_size;
  *out_total_pages = tp;
  return 1;
}

extern "C" int KadeDB_Paginate_Bounds(unsigned long long total_rows,
                                       unsigned long long page_size,
                                       unsigned long long page_index,
                                       unsigned long long* out_start,
                                       unsigned long long* out_end) {
  unsigned long long tp = 0ULL;
  if (page_size == 0ULL) tp = (total_rows == 0ULL) ? 0ULL : 1ULL;
  else tp = (total_rows + page_size - 1ULL) / page_size;
  if (page_index >= tp) return 0;
  unsigned long long start = 0ULL, end = 0ULL;
  if (page_size == 0ULL) { start = 0ULL; end = total_rows; }
  else {
    start = page_index * page_size;
    unsigned long long maxEnd = start + page_size;
    end = maxEnd > total_rows ? total_rows : maxEnd;
  }
  if (out_start) *out_start = start;
  if (out_end) *out_end = end;
  return 1;
}
