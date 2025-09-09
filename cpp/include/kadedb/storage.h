#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "kadedb/result.h" // ResultSet
#include "kadedb/schema.h" // TableSchema, Row, Document
#include "kadedb/status.h" // Status, Result<T>
#include "kadedb/value.h"  // Value helpers

namespace kadedb {

/**
 * @defgroup StorageAPI Relational Storage API
 * @brief Core interfaces and in-memory implementation for relational storage.
 */

/**
 * @defgroup DocumentAPI Document Storage API
 * @brief Core interfaces and in-memory implementation for document storage.
 */

/**
 * Predicate model for SELECT/UPDATE/DELETE filtering.
 *
 * Backward compatible with simple single-column comparisons used in tests, and
 * now supports logical composition (AND/OR/NOT) with multiple child predicates.
 *
 * Usage:
 *  - Comparison (legacy): kind=Comparison, set column/op/rhs
 *  - AND/OR: kind=And/Or, populate children with 2+ predicates
 *  - NOT: kind=Not, populate children with exactly 1 predicate
 *
 * Empty-children semantics (edge cases):
 *  - AND with zero children evaluates to true (neutral element)
 *  - OR with zero children evaluates to false (neutral element)
 *  - NOT with zero children evaluates to false
 */
struct Predicate {
  enum class Kind { Comparison, And, Or, Not };
  enum class Op { Eq, Ne, Lt, Le, Gt, Ge };

  // Node kind (default Comparison for backward compatibility)
  Kind kind = Kind::Comparison;

  // Comparison payload (used when kind==Comparison)
  std::string column;
  Op op = Op::Eq;
  std::unique_ptr<Value> rhs; // Right-hand side as a Value

  // Logical payload (used when kind==And/Or/Not)
  std::vector<Predicate> children;
};

/**
 * A simple predicate for Document queries.
 *
 * Mirrors `Predicate` but targets a document field name instead of a table
 * column. The RHS is carried as a `Value` to allow typed comparisons.
 *
 * Empty-children semantics (edge cases):
 *  - AND with zero children evaluates to true (neutral element)
 *  - OR with zero children evaluates to false (neutral element)
 *  - NOT with zero children evaluates to false
 */
struct DocPredicate {
  enum class Kind { Comparison, And, Or, Not };
  enum class Op { Eq, Ne, Lt, Le, Gt, Ge };
  // Node kind (default Comparison)
  Kind kind = Kind::Comparison;
  // Comparison payload
  std::string field;
  Op op = Op::Eq;
  std::unique_ptr<Value> rhs;
  // Logical payload
  std::vector<DocPredicate> children;
};

/**
 * Storage API for the relational model.
 *
 * Implementations should validate inputs against the provided TableSchema and
 * return appropriate Status codes using the Result<T> wrapper where applicable.
 *
 * Error semantics (MVP):
 *  - createTable: AlreadyExists when table exists; Ok on success
 *  - insertRow: NotFound when table missing; InvalidArgument on schema
 *    mismatch; FailedPrecondition on uniqueness constraint violations; Ok on
 *    success
 *  - select: NotFound when table missing; InvalidArgument when a requested
 *    projection column does not exist; Ok with ResultSet on success
 */
/** @ingroup StorageAPI */
class RelationalStorage {
public:
  virtual ~RelationalStorage() = default;

  /**
   * Create a table with a name and schema.
   * @param table Table name
   * @param schema TableSchema definition
   * @return Status::AlreadyExists if table exists; Status::OK on success
   */
  virtual Status createTable(const std::string &table,
                             const TableSchema &schema) = 0;

  /**
   * Insert a row validated against the table schema.
   * @param table Table name
   * @param row Row to insert (deep copied by implementation)
   * @return Status::NotFound if table missing; Status::InvalidArgument if row
   *         fails schema validation; Status::FailedPrecondition if constraints
   *         (e.g., unique) would be violated; Status::OK on success
   */
  virtual Status insertRow(const std::string &table, const Row &row) = 0;

  /**
   * Basic SELECT across all rows with optional projection and predicate.
   * @param table Table name
   * @param columns Projection list; empty means select *
   * @param where Optional Predicate applied to each row
   * @return Result<ResultSet>::err(Status::NotFound) if table missing;
   *         Result<ResultSet>::err(Status::InvalidArgument) if any requested
   *         projection column is unknown; Result<ResultSet>::ok(ResultSet)
   *         on success (possibly empty)
   */
  virtual Result<ResultSet>
  select(const std::string &table, const std::vector<std::string> &columns,
         const std::optional<Predicate> &where = std::nullopt) = 0;

  /**
   * List existing table names.
   * @return Vector of table names; empty if none.
   */
  virtual std::vector<std::string> listTables() const = 0;

  /**
   * Drop a table and its data.
   * @param table Table name
   * @return Status::NotFound if table missing; Status::OK on success
   */
  virtual Status dropTable(const std::string &table) = 0;

  /**
   * Delete rows matching an optional predicate.
   * @param table Table name
   * @param where Optional Predicate; when not provided, deletes all rows
   * @return Result<size_t> count of deleted rows, or Status::NotFound if table
   * missing
   */
  virtual Result<size_t>
  deleteRows(const std::string &table,
             const std::optional<Predicate> &where = std::nullopt) = 0;

