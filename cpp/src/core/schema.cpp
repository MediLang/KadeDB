#include "kadedb/schema.h"

#include <stdexcept>
#include <unordered_map>

namespace kadedb {

TableSchema::TableSchema(std::vector<Column> cols,
                         std::optional<std::string> primaryKey)
    : columns_(std::move(cols)), primaryKey_(std::move(primaryKey)) {
  indexByName_.reserve(columns_.size());
  for (size_t i = 0; i < columns_.size(); ++i) {
    indexByName_.emplace(columns_[i].name, i);
  }
  if (primaryKey_) {
    if (indexByName_.find(*primaryKey_) == indexByName_.end()) {
      throw std::invalid_argument("Primary key column not found in schema: " +
                                  *primaryKey_);
    }
  }
}

size_t TableSchema::findColumn(const std::string &name) const {
  auto it = indexByName_.find(name);
  return it == indexByName_.end() ? npos : it->second;
}

bool TableSchema::addColumn(const Column &col) {
  if (indexByName_.find(col.name) != indexByName_.end()) {
    return false;
  }
  columns_.push_back(col);
  indexByName_.emplace(col.name, columns_.size() - 1);
  return true;
}

bool TableSchema::removeColumn(const std::string &name) {
  auto it = indexByName_.find(name);
  if (it == indexByName_.end())
    return false;
  size_t idx = it->second;
  columns_.erase(columns_.begin() + static_cast<std::ptrdiff_t>(idx));
  indexByName_.clear();
  indexByName_.reserve(columns_.size());
  for (size_t i = 0; i < columns_.size(); ++i) {
    indexByName_.emplace(columns_[i].name, i);
  }
  if (primaryKey_ && *primaryKey_ == name) {
    primaryKey_.reset();
  }
  return true;
}

bool TableSchema::getColumn(const std::string &name, Column &out) const {
  auto it = indexByName_.find(name);
  if (it == indexByName_.end())
    return false;
  out = columns_[it->second];
  return true;
}

bool TableSchema::updateColumn(const Column &col) {
  auto it = indexByName_.find(col.name);
  if (it == indexByName_.end())
    return false;
  columns_[it->second] = col;
  return true;
}

void TableSchema::setPrimaryKey(std::optional<std::string> primaryKey) {
  if (primaryKey) {
    if (indexByName_.find(*primaryKey) == indexByName_.end()) {
      throw std::invalid_argument("Primary key column not found in schema: " +
                                  *primaryKey);
    }
  }
  primaryKey_ = std::move(primaryKey);
}

Row::Row(size_t columnCount) : values_(columnCount) {}

void Row::set(size_t idx, std::unique_ptr<Value> v) {
  if (idx >= values_.size()) {
    throw std::out_of_range("Row::set index out of range");
  }
  values_[idx] = std::move(v);
}

// Deep copy ctor
Row::Row(const Row &other) : values_(other.values_.size()) {
  for (size_t i = 0; i < other.values_.size(); ++i) {
    const auto &ptr = other.values_[i];
    if (ptr)
      values_[i] = ptr->clone();
  }
}

// Deep copy assignment
Row &Row::operator=(const Row &other) {
  if (this == &other)
    return *this;
  values_.resize(other.values_.size());
  for (size_t i = 0; i < other.values_.size(); ++i) {
    const auto &ptr = other.values_[i];
    if (ptr)
      values_[i] = ptr->clone();
    else
      values_[i].reset();
  }
  return *this;
}

Row Row::clone() const { return Row(*this); }

bool SchemaValidator::valueMatches(ColumnType ct, const Value &v) {
  switch (ct) {
  case ColumnType::Null:
    return v.type() == ValueType::Null;
  case ColumnType::Integer:
    return v.type() == ValueType::Integer;
  case ColumnType::Float:
    return v.type() == ValueType::Float || v.type() == ValueType::Integer;
  case ColumnType::String:
    return v.type() == ValueType::String;
  case ColumnType::Boolean:
    return v.type() == ValueType::Boolean;
  }
  return false;
}

bool SchemaValidator::checkConstraints(const Column &col, const Value &v,
                                       std::string &err) {
  // Type-specific richer constraints
  switch (col.type) {
  case ColumnType::String: {
    if (v.type() != ValueType::String)
      return true; // type mismatch handled elsewhere
    const auto &s = static_cast<const StringValue &>(v).asString();
    if (col.constraints.minLength && s.size() < *col.constraints.minLength) {
      err = "String shorter than minLength for '" + col.name + "'";
      return false;
    }
    if (col.constraints.maxLength && s.size() > *col.constraints.maxLength) {
      err = "String longer than maxLength for '" + col.name + "'";
      return false;
    }
    if (!col.constraints.oneOf.empty()) {
      bool ok = false;
      for (const auto &allowed : col.constraints.oneOf) {
        if (s == allowed) {
          ok = true;
          break;
        }
      }
      if (!ok) {
        err = "Value not in allowed set for '" + col.name + "'";
        return false;
      }
    }
    break;
  }
  case ColumnType::Integer:
  case ColumnType::Float: {
    double d = 0.0;
    if (v.type() == ValueType::Integer)
      d = static_cast<const IntegerValue &>(v).asFloat();
    else if (v.type() == ValueType::Float)
      d = static_cast<const FloatValue &>(v).asFloat();
    else
      return true; // type mismatch handled elsewhere
    if (col.constraints.minValue && d < *col.constraints.minValue) {
      err = "Numeric value below minValue for '" + col.name + "'";
      return false;
    }
    if (col.constraints.maxValue && d > *col.constraints.maxValue) {
      err = "Numeric value above maxValue for '" + col.name + "'";
      return false;
    }
    break;
  }
  case ColumnType::Null:
  case ColumnType::Boolean:
    // no extra constraints for these
    break;
  }
  return true;
}

std::string SchemaValidator::validateRow(const TableSchema &schema,
                                         const Row &row) {
  const auto &cols = schema.columns();
  if (row.size() != cols.size()) {
    return "Row size does not match schema column count";
  }
  for (size_t i = 0; i < cols.size(); ++i) {
    const auto &col = cols[i];
    const auto &val = row.values()[i];
    if (!val) {
      if (!col.nullable) {
        return "Non-nullable column '" + col.name + "' has null value";
      }
      continue;
    }
    if (!valueMatches(col.type, *val)) {
      return "Value type does not match column '" + col.name + "'";
    }
    std::string err;
    if (!checkConstraints(col, *val, err)) {
      return err;
    }
  }
  return {};
}

Document deepCopyDocument(const Document &doc) {
  Document out;
  out.reserve(doc.size());
  for (const auto &kv : doc) {
    if (kv.second)
      out.emplace(kv.first, kv.second->clone());
    else
      out.emplace(kv.first, nullptr);
  }
  return out;
}

// ----- RowShallow helpers -----
RowShallow RowShallow::fromClones(const Row &r) {
  RowShallow rs(r.size());
  for (size_t i = 0; i < r.size(); ++i) {
    rs.set(i, std::shared_ptr<Value>(r.at(i).clone().release()));
  }
  return rs;
}

Row RowShallow::toRowDeep() const {
  Row r(values_.size());
  for (size_t i = 0; i < values_.size(); ++i) {
    if (values_[i])
      r.set(i, values_[i]->clone());
  }
  return r;
}

// DocumentSchema methods
void DocumentSchema::addField(Column field) {
  fields_[field.name] = std::move(field);
}

bool DocumentSchema::removeField(const std::string &name) {
  return fields_.erase(name) > 0;
}

bool DocumentSchema::getField(const std::string &name, Column &out) const {
  auto it = fields_.find(name);
  if (it == fields_.end())
    return false;
  out = it->second;
  return true;
}

std::string SchemaValidator::validateDocument(const DocumentSchema &schema,
                                              const Document &doc) {
  // Check required fields and types for present fields
  for (const auto &kv : schema.fields()) {
    const auto &fieldName = kv.first;
    const auto &col = kv.second;
    auto it = doc.find(fieldName);
    if (it == doc.end()) {
      if (!col.nullable) {
        return "Missing required field '" + fieldName + "'";
      }
      continue;
    }
    const auto &ptr = it->second;
    if (!ptr) {
      if (!col.nullable) {
        return "Non-nullable field '" + fieldName + "' has null value";
      }
      continue;
    }
    if (!valueMatches(col.type, *ptr)) {
      return "Value type does not match field '" + fieldName + "'";
    }
    std::string err;
    if (!checkConstraints(col, *ptr, err)) {
      return err;
    }
  }
  // Unknown fields are allowed
  return {};
}

std::string SchemaValidator::validateUnique(const TableSchema &schema,
                                            const std::vector<Row> &rows,
                                            bool ignoreNulls) {
  std::vector<size_t> uniqueIdx;
  uniqueIdx.reserve(schema.columns().size());
  for (size_t i = 0; i < schema.columns().size(); ++i) {
    if (schema.columns()[i].unique)
      uniqueIdx.push_back(i);
  }
  if (uniqueIdx.empty())
    return {};

  std::vector<std::unordered_map<std::string, size_t>> seen(uniqueIdx.size());
  for (size_t r = 0; r < rows.size(); ++r) {
    const Row &row = rows[r];
    for (size_t ui = 0; ui < uniqueIdx.size(); ++ui) {
      size_t idx = uniqueIdx[ui];
      const auto &valPtr = row.values()[idx];
      if (!valPtr) {
        if (ignoreNulls)
          continue;
        auto [it, inserted] = seen[ui].emplace("<null>", r);
        if (!inserted) {
          return "Duplicate value for unique column '" +
                 schema.columns()[idx].name + "'";
        }
        continue;
      }
      std::string key = valPtr->toString();
      auto [it, inserted] = seen[ui].emplace(std::move(key), r);
      if (!inserted) {
        return "Duplicate value for unique column '" +
               schema.columns()[idx].name + "'";
      }
    }
  }
  return {};
}

std::string SchemaValidator::validateUnique(const DocumentSchema &schema,
                                            const std::vector<Document> &docs,
                                            bool ignoreNulls) {
  std::vector<std::string> uniqueFields;
  uniqueFields.reserve(schema.fields().size());
  for (const auto &kv : schema.fields()) {
    if (kv.second.unique)
      uniqueFields.push_back(kv.first);
  }
  if (uniqueFields.empty())
    return {};

  std::vector<std::unordered_map<std::string, size_t>> seen(
      uniqueFields.size());
  for (size_t di = 0; di < docs.size(); ++di) {
    const auto &doc = docs[di];
    for (size_t ui = 0; ui < uniqueFields.size(); ++ui) {
      const auto &fname = uniqueFields[ui];
      auto it = doc.find(fname);
      bool isNullish = (it == doc.end()) || (!it->second) ||
                       (it->second && it->second->type() == ValueType::Null);
      if (isNullish) {
        if (ignoreNulls)
          continue;
        auto [sit, inserted] = seen[ui].emplace("<null>", di);
        if (!inserted)
          return "Duplicate value for unique field '" + fname + "'";
        continue;
      }
      std::string key = it->second->toString();
      auto [sit, inserted] = seen[ui].emplace(std::move(key), di);
      if (!inserted)
        return "Duplicate value for unique field '" + fname + "'";
    }
  }
  return {};
}

} // namespace kadedb
