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

// Minimal validation utility
class SchemaValidator {
public:
  // Returns empty string on success, otherwise an error message
  static std::string validateRow(const TableSchema& schema, const Row& row);

private:
  static bool valueMatches(ColumnType ct, const Value& v);
};

} // namespace kadedb
