#pragma once

#include <memory>
#include <optional>
#include <stdexcept>
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
  explicit TableSchema(std::vector<Column> cols,
                       std::optional<std::string> primaryKey = std::nullopt);

  // Copy/move semantics (value-type; shallow copy of Columns which are value
  // objects)
  TableSchema(const TableSchema &) = default;
  TableSchema(TableSchema &&) noexcept = default;
  TableSchema &operator=(const TableSchema &) = default;
  TableSchema &operator=(TableSchema &&) noexcept = default;

  const std::vector<Column> &columns() const { return columns_; }
  const std::optional<std::string> &primaryKey() const { return primaryKey_; }

  // Lookup by name; returns index or npos if not found
  size_t findColumn(const std::string &name) const;
  static constexpr size_t npos = static_cast<size_t>(-1);

  // Modification helpers
  // Returns false if a column with the same name already exists
  bool addColumn(const Column &col);
  // Returns false if the column does not exist
  bool removeColumn(const std::string &name);
  // Retrieve a copy of the column definition; returns false if not found
  bool getColumn(const std::string &name, Column &out) const;
  // Update an existing column by name; returns false if not found
  bool updateColumn(const Column &col);
  // Set or clear primary key. Throws if the column doesn't exist when set.
  void setPrimaryKey(std::optional<std::string> primaryKey);

private:
  std::vector<Column> columns_;
  std::unordered_map<std::string, size_t> indexByName_;
  std::optional<std::string> primaryKey_;
};

// Row representations
//
// Row: deep-copy semantics
// - Stores std::unique_ptr<Value> per cell.
// - Copy/assignment performs deep copies by cloning each Value.
// - Use when isolation across copies is required.
//
// RowShallow: shallow-copy semantics
// - Stores std::shared_ptr<Value> per cell.
// - Default copy/assignment is shallow (shared ownership of the same Value
// objects).
// - Provides conversion helpers:
//     * RowShallow::fromClones(const Row&) -> one-time deep clone, then shared.
//     * RowShallow::toRowDeep() -> produce a deep Row by cloning each shared
//     Value.
// - Use for cheaper copies when sharing is acceptable; call toRowDeep() when
// isolation is needed.
//
// Both align with TableSchema column counts and perform bounds checking in
// accessors.
//
// A simple row representation that aligns with a TableSchema
class Row {
public:
  explicit Row(size_t columnCount = 0);

  // Deep copy/move semantics: copies clone underlying Values
  Row(const Row &other);
  Row(Row &&other) noexcept = default;
  Row &operator=(const Row &other);
  Row &operator=(Row &&other) noexcept = default;

  size_t size() const { return values_.size(); }
  const Value &at(size_t idx) const { return *values_.at(idx); }
  Value &at(size_t idx) { return *values_.at(idx); }

  void set(size_t idx, std::unique_ptr<Value> v);
  const std::vector<std::unique_ptr<Value>> &values() const { return values_; }

  // Deep clone utility
  Row clone() const;

private:
  std::vector<std::unique_ptr<Value>> values_;
};

// A flexible document schema keyed by field name.
class DocumentSchema {
public:
  DocumentSchema() = default;

  // Copy/move semantics (value-type; fields map holds Columns which are value
  // objects)
  DocumentSchema(const DocumentSchema &) = default;
  DocumentSchema(DocumentSchema &&) noexcept = default;
  DocumentSchema &operator=(const DocumentSchema &) = default;
  DocumentSchema &operator=(DocumentSchema &&) noexcept = default;

  // Add or replace a field definition
  void addField(Column field);
  // Remove a field if present; returns false if not found
  bool removeField(const std::string &name);

  const std::unordered_map<std::string, Column> &fields() const {
    return fields_;
  }
  bool hasField(const std::string &name) const {
    return fields_.find(name) != fields_.end();
  }
  // Retrieve a copy of the field definition; returns false if not found
  bool getField(const std::string &name, Column &out) const;

private:
  std::unordered_map<std::string, Column> fields_;
};

// A simple in-memory document representation
// Wrapper around unordered_map to guarantee noexcept moves on MSVC and expose
// a map-like API used throughout the codebase.
class Document {
public:
  using map_type = std::unordered_map<std::string, std::unique_ptr<Value>>;
  using value_type = map_type::value_type;
  using iterator = map_type::iterator;
  using const_iterator = map_type::const_iterator;

  Document() = default;

