#include "kadedb/schema.h"

#include <stdexcept>

namespace kadedb {

TableSchema::TableSchema(std::vector<Column> cols, std::optional<std::string> primaryKey)
    : columns_(std::move(cols)), primaryKey_(std::move(primaryKey)) {
  indexByName_.reserve(columns_.size());
  for (size_t i = 0; i < columns_.size(); ++i) {
    indexByName_.emplace(columns_[i].name, i);
  }
  if (primaryKey_) {
    if (indexByName_.find(*primaryKey_) == indexByName_.end()) {
      throw std::invalid_argument("Primary key column not found in schema: " + *primaryKey_);
    }
  }
}

size_t TableSchema::findColumn(const std::string& name) const {
  auto it = indexByName_.find(name);
  return it == indexByName_.end() ? npos : it->second;
}

Row::Row(size_t columnCount) : values_(columnCount) {}

void Row::set(size_t idx, std::unique_ptr<Value> v) {
  if (idx >= values_.size()) {
    throw std::out_of_range("Row::set index out of range");
  }
  values_[idx] = std::move(v);
}

bool SchemaValidator::valueMatches(ColumnType ct, const Value& v) {
  switch (ct) {
    case ColumnType::Null: return v.type() == ValueType::Null;
    case ColumnType::Integer: return v.type() == ValueType::Integer;
    case ColumnType::Float: return v.type() == ValueType::Float || v.type() == ValueType::Integer;
    case ColumnType::String: return v.type() == ValueType::String;
    case ColumnType::Boolean: return v.type() == ValueType::Boolean;
  }
  return false;
}

std::string SchemaValidator::validateRow(const TableSchema& schema, const Row& row) {
  const auto& cols = schema.columns();
  if (row.size() != cols.size()) {
    return "Row size does not match schema column count";
  }
  for (size_t i = 0; i < cols.size(); ++i) {
    const auto& col = cols[i];
    const auto& val = row.values()[i];
    if (!val) {
      if (!col.nullable) {
        return "Non-nullable column '" + col.name + "' has null value";
      }
      continue;
    }
    if (!valueMatches(col.type, *val)) {
      return "Value type does not match column '" + col.name + "'";
    }
  }
  return {};
}

} // namespace kadedb
