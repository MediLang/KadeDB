#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <stdexcept>
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
  // Iteration API: zero-based cursor, starts before first (-1)
  void reset() { cursor_ = static_cast<size_t>(-1); }
  bool next() {
    // Move to the next row if available
    if (cursor_ + 1 < rows_.size()) {
      ++cursor_;
      return true;
    }
    return false;
  }
  const ResultRow& current() const {
    if (cursor_ >= rows_.size()) {
      throw std::out_of_range("ResultSet::current(): no current row");
    }
    return rows_.at(cursor_);
  }

private:
  std::vector<std::string> columnNames_;
  std::vector<ColumnType> columnTypes_;
  std::vector<ResultRow> rows_;
  size_t cursor_ = static_cast<size_t>(-1); // index of current row; -1 means before first
};

} // namespace kadedb
