#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "kadedb/schema.h"
#include "kadedb/value.h"

namespace kadedb {

class ResultRow {
public:
  ResultRow() = default;
  explicit ResultRow(std::vector<std::unique_ptr<Value>> values)
      : values_(std::move(values)) {}

  size_t size() const { return values_.size(); }
  const Value& at(size_t idx) const { return *values_.at(idx); }
  const std::vector<std::unique_ptr<Value>>& values() const { return values_; }

private:
  std::vector<std::unique_ptr<Value>> values_;
};

class ResultSet {
public:
  ResultSet() = default;
  explicit ResultSet(std::vector<std::string> columnNames, std::vector<ColumnType> columnTypes)
      : columnNames_(std::move(columnNames)), columnTypes_(std::move(columnTypes)) {}

  const std::vector<std::string>& columnNames() const { return columnNames_; }
  const std::vector<ColumnType>& columnTypes() const { return columnTypes_; }

  void addRow(ResultRow row) { rows_.push_back(std::move(row)); }
  size_t rowCount() const { return rows_.size(); }
  const ResultRow& row(size_t idx) const { return rows_.at(idx); }

  // Simple forward iteration
  void reset() { cursor_ = 0; }
  bool next() { return cursor_ < rows_.size() ? ++cursor_ <= rows_.size() : false; }
  const ResultRow& current() const { return rows_.at(cursor_ - 1); }

private:
  std::vector<std::string> columnNames_;
  std::vector<ColumnType> columnTypes_;
  std::vector<ResultRow> rows_;
  size_t cursor_ = 0; // 1-based position when iterating; 0 means before first
};

} // namespace kadedb
