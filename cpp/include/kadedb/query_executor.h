#pragma once

#include "kadedb/kadeql_ast.h"
#include "kadedb/result.h"
#include "kadedb/storage.h"

#include <optional>
#include <string>
#include <vector>

namespace kadedb {
namespace kadeql {

class QueryExecutor {
public:
  explicit QueryExecutor(RelationalStorage &storage) : storage_(storage) {}

  // Execute any KadeQL statement against the relational storage layer.
  Result<ResultSet> execute(const Statement &statement);

private:
  RelationalStorage &storage_;

  // Helpers
  Result<ResultSet> executeSelect(const SelectStatement &select);
  Result<ResultSet> executeInsert(const InsertStatement &insert);
  Result<ResultSet> executeUpdate(const UpdateStatement &update);
  Result<ResultSet> executeDelete(const DeleteStatement &del);

  // Build a storage Predicate (optional) from an expression tree.
  // Returns std::nullopt if expr is null. Returns InvalidArgument if
  // unsupported.
  Result<std::optional<Predicate>> buildPredicate(const Expression *expr) const;

  // Literal to Value factory
  std::unique_ptr<Value>
  literalToValue(const LiteralExpression::Value &v) const;

  // Evaluate an expression against a row (for computed UPDATE assignments)
  Result<std::unique_ptr<Value>> evalExpr(const Expression *expr,
                                          const TableSchema &schema,
                                          const Row &row) const;
};

} // namespace kadeql
} // namespace kadedb
