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
class BetweenExpression;
class IdentifierExpression;
class LiteralExpression;
class FunctionCallExpression;

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

class BetweenExpression : public Expression {
public:
  BetweenExpression(std::unique_ptr<Expression> expr,
                    std::unique_ptr<Expression> lower,
                    std::unique_ptr<Expression> upper)
      : expr_(std::move(expr)), lower_(std::move(lower)),
        upper_(std::move(upper)) {}

  const Expression *getExpr() const { return expr_.get(); }
  const Expression *getLower() const { return lower_.get(); }
  const Expression *getUpper() const { return upper_.get(); }

  std::string toString() const override;

private:
  std::unique_ptr<Expression> expr_;
  std::unique_ptr<Expression> lower_;
  std::unique_ptr<Expression> upper_;
};

/**
 * Function call expression for aggregate and scalar functions
 * Examples: TIME_BUCKET(timestamp, 60), FIRST(value, timestamp), LAST(value,
 * timestamp)
 */
class FunctionCallExpression : public Expression {
public:
  FunctionCallExpression(std::string name,
                         std::vector<std::unique_ptr<Expression>> args)
      : name_(std::move(name)), args_(std::move(args)) {}

  const std::string &getName() const { return name_; }
  const std::vector<std::unique_ptr<Expression>> &getArgs() const {
    return args_;
  }

  std::string toString() const override;

private:
  std::string name_;
  std::vector<std::unique_ptr<Expression>> args_;
};

/**
 * Select item: an expression with an optional alias
 * Examples: col, col AS alias, TIME_BUCKET(ts, 60) AS bucket
 */
struct SelectItem {
  std::unique_ptr<Expression> expr;
  std::string alias; // empty if no alias

  SelectItem(std::unique_ptr<Expression> e, std::string a = "")
      : expr(std::move(e)), alias(std::move(a)) {}

  // Move-only
  SelectItem(SelectItem &&) = default;
  SelectItem &operator=(SelectItem &&) = default;
  SelectItem(const SelectItem &) = delete;
  SelectItem &operator=(const SelectItem &) = delete;

  std::string toString() const;
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
 *
 * Supports two modes:
 * 1. Legacy column-name mode: SELECT col1, col2 FROM t (backward compatible)
 * 2. Expression mode: SELECT expr AS alias, ... FROM t (new)
 *
 * Use isExpressionMode() to check which mode is active.
 */
class SelectStatement : public Statement {
public:
  // Legacy constructor (column names only)
  SelectStatement(std::vector<std::string> columns, std::string table_name,
                  std::unique_ptr<Expression> where_clause = nullptr)
      : columns_(std::move(columns)), table_name_(std::move(table_name)),
        where_clause_(std::move(where_clause)), expression_mode_(false) {}

  // New constructor (expression-based select items)
  SelectStatement(std::vector<SelectItem> select_items, std::string table_name,
                  std::unique_ptr<Expression> where_clause, bool /*expr_tag*/)
      : table_name_(std::move(table_name)),
        where_clause_(std::move(where_clause)),
        select_items_(std::move(select_items)), expression_mode_(true) {}

  // Legacy accessor (returns column names; works in both modes)
  const std::vector<std::string> &getColumns() const { return columns_; }

  // New accessor for expression mode
  bool isExpressionMode() const { return expression_mode_; }
  const std::vector<SelectItem> &getSelectItems() const {
    return select_items_;
  }

  const std::string &getTableName() const { return table_name_; }
  const Expression *getWhereClause() const { return where_clause_.get(); }

  std::string toString() const override;
  StatementType type() const override { return StatementType::SELECT; }

private:
  std::vector<std::string> columns_; // legacy mode
  std::string table_name_;
  std::unique_ptr<Expression> where_clause_;
  std::vector<SelectItem> select_items_; // expression mode
  bool expression_mode_ = false;
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
