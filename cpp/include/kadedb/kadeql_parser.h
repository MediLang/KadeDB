#pragma once

#include "kadedb/kadeql_ast.h"
#include "kadedb/kadeql_tokenizer.h"
#include <memory>

namespace kadedb {
namespace kadeql {

/**
 * KadeQL Parser - Recursive descent parser for KadeQL statements
 *
 * Supports:
 * - SELECT statements with basic WHERE clauses
 * - INSERT statements for adding data
 *
 * Grammar (simplified):
 * statement := select_statement | insert_statement
 * select_statement := SELECT column_list FROM table_name [WHERE expression]
 * insert_statement := INSERT INTO table_name [(column_list)] VALUES value_list
 * column_list := identifier | '*' | identifier (',' identifier)*
 * value_list := '(' expression_list ')' (',' '(' expression_list ')')*
 * expression_list := expression (',' expression)*
 * expression := logical_or
 * logical_or := logical_and (OR logical_and)*
 * logical_and := comparison (AND comparison)*
 * comparison := primary (('=' | '!=' | '<' | '>' | '<=' | '>=') primary)*
 * primary := identifier | string_literal | number_literal | '(' expression ')'
 */
class KadeQLParser {
public:
  /**
   * Constructor
   */
  KadeQLParser() = default;

  /**
   * Parse a KadeQL query string into an AST
   * @param query The KadeQL query string to parse
   * @return Unique pointer to the parsed statement
   * @throws ParseError if the query is invalid
   */
  std::unique_ptr<Statement> parse(const std::string &query);

private:
  std::unique_ptr<Tokenizer> tokenizer_;
  Token current_token_;

  // Core parsing methods
  std::unique_ptr<Statement> parseStatement();
  std::unique_ptr<SelectStatement> parseSelectStatement();
  std::unique_ptr<InsertStatement> parseInsertStatement();

  // Expression parsing (recursive descent)
  std::unique_ptr<Expression> parseExpression();
  std::unique_ptr<Expression> parseLogicalOr();
  std::unique_ptr<Expression> parseLogicalAnd();
  std::unique_ptr<Expression> parseComparison();
  std::unique_ptr<Expression> parsePrimary();

  // Helper methods
  std::vector<std::string> parseColumnList();
  // Parse a comma-separated list of identifiers (no '*') used for INSERT
  // columns
  std::vector<std::string> parseIdentifierList();
  std::vector<std::vector<std::unique_ptr<Expression>>> parseValuesList();
  std::vector<std::unique_ptr<Expression>> parseExpressionList();

  // Token management
  void advance();
  bool match(TokenType type);
  bool check(TokenType type) const;
  Token consume(TokenType type, const std::string &message);

  // Error handling
  void error(const std::string &message);
  void synchronize();

  // Utility methods
  BinaryExpression::Operator tokenToBinaryOperator(TokenType type);
  bool isComparisonOperator(TokenType type) const;
  bool isAtEnd() const;
};

} // namespace kadeql
} // namespace kadedb