  // Non-copyable (unique_ptr values)
  Document(const Document &) = delete;
  Document &operator=(const Document &) = delete;

  // Strong noexcept moves implemented via swap to avoid MSVC picking copy
  Document(Document &&other) noexcept { swap(other); }
  Document &operator=(Document &&other) noexcept {
    if (this != &other)
      swap(other);
    return *this;
  }

  void swap(Document &other) noexcept { map_.swap(other.map_); }

  // Capacity
  size_t size() const noexcept { return map_.size(); }
  bool empty() const noexcept { return map_.empty(); }
  void reserve(size_t n) { map_.reserve(n); }

  // Iteration
  iterator begin() noexcept { return map_.begin(); }
  const_iterator begin() const noexcept { return map_.begin(); }
  const_iterator cbegin() const noexcept { return map_.cbegin(); }
  iterator end() noexcept { return map_.end(); }
  const_iterator end() const noexcept { return map_.end(); }
  const_iterator cend() const noexcept { return map_.cend(); }

  // Lookup
  iterator find(const std::string &key) { return map_.find(key); }
  const_iterator find(const std::string &key) const { return map_.find(key); }

  // Element access
  map_type::mapped_type &operator[](const std::string &key) {
    return map_[key];
  }
  map_type::mapped_type &at(const std::string &key) { return map_.at(key); }
  const map_type::mapped_type &at(const std::string &key) const {
    return map_.at(key);
  }

  // Modifiers (forward to underlying map)
  template <class... Args> std::pair<iterator, bool> emplace(Args &&...args) {
    return map_.emplace(std::forward<Args>(args)...);
  }
  template <class... Args>
  std::pair<iterator, bool> try_emplace(const std::string &key,
                                        Args &&...args) {
    return map_.try_emplace(key, std::forward<Args>(args)...);
  }
  void clear() noexcept { map_.clear(); }

private:
  map_type map_;
};

// Deep copy helper for Document (since Document is an alias, not a class)
Document deepCopyDocument(const Document &doc);

// A shallow-copy row representation that shares Value objects via shared_ptr
class RowShallow {
public:
  explicit RowShallow(size_t columnCount = 0) : values_(columnCount) {}

  // Default copy/move semantics are shallow (shared ownership)
  RowShallow() = default;
  RowShallow(const RowShallow &) = default;
  RowShallow(RowShallow &&) noexcept = default;
  RowShallow &operator=(const RowShallow &) = default;
  RowShallow &operator=(RowShallow &&) noexcept = default;

  size_t size() const { return values_.size(); }
  const Value &at(size_t idx) const { return *values_.at(idx); }
  Value &at(size_t idx) { return *values_.at(idx); }

  void set(size_t idx, std::shared_ptr<Value> v) {
    if (idx >= values_.size())
      throw std::out_of_range("RowShallow::set index out of range");
    values_[idx] = std::move(v);
  }
  const std::vector<std::shared_ptr<Value>> &values() const { return values_; }

  // Construct a shallow row by deep-cloning the source Row values
  static RowShallow fromClones(const Row &r);
  // Convert back to deep Row (clones values)
  Row toRowDeep() const;

private:
  std::vector<std::shared_ptr<Value>> values_;
};

// Minimal validation utility
class SchemaValidator {
public:
  // Returns empty string on success, otherwise an error message
  static std::string validateRow(const TableSchema &schema, const Row &row);

  // Validate a document against a DocumentSchema. Flexible: unknown fields are
  // allowed.
  static std::string validateDocument(const DocumentSchema &schema,
                                      const Document &doc);

  // Uniqueness/richer constraints (in-memory checks)
  // Ensures columns with unique=true do not have duplicate non-null values
  // across rows
  static std::string validateUnique(const TableSchema &schema,
                                    const std::vector<Row> &rows,
                                    bool ignoreNulls = true);
  // Ensures fields with unique=true do not have duplicate non-null values
  // across documents
  static std::string validateUnique(const DocumentSchema &schema,
                                    const std::vector<Document> &docs,
                                    bool ignoreNulls = true);

  // Overload to validate uniqueness across a set of document references
  // without requiring copies/moves of Document (which contains unique_ptr).
  static std::string validateUnique(const DocumentSchema &schema,
                                    const std::vector<const Document *> &docs,
                                    bool ignoreNulls = true);

private:
  static bool valueMatches(ColumnType ct, const Value &v);
  static bool checkConstraints(const Column &col, const Value &v,
                               std::string &err);
};

} // namespace kadedb
