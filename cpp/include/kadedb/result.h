#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <stdexcept>
#include <vector>
#include <sstream>
#include <algorithm>

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

  // Convenience: string view of a cell (uses Value::toString())
  std::string toString(size_t idx) const { return values_.at(idx)->toString(); }

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
  size_t columnCount() const { return columnNames_.size(); }

  void addRow(ResultRow row) { rows_.push_back(std::move(row)); }
  size_t rowCount() const { return rows_.size(); }
  const ResultRow& row(size_t idx) const { return rows_.at(idx); }

  // Lookup column index by name; returns npos if not found
  size_t findColumn(const std::string& name) const {
    for (size_t i = 0; i < columnNames_.size(); ++i) {
      if (columnNames_[i] == name) return i;
    }
    return npos;
  }
  static constexpr size_t npos = static_cast<size_t>(-1);

  // Safe cell access with bounds checking
  const Value& at(size_t rowIdx, size_t colIdx) const { return rows_.at(rowIdx).at(colIdx); }
  const Value& at(size_t rowIdx, const std::string& colName) const {
    size_t c = findColumn(colName);
    if (c == npos) throw std::out_of_range("ResultSet::at(): unknown column '" + colName + "'");
    return at(rowIdx, c);
  }

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

  // STL-style iteration over rows (const)
  auto begin() const { return rows_.begin(); }
  auto end() const { return rows_.end(); }

  // -------- Conversion Utilities --------
  // Convert to a matrix of strings using Value::toString()
  std::vector<std::vector<std::string>> toStringMatrix(bool includeHeader = false) const {
    std::vector<std::vector<std::string>> out;
    out.reserve(rows_.size() + (includeHeader ? 1 : 0));
    if (includeHeader) {
      out.emplace_back(columnNames_);
    }
    for (const auto& r : rows_) {
      std::vector<std::string> rowStr;
      rowStr.reserve(r.size());
      for (size_t i = 0; i < r.size(); ++i) rowStr.emplace_back(r.toString(i));
      out.emplace_back(std::move(rowStr));
    }
    return out;
  }

  // Convert to CSV (no external deps). Simple escaping for quotes and delimiters.
  // alwaysQuote: if true, every field is quoted. quoteChar: quoting character.
  std::string toCSV(char delimiter = ',', bool includeHeader = true, bool alwaysQuote = false, char quoteChar = '"') const {
    auto csvEscape = [delimiter, alwaysQuote, quoteChar](const std::string& s) {
      bool needQuotes = alwaysQuote || s.find(delimiter) != std::string::npos || s.find(quoteChar) != std::string::npos || s.find('\n') != std::string::npos || s.find('\r') != std::string::npos;
      if (!needQuotes) return s;
      std::string t;
      t.reserve(s.size() + 2);
      t.push_back(quoteChar);
      for (char ch : s) {
        if (ch == quoteChar) t.push_back(quoteChar);
        t.push_back(ch);
      }
      t.push_back(quoteChar);
      return t;
    };

    std::ostringstream oss;
    if (includeHeader && !columnNames_.empty()) {
      for (size_t i = 0; i < columnNames_.size(); ++i) {
        if (i) oss << delimiter;
        oss << csvEscape(columnNames_[i]);
      }
      oss << '\n';
    }
    for (const auto& r : rows_) {
      for (size_t i = 0; i < r.size(); ++i) {
        if (i) oss << delimiter;
        oss << csvEscape(r.toString(i));
      }
      oss << '\n';
    }
    return oss.str();
  }

  // Convert to a JSON string: [{col: value, ...}, ...]; values typed based on ValueType
  // indent: spaces per level; 0 means compact one-line JSON
  std::string toJSON(bool includeMetadata = false, int indent = 0) const {
    auto jsonEscape = [](const std::string& s) {
      std::string out;
      out.reserve(s.size() + 2);
      for (char ch : s) {
        switch (ch) {
          case '"': out += "\\\""; break;
          case '\\': out += "\\\\"; break;
          case '\b': out += "\\b"; break;
          case '\f': out += "\\f"; break;
          case '\n': out += "\\n"; break;
          case '\r': out += "\\r"; break;
          case '\t': out += "\\t"; break;
          default:
            if (static_cast<unsigned char>(ch) < 0x20) {
              // Control character -> \u00XX
              const char* hex = "0123456789abcdef";
              out += "\\u00";
              out.push_back(hex[(ch >> 4) & 0xF]);
              out.push_back(hex[ch & 0xF]);
            } else {
              out.push_back(ch);
            }
        }
      }
      return out;
    };

    auto emitValue = [&](const Value& v) {
      switch (v.type()) {
        case ValueType::Null: return std::string("null");
        case ValueType::Boolean: return std::string(static_cast<const BooleanValue&>(v).value() ? "true" : "false");
        case ValueType::Integer: return std::to_string(static_cast<const IntegerValue&>(v).value());
        case ValueType::Float: return static_cast<const FloatValue&>(v).toString();
        case ValueType::String: return std::string("\"") + jsonEscape(static_cast<const StringValue&>(v).value()) + "\"";
      }
      return std::string("null");
    };

    std::ostringstream oss;
    auto indentNL = [&](int level) {
      if (indent > 0) {
        oss << '\n';
        for (int i = 0; i < level * indent; ++i) oss << ' ';
      }
    };
    auto commaSpace = [&]() {
      if (indent == 0) return; // compact
      oss << ' ';
    };

    if (!includeMetadata) {
      oss << '[';
      if (indent > 0 && !rows_.empty()) indentNL(1);
      for (size_t r = 0; r < rows_.size(); ++r) {
        if (r) { oss << ','; indentNL(1); }
        oss << '{';
        if (indent > 0 && !columnNames_.empty()) indentNL(2);
        for (size_t c = 0; c < columnNames_.size(); ++c) {
          if (c) { oss << ','; indentNL(2); }
          oss << '"' << jsonEscape(columnNames_[c]) << '"' << ':'; if (indent>0) oss << ' ';
          oss << emitValue(rows_[r].at(c));
        }
        if (indent > 0 && !columnNames_.empty()) { indentNL(1); }
        oss << '}';
      }
      if (indent > 0 && !rows_.empty()) { indentNL(0); }
      oss << ']';
      return oss.str();
    }
    // With metadata wrapper
    oss << '{';
    if (indent > 0) indentNL(1);
    // columns metadata
    oss << "\"columns\":[";
    for (size_t i = 0; i < columnNames_.size(); ++i) {
      if (i) oss << ',' << (indent>0?' ':'\0');
      oss << '"' << jsonEscape(columnNames_[i]) << '"';
    }
    oss << "],"; if (indent>0) indentNL(1), oss << "\"types\":["; else oss << "\"types\":[";
    for (size_t i = 0; i < columnTypes_.size(); ++i) {
      if (i) oss << ',' << (indent>0?' ':'\0');
      switch (columnTypes_[i]) {
        case ColumnType::Null: oss << "\"Null\""; break;
        case ColumnType::Integer: oss << "\"Integer\""; break;
        case ColumnType::Float: oss << "\"Float\""; break;
        case ColumnType::String: oss << "\"String\""; break;
        case ColumnType::Boolean: oss << "\"Boolean\""; break;
      }
    }
    oss << "],"; if (indent>0) indentNL(1), oss << "\"rows\":"; else oss << "\"rows\":";
    // reuse array form
    oss << '[';
    if (indent > 0 && !rows_.empty()) indentNL(2);
    for (size_t r = 0; r < rows_.size(); ++r) {
      if (r) { oss << ','; indentNL(2); }
      oss << '{';
      if (indent > 0 && !columnNames_.empty()) indentNL(3);
      for (size_t c = 0; c < columnNames_.size(); ++c) {
        if (c) { oss << ','; indentNL(3); }
        oss << '"' << jsonEscape(columnNames_[c]) << '"' << ':'; if (indent>0) oss << ' ';
        oss << emitValue(rows_[r].at(c));
      }
      if (indent > 0 && !columnNames_.empty()) { indentNL(2); }
      oss << '}';
    }
    if (indent > 0 && !rows_.empty()) indentNL(1);
    oss << ']';
    if (indent > 0) indentNL(0);
    oss << '}';
    return oss.str();
  }

  // -------- Pagination --------
  void setPageSize(size_t ps) { pageSize_ = (ps == 0 ? 0 : ps); }
  size_t pageSize() const { return pageSize_; }
  size_t totalPages() const {
    if (pageSize_ == 0) return rows_.empty() ? 0 : 1; // single page with all rows
    return (rows_.size() + pageSize_ - 1) / pageSize_;
  }
  // Return [start, end) bounds for a page (throws if pageIndex out of range)
  std::pair<size_t, size_t> pageBounds(size_t pageIndex) const {
    size_t tp = totalPages();
    if (pageIndex >= tp) throw std::out_of_range("ResultSet::pageBounds(): page index out of range");
    if (pageSize_ == 0) return {0, rows_.size()};
    size_t start = pageIndex * pageSize_;
    size_t end = std::min(start + pageSize_, rows_.size());
    return {start, end};
  }
  // Non-owning view of a page as pointers (valid while ResultSet lives)
  std::vector<const ResultRow*> page(size_t pageIndex) const {
    auto [s, e] = pageBounds(pageIndex);
    std::vector<const ResultRow*> out;
    out.reserve(e - s);
    for (size_t i = s; i < e; ++i) out.push_back(&rows_[i]);
    return out;
  }

private:
  std::vector<std::string> columnNames_;
  std::vector<ColumnType> columnTypes_;
  std::vector<ResultRow> rows_;
  size_t cursor_ = static_cast<size_t>(-1); // index of current row; -1 means before first
  size_t pageSize_ = 0; // 0 means no pagination (all rows in a single page)
};

} // namespace kadedb

