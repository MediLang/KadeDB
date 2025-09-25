#include "kadedb/kadeql_parser.h"
#include <sstream>
#include <stdexcept>

namespace kadedb {
namespace kadeql {

std::unique_ptr<Statement> KadeQLParser::parse(const std::string &query) {
  tokenizer_ = std::make_unique<Tokenizer>(query);
  advance(); // Initialize current_token_

  try {
    auto statement = parseStatement();

    // Ensure we've consumed all tokens
    if (!isAtEnd()) {
      error("Unexpected token after statement: " + current_token_.value);
    }

    return statement;
  } catch (const ParseError &e) {
    throw;
  } catch (const std::exception &e) {
    error("Parse error: " + std::string(e.what()));
    return nullptr; // Never reached due to error() throwing
  }
}

std::unique_ptr<Statement> KadeQLParser::parseStatement() {
  if (match(TokenType::SELECT)) {
    return parseSelectStatement();
  } else if (match(TokenType::INSERT)) {
    return parseInsertStatement();
  } else {
    error("Expected SELECT or INSERT statement, got: " + current_token_.value);
    return nullptr; // Never reached
  }
}

std::unique_ptr<SelectStatement> KadeQLParser::parseSelectStatement() {
  // Parse column list
  std::vector<std::string> columns = parseColumnList();

  // Expect FROM
  consume(TokenType::FROM, "Expected FROM after column list");

  // Parse table name
  Token table_token =
      consume(TokenType::IDENTIFIER, "Expected table name after FROM");
  std::string table_name = table_token.value;

  // Optional WHERE clause
  std::unique_ptr<Expression> where_clause = nullptr;
  if (match(TokenType::WHERE)) {
    where_clause = parseExpression();
  }

  return std::make_unique<SelectStatement>(
      std::move(columns), std::move(table_name), std::move(where_clause));
}

std::unique_ptr<InsertStatement> KadeQLParser::parseInsertStatement() {
  // Expect INTO
  consume(TokenType::INTO, "Expected INTO after INSERT");

  // Parse table name
  Token table_token =
      consume(TokenType::IDENTIFIER, "Expected table name after INTO");
  std::string table_name = table_token.value;

  // Optional column list
  std::vector<std::string> columns;
  if (match(TokenType::LPAREN)) {
    columns = parseColumnList();
    consume(TokenType::RPAREN, "Expected ')' after column list");
  }

  // Expect VALUES
  consume(TokenType::VALUES, "Expected VALUES");

  // Parse values list
  auto values = parseValuesList();

  return std::make_unique<InsertStatement>(
      std::move(table_name), std::move(columns), std::move(values));
}

std::unique_ptr<Expression> KadeQLParser::parseExpression() {
  return parseLogicalOr();
}

std::unique_ptr<Expression> KadeQLParser::parseLogicalOr() {
  auto expr = parseLogicalAnd();

  while (match(TokenType::OR)) {
    auto right = parseLogicalAnd();
    expr = std::make_unique<BinaryExpression>(
        std::move(expr), BinaryExpression::Operator::OR, std::move(right));
  }

  return expr;
}

std::unique_ptr<Expression> KadeQLParser::parseLogicalAnd() {
  auto expr = parseComparison();

  while (match(TokenType::AND)) {
    auto right = parseComparison();
    expr = std::make_unique<BinaryExpression>(
        std::move(expr), BinaryExpression::Operator::AND, std::move(right));
  }

  return expr;
}

std::unique_ptr<Expression> KadeQLParser::parseComparison() {
  auto expr = parsePrimary();

  while (isComparisonOperator(current_token_.type)) {
    TokenType op_token = current_token_.type;
    advance();
    auto right = parsePrimary();
    auto op = tokenToBinaryOperator(op_token);
    expr = std::make_unique<BinaryExpression>(std::move(expr), op,
                                              std::move(right));
  }

  return expr;
}

std::unique_ptr<Expression> KadeQLParser::parsePrimary() {
  if (check(TokenType::STRING_LITERAL)) {
    Token token = current_token_;
    advance();
    return std::make_unique<LiteralExpression>(token.value);
  }

  if (check(TokenType::NUMBER_LITERAL)) {
    Token token = current_token_;
    advance();
    // Try to parse as integer first, then as double
    try {
      if (token.value.find('.') != std::string::npos) {
        double value = std::stod(token.value);
        return std::make_unique<LiteralExpression>(value);
      } else {
        int64_t value = std::stoll(token.value);
        return std::make_unique<LiteralExpression>(value);
      }
    } catch (const std::exception &) {
      error("Invalid number format: " + token.value);
      return nullptr; // Never reached
    }
  }

  if (check(TokenType::IDENTIFIER)) {
    Token token = current_token_;
    advance();
    return std::make_unique<IdentifierExpression>(token.value);
  }

  if (match(TokenType::LPAREN)) {
    auto expr = parseExpression();
    consume(TokenType::RPAREN, "Expected ')' after expression");
    return expr;
  }

  error("Expected expression, got: " + current_token_.value);
  return nullptr; // Never reached
}

std::vector<std::string> KadeQLParser::parseColumnList() {
  std::vector<std::string> columns;

  // Handle SELECT *
  if (match(TokenType::ASTERISK)) {
    columns.push_back("*");
    return columns;
  }

  // Parse first column
  Token column_token = consume(TokenType::IDENTIFIER, "Expected column name");
  columns.push_back(column_token.value);

  // Parse additional columns
  while (match(TokenType::COMMA)) {
    column_token =
        consume(TokenType::IDENTIFIER, "Expected column name after ','");
    columns.push_back(column_token.value);
  }

  return columns;
}

std::vector<std::vector<std::unique_ptr<Expression>>>
KadeQLParser::parseValuesList() {
  std::vector<std::vector<std::unique_ptr<Expression>>> values;

  // Parse first value tuple
  consume(TokenType::LPAREN, "Expected '(' before values");
  values.push_back(parseExpressionList());
  consume(TokenType::RPAREN, "Expected ')' after values");

  // Parse additional value tuples
  while (match(TokenType::COMMA)) {
    consume(TokenType::LPAREN, "Expected '(' before values");
    values.push_back(parseExpressionList());
    consume(TokenType::RPAREN, "Expected ')' after values");
  }

  return values;
}

std::vector<std::unique_ptr<Expression>> KadeQLParser::parseExpressionList() {
  std::vector<std::unique_ptr<Expression>> expressions;

  // Parse first expression
  expressions.push_back(parseExpression());

  // Parse additional expressions
  while (match(TokenType::COMMA)) {
    expressions.push_back(parseExpression());
  }

  return expressions;
}

void KadeQLParser::advance() {
  if (!isAtEnd()) {
    current_token_ = tokenizer_->next();
  }
}

bool KadeQLParser::match(TokenType type) {
  if (check(type)) {
    advance();
    return true;
  }
  return false;
}

bool KadeQLParser::check(TokenType type) const {
  if (isAtEnd())
    return false;
  return current_token_.type == type;
}

Token KadeQLParser::consume(TokenType type, const std::string &message) {
  if (check(type)) {
    Token token = current_token_;
    advance();
    return token;
  }

  error(message + ", got: " + current_token_.value);
  return Token(); // Never reached
}

void KadeQLParser::error(const std::string &message) {
  std::ostringstream oss;
  oss << message << " at line " << current_token_.line << ", column "
      << current_token_.column;
  throw ParseError(oss.str(), current_token_.line, current_token_.column);
}

void KadeQLParser::synchronize() {
  advance();

  while (!isAtEnd()) {
    if (current_token_.type == TokenType::SEMICOLON)
      return;

    switch (current_token_.type) {
    case TokenType::SELECT:
    case TokenType::INSERT:
      return;
    default:
      break;
    }

    advance();
  }
}

BinaryExpression::Operator KadeQLParser::tokenToBinaryOperator(TokenType type) {
  switch (type) {
  case TokenType::EQUALS:
    return BinaryExpression::Operator::EQUALS;
  case TokenType::NOT_EQUAL:
    return BinaryExpression::Operator::NOT_EQUALS;
  case TokenType::LESS_THAN:
    return BinaryExpression::Operator::LESS_THAN;
  case TokenType::GREATER_THAN:
    return BinaryExpression::Operator::GREATER_THAN;
  case TokenType::LESS_EQUAL:
    return BinaryExpression::Operator::LESS_EQUAL;
  case TokenType::GREATER_EQUAL:
    return BinaryExpression::Operator::GREATER_EQUAL;
  case TokenType::AND:
    return BinaryExpression::Operator::AND;
  case TokenType::OR:
    return BinaryExpression::Operator::OR;
  default:
    error("Invalid binary operator: " + Tokenizer::tokenTypeToString(type));
    return BinaryExpression::Operator::EQUALS; // Never reached
  }
}

bool KadeQLParser::isComparisonOperator(TokenType type) const {
  return type == TokenType::EQUALS || type == TokenType::NOT_EQUAL ||
         type == TokenType::LESS_THAN || type == TokenType::GREATER_THAN ||
         type == TokenType::LESS_EQUAL || type == TokenType::GREATER_EQUAL;
}

bool KadeQLParser::isAtEnd() const {
  return current_token_.type == TokenType::END_OF_INPUT;
}

} // namespace kadeql
} // namespace kadedb
