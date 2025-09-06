#pragma once

#include <functional>
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
 * A simple predicate model for MVP SELECT filtering.
 * Supports basic comparisons on a single column. Right-hand side is carried as
 * a Value to allow typed comparisons using Value::compare.
 */
struct Predicate {
  enum class Op { Eq, Ne, Lt, Le, Gt, Ge };
  std::string column;
  Op op = Op::Eq;
  // Right-hand side as a Value; caller provides an owning unique_ptr
  std::unique_ptr<Value> rhs;
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
class DocumentStorage {
public:
  virtual ~DocumentStorage() = default;

  // Put a document under collection/key. Overwrites if exists.
  virtual Status put(const std::string &collection, const std::string &key,
                     const Document &doc) = 0;

  // Get a document if present.
  virtual Result<Document> get(const std::string &collection,
                               const std::string &key) = 0;
};

// In-memory implementations for development and testing
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
};

class InMemoryDocumentStorage final : public DocumentStorage {
public:
  InMemoryDocumentStorage() = default;
  ~InMemoryDocumentStorage() override = default;

  Status put(const std::string &collection, const std::string &key,
             const Document &doc) override;
  Result<Document> get(const std::string &collection,
                       const std::string &key) override;

private:
  using Collection = std::unordered_map<std::string, Document>;
  std::unordered_map<std::string, Collection> data_;
};

} // namespace kadedb
