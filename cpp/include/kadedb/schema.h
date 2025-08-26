#pragma once

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "kadedb/value.h"

namespace kadedb {

// Map logical column types to ValueType for simplicity
enum class ColumnType {
  Null = static_cast<int>(ValueType::Null),
  Integer = static_cast<int>(ValueType::Integer),
  Float = static_cast<int>(ValueType::Float),
  String = static_cast<int>(ValueType::String),
  Boolean = static_cast<int>(ValueType::Boolean),
};

struct Column {
  std::string name;
  ColumnType type;
  bool nullable = true;
  bool unique = false;
  // Optional richer constraints (simple, storage-agnostic)
  struct ColumnConstraints {
    // For String
    std::optional<size_t> minLength;
    std::optional<size_t> maxLength;
    std::vector<std::string> oneOf; // allowed set for String

    // For numeric (Integer/Float)
    std::optional<double> minValue; // inclusive
    std::optional<double> maxValue; // inclusive
  } constraints;
};

class TableSchema {
public:
  TableSchema() = default;
  explicit TableSchema(std::vector<Column> cols, std::optional<std::string> primaryKey = std::nullopt);

  const std::vector<Column>& columns() const { return columns_; }
  const std::optional<std::string>& primaryKey() const { return primaryKey_; }

  // Lookup by name; returns index or npos if not found
  size_t findColumn(const std::string& name) const;
  static constexpr size_t npos = static_cast<size_t>(-1);

  // Modification helpers
  // Returns false if a column with the same name already exists
  bool addColumn(const Column& col);
  // Returns false if the column does not exist
  bool removeColumn(const std::string& name);
  // Retrieve a copy of the column definition; returns false if not found
  bool getColumn(const std::string& name, Column& out) const;
  // Update an existing column by name; returns false if not found
  bool updateColumn(const Column& col);
  // Set or clear primary key. Throws if the column doesn't exist when set.
  void setPrimaryKey(std::optional<std::string> primaryKey);

private:
  std::vector<Column> columns_;
  std::unordered_map<std::string, size_t> indexByName_;
  std::optional<std::string> primaryKey_;
};

// A simple row representation that aligns with a TableSchema
class Row {
public:
  explicit Row(size_t columnCount = 0);

  size_t size() const { return values_.size(); }
  const Value& at(size_t idx) const { return *values_.at(idx); }
  Value& at(size_t idx) { return *values_.at(idx); }

  void set(size_t idx, std::unique_ptr<Value> v);
  const std::vector<std::unique_ptr<Value>>& values() const { return values_; }

private:
  std::vector<std::unique_ptr<Value>> values_;
};

// A flexible document schema keyed by field name.
class DocumentSchema {
public:
  DocumentSchema() = default;

  // Add or replace a field definition
  void addField(Column field);
  // Remove a field if present; returns false if not found
  bool removeField(const std::string& name);

  const std::unordered_map<std::string, Column>& fields() const { return fields_; }
  bool hasField(const std::string& name) const { return fields_.find(name) != fields_.end(); }
  // Retrieve a copy of the field definition; returns false if not found
  bool getField(const std::string& name, Column& out) const;

private:
  std::unordered_map<std::string, Column> fields_;
};

// A simple in-memory document representation
using Document = std::unordered_map<std::string, std::unique_ptr<Value>>;

// Minimal validation utility
class SchemaValidator {
public:
  // Returns empty string on success, otherwise an error message
  static std::string validateRow(const TableSchema& schema, const Row& row);

  // Validate a document against a DocumentSchema. Flexible: unknown fields are allowed.
  static std::string validateDocument(const DocumentSchema& schema, const Document& doc);

  // Uniqueness/richer constraints (in-memory checks)
  // Ensures columns with unique=true do not have duplicate non-null values across rows
  static std::string validateUnique(const TableSchema& schema, const std::vector<Row>& rows,
                                    bool ignoreNulls = true);
  // Ensures fields with unique=true do not have duplicate non-null values across documents
  static std::string validateUnique(const DocumentSchema& schema, const std::vector<Document>& docs,
                                    bool ignoreNulls = true);

private:
  static bool valueMatches(ColumnType ct, const Value& v);
  static bool checkConstraints(const Column& col, const Value& v, std::string& err);
};

} // namespace kadedb
