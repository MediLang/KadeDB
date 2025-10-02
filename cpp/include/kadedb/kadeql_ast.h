#pragma once

#include <memory>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

namespace kadedb {
namespace kadeql {

// Forward declarations
class Statement;
class SelectStatement;
class InsertStatement;
class UpdateStatement;
class DeleteStatement;
class Expression;
class UnaryExpression;
class BinaryExpression;
class IdentifierExpression;
class LiteralExpression;

/**
 * Statement type discriminator
 */
enum class StatementType { SELECT, INSERT, UPDATE, DELETE };

/**
 * Base class for all AST nodes
 */
class ASTNode {
public:
  virtual ~ASTNode() = default;
  virtual std::string toString() const = 0;
};

/**
 * Base class for all expressions
 */
class Expression : public ASTNode {
public:
  virtual ~Expression() = default;
};

/**
 * Unary expression for logical operations (currently NOT)
 */
class UnaryExpression : public Expression {
public:
  enum class Operator { NOT };

  explicit UnaryExpression(Operator op, std::unique_ptr<Expression> operand)
      : operator_(op), operand_(std::move(operand)) {}

  Operator getOperator() const { return operator_; }
  const Expression *getOperand() const { return operand_.get(); }

  std::string toString() const override;

private:
  Operator operator_;
  std::unique_ptr<Expression> operand_;
};

/**
 * Literal value expression (string, number)
 */
class LiteralExpression : public Expression {
public:
  using Value = std::variant<std::string, double, int64_t>;

  explicit LiteralExpression(const std::string &str_value)
      : value_(str_value) {}

  explicit LiteralExpression(double num_value) : value_(num_value) {}

  explicit LiteralExpression(int64_t int_value) : value_(int_value) {}

  const Value &getValue() const { return value_; }

  std::string toString() const override;

private:
  Value value_;
};

/**
 * Identifier expression (column names, table names)
 */
class IdentifierExpression : public Expression {
public:
  explicit IdentifierExpression(const std::string &name) : name_(name) {}

  const std::string &getName() const { return name_; }

  std::string toString() const override { return name_; }

private:
  std::string name_;
};
class BinaryExpression : public Expression {
public:
  enum class Operator {
    EQUALS,        // =
    NOT_EQUALS,    // !=
    LESS_THAN,     // <
    GREATER_THAN,  // >
    LESS_EQUAL,    // <=
    GREATER_EQUAL, // >=
    AND,           // AND
    OR,            // OR
    ADD,           // +
    SUB,           // -
    MUL,           // *
    DIV            // /
  };

  BinaryExpression(std::unique_ptr<Expression> left, Operator op,
                   std::unique_ptr<Expression> right)
      : left_(std::move(left)), operator_(op), right_(std::move(right)) {}

  const Expression *getLeft() const { return left_.get(); }
  const Expression *getRight() const { return right_.get(); }
  Operator getOperator() const { return operator_; }

  std::string toString() const override;

  static std::string operatorToString(Operator op);

private:
  std::unique_ptr<Expression> left_;
  Operator operator_;
  std::unique_ptr<Expression> right_;
};

/**
 * Base class for all statements
 */
class Statement : public ASTNode {
public:
  virtual ~Statement() = default;
  /** Return the concrete statement type */
  virtual StatementType type() const = 0;
};

/**
 * SELECT statement AST node
 */
class SelectStatement : public Statement {
public:
  SelectStatement(std::vector<std::string> columns, std::string table_name,
                  std::unique_ptr<Expression> where_clause = nullptr)
      : columns_(std::move(columns)), table_name_(std::move(table_name)),
        where_clause_(std::move(where_clause)) {}

  const std::vector<std::string> &getColumns() const { return columns_; }
  const std::string &getTableName() const { return table_name_; }
  const Expression *getWhereClause() const { return where_clause_.get(); }

  std::string toString() const override;
  StatementType type() const override { return StatementType::SELECT; }

private:
  std::vector<std::string> columns_;
  std::string table_name_;
  std::unique_ptr<Expression> where_clause_;
};

/**
 * INSERT statement AST node
 */
class InsertStatement : public Statement {
public:
  InsertStatement(std::string table_name, std::vector<std::string> columns,
                  std::vector<std::vector<std::unique_ptr<Expression>>> values)
      : table_name_(std::move(table_name)), columns_(std::move(columns)),
        values_(std::move(values)) {}

  const std::string &getTableName() const { return table_name_; }
  const std::vector<std::string> &getColumns() const { return columns_; }
  const std::vector<std::vector<std::unique_ptr<Expression>>> &
  getValues() const {
    return values_;
  }

  std::string toString() const override;
  StatementType type() const override { return StatementType::INSERT; }

private:
  std::string table_name_;
  std::vector<std::string> columns_;
  std::vector<std::vector<std::unique_ptr<Expression>>> values_;
};

/**
 * UPDATE statement AST node
 */
class UpdateStatement : public Statement {
public:
  using Assignment = std::pair<std::string, std::unique_ptr<Expression>>;

  UpdateStatement(std::string table_name, std::vector<Assignment> assignments,
                  std::unique_ptr<Expression> where_clause = nullptr)
      : table_name_(std::move(table_name)),
        assignments_(std::move(assignments)),
        where_clause_(std::move(where_clause)) {}

  const std::string &getTableName() const { return table_name_; }
  const std::vector<Assignment> &getAssignments() const { return assignments_; }
  const Expression *getWhereClause() const { return where_clause_.get(); }

  std::string toString() const override;
  StatementType type() const override { return StatementType::UPDATE; }

private:
  std::string table_name_;
  std::vector<Assignment> assignments_;
  std::unique_ptr<Expression> where_clause_;
};

/**
 * DELETE statement AST node
 */
class DeleteStatement : public Statement {
public:
  DeleteStatement(std::string table_name,
                  std::unique_ptr<Expression> where_clause = nullptr)
      : table_name_(std::move(table_name)),
        where_clause_(std::move(where_clause)) {}

  const std::string &getTableName() const { return table_name_; }
  const Expression *getWhereClause() const { return where_clause_.get(); }

  std::string toString() const override;
  StatementType type() const override { return StatementType::DELETE; }

private:
  std::string table_name_;
  std::unique_ptr<Expression> where_clause_;
};

/**
 * Exception class for parse errors
 */
class ParseError : public std::runtime_error {
public:
  ParseError(const std::string &message, size_t line = 0, size_t column = 0)
      : std::runtime_error(message), line_(line), column_(column) {}

  size_t getLine() const { return line_; }
  size_t getColumn() const { return column_; }

private:
  size_t line_;
  size_t column_;
};

} // namespace kadeql
} // namespace kadedb