  /**
   * Update rows by assigning new values to specified columns, for rows matching
   * an optional predicate.
   * @param table Table name
   * @param assignments Map column -> Value (cloned by implementation)
   * @param where Optional Predicate; when not provided, updates all rows
   * @return Status::NotFound if table missing; Status::InvalidArgument for
   *         unknown columns or type/constraint violations;
   * Status::FailedPrecondition for post-update uniqueness violations;
   * Status::OK on success
   */
  virtual Status
  updateRows(const std::string &table,
             const std::unordered_map<std::string, std::unique_ptr<Value>>
                 &assignments,
             const std::optional<Predicate> &where = std::nullopt) = 0;

  /**
   * Truncate a table (delete all rows) without dropping schema.
   * @param table Table name
   * @return Status::NotFound if table missing; Status::OK on success
   */
  virtual Status truncateTable(const std::string &table) = 0;
};

// Storage API for document model
/** @ingroup DocumentAPI */
class DocumentStorage {
public:
  virtual ~DocumentStorage() = default;

  /**
   * Create a collection with an optional schema for validation.
   * - Returns Status::AlreadyExists if the collection exists.
   */
  virtual Status createCollection(
      const std::string &collection,
      const std::optional<DocumentSchema> &schema = std::nullopt) = 0;

  /**
   * Drop a collection and all of its documents.
   * - Returns Status::NotFound if the collection does not exist.
   */
  virtual Status dropCollection(const std::string &collection) = 0;

  /**
   * List existing collection names.
   */
  virtual std::vector<std::string> listCollections() const = 0;

  /**
   * Put (insert or replace) a document under collection/key.
   * - If a schema exists for the collection, validates the document.
   * - If the collection does not exist, it will be created (MVP behavior).
   * - Returns Status::InvalidArgument if schema validation fails.
   * - Returns Status::FailedPrecondition on uniqueness violations per schema.
   */
  virtual Status put(const std::string &collection, const std::string &key,
                     const Document &doc) = 0;

  /**
   * Get a document if present.
   * - Returns Result::err(Status::NotFound) if collection/key is not found.
   */
  virtual Result<Document> get(const std::string &collection,
                               const std::string &key) = 0;

  /**
   * Erase a document by key.
   * - Returns Status::NotFound if collection or key is not found.
   */
  virtual Status erase(const std::string &collection,
                       const std::string &key) = 0;

  /**
   * Count documents in a collection.
   * - Returns Result::err(Status::NotFound) if the collection does not exist.
   */
  virtual Result<size_t> count(const std::string &collection) const = 0;

  /**
   * Query documents with optional field projection and predicate filter.
   * - fields: empty means return entire Document values.
   * - where: optional single-field predicate.
   * - Returns Result::err(Status::NotFound) if collection missing.
   * - Returns Result::err(Status::InvalidArgument) if requested projection
   *   fields are unknown under the collection's schema (when schema exists).
   *
   * The returned vector carries pairs of (key, Document). Documents are deep
   * copies and may be projected to only include requested fields.
   */
  virtual Result<std::vector<std::pair<std::string, Document>>>
  query(const std::string &collection, const std::vector<std::string> &fields,
        const std::optional<DocPredicate> &where = std::nullopt) = 0;
};

// In-memory implementations for development and testing
/** @ingroup StorageAPI */
class InMemoryRelationalStorage final : public RelationalStorage {
public:
  InMemoryRelationalStorage() = default;
  ~InMemoryRelationalStorage() override = default;

  Status createTable(const std::string &table,
                     const TableSchema &schema) override;
  Status insertRow(const std::string &table, const Row &row) override;
  Result<ResultSet> select(const std::string &table,
                           const std::vector<std::string> &columns,
                           const std::optional<Predicate> &where) override;
  std::vector<std::string> listTables() const override;
  Status dropTable(const std::string &table) override;
  Result<size_t> deleteRows(const std::string &table,
                            const std::optional<Predicate> &where) override;
  Status
  updateRows(const std::string &table,
             const std::unordered_map<std::string, std::unique_ptr<Value>>
                 &assignments,
             const std::optional<Predicate> &where) override;
  Status truncateTable(const std::string &table) override;

private:
  struct TableData {
    TableSchema schema;
    std::vector<Row> rows; // deep rows
  };
  std::unordered_map<std::string, TableData> tables_;
  // Simple mutex for dev/test thread-safety of the in-memory maps/vectors
  mutable std::mutex mtx_;
};

/** @ingroup DocumentAPI */
class InMemoryDocumentStorage final : public DocumentStorage {
public:
  InMemoryDocumentStorage() = default;
  ~InMemoryDocumentStorage() override = default;

  Status createCollection(const std::string &collection,
                          const std::optional<DocumentSchema> &schema) override;
  Status dropCollection(const std::string &collection) override;
  std::vector<std::string> listCollections() const override;

  Status put(const std::string &collection, const std::string &key,
             const Document &doc) override;
  Result<Document> get(const std::string &collection,
                       const std::string &key) override;
  Status erase(const std::string &collection, const std::string &key) override;
  Result<size_t> count(const std::string &collection) const override;
  Result<std::vector<std::pair<std::string, Document>>>
  query(const std::string &collection, const std::vector<std::string> &fields,
        const std::optional<DocPredicate> &where) override;

private:
  struct CollectionData {
    std::optional<DocumentSchema> schema;
    std::unordered_map<std::string, Document> docs; // key -> Document
  };

  std::unordered_map<std::string, CollectionData> data_;
};

} // namespace kadedb
