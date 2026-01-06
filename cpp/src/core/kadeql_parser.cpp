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

    // Allow optional trailing semicolons
    while (match(TokenType::SEMICOLON)) {
      // consume any extra semicolons
    }

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
  } else if (match(TokenType::UPDATE)) {
    return parseUpdateStatement();
  } else if (match(TokenType::DELETE_)) {
    return parseDeleteStatement();
  } else {
    error("Expected SELECT, INSERT, UPDATE or DELETE statement, got: " +
          current_token_.value);
    return nullptr; // Never reached
  }
}

std::unique_ptr<SelectStatement> KadeQLParser::parseSelectStatement() {
  // Try to parse expression-based select items first
  // This handles: SELECT *, SELECT col, SELECT func(...), SELECT expr AS alias
  std::vector<SelectItem> select_items;
  bool has_expressions = false; // true if any non-simple-identifier item found

  // Handle SELECT *
  if (match(TokenType::ASTERISK)) {
    // Legacy mode: SELECT *
    std::vector<std::string> columns;
    columns.push_back("*");

    consume(TokenType::FROM, "Expected FROM after column list");
    Token table_token =
        consume(TokenType::IDENTIFIER, "Expected table name after FROM");
    std::string table_name = table_token.value;

    std::unique_ptr<Expression> where_clause = nullptr;
    if (match(TokenType::WHERE)) {
      where_clause = parseExpression();
    }

    return std::make_unique<SelectStatement>(
        std::move(columns), std::move(table_name), std::move(where_clause));
  }

  // Parse first select item (expression with optional alias)
  auto first_expr = parseExpression();

  // Check if it's a function call or complex expression
  if (dynamic_cast<FunctionCallExpression *>(first_expr.get()) ||
      dynamic_cast<BinaryExpression *>(first_expr.get())) {
    has_expressions = true;
  }

  std::string first_alias;
  if (match(TokenType::AS)) {
    Token alias_token =
        consume(TokenType::IDENTIFIER, "Expected alias after AS");
    first_alias = alias_token.value;
    has_expressions = true;
  }
  select_items.emplace_back(std::move(first_expr), std::move(first_alias));

  // Parse additional select items
  while (match(TokenType::COMMA)) {
    auto expr = parseExpression();
    if (dynamic_cast<FunctionCallExpression *>(expr.get()) ||
        dynamic_cast<BinaryExpression *>(expr.get())) {
      has_expressions = true;
    }
    std::string alias;
    if (match(TokenType::AS)) {
      Token alias_token =
          consume(TokenType::IDENTIFIER, "Expected alias after AS");
      alias = alias_token.value;
      has_expressions = true;
    }
    select_items.emplace_back(std::move(expr), std::move(alias));
  }

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

  // If we have expressions (function calls, aliases, etc.), use expression mode
  if (has_expressions) {
    return std::make_unique<SelectStatement>(
        std::move(select_items), std::move(table_name), std::move(where_clause),
        true /*expr_tag*/);
  }

  // Otherwise, convert to legacy column-name mode for backward compatibility
  std::vector<std::string> columns;
  for (const auto &item : select_items) {
    if (auto id = dynamic_cast<IdentifierExpression *>(item.expr.get())) {
      columns.push_back(id->getName());
    } else {
      // Fallback: use expression mode if we can't extract simple column names
      return std::make_unique<SelectStatement>(
          std::move(select_items), std::move(table_name),
          std::move(where_clause), true /*expr_tag*/);
    }
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
    // For INSERT, only identifiers are allowed (no '*')
    columns = parseIdentifierList();
    consume(TokenType::RPAREN, "Expected ')' after column list");
  }

  // Expect VALUES
  consume(TokenType::VALUES, "Expected VALUES");

  // Parse values list
  auto values = parseValuesList();

  // Semantic validations
  // 1) Ensure all value tuples have the same arity
  if (!values.empty()) {
    const size_t arity = values.front().size();
    for (size_t i = 1; i < values.size(); ++i) {
      if (values[i].size() != arity) {
        error("Inconsistent VALUES tuple sizes: expected " +
              std::to_string(arity) + ", got " +
              std::to_string(values[i].size()));
      }
    }

    // 2) If explicit columns provided, ensure arity matches
    if (!columns.empty() && arity != columns.size()) {
      error("VALUES count (" + std::to_string(arity) +
            ") does not match column count (" + std::to_string(columns.size()) +
            ")");
    }
  }

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
  auto expr = parseNot();

  while (match(TokenType::AND)) {
    auto right = parseNot();
    expr = std::make_unique<BinaryExpression>(
        std::move(expr), BinaryExpression::Operator::AND, std::move(right));
  }

  return expr;
}

std::unique_ptr<Expression> KadeQLParser::parseComparison() {
  auto expr = parseAdditive();

  while (true) {
    if (isComparisonOperator(current_token_.type)) {
      TokenType op_token = current_token_.type;
      advance();
      auto right = parseAdditive();
      auto op = tokenToBinaryOperator(op_token);
      expr = std::make_unique<BinaryExpression>(std::move(expr), op,
                                                std::move(right));
      continue;
    }

    if (match(TokenType::BETWEEN)) {
      auto lower = parseAdditive();
      consume(TokenType::AND, "Expected AND in BETWEEN expression");
      auto upper = parseAdditive();
      expr = std::make_unique<BetweenExpression>(
          std::move(expr), std::move(lower), std::move(upper));
      continue;
    }

    break;
  }

  return expr;
}

std::unique_ptr<Expression> KadeQLParser::parseNot() {
  if (match(TokenType::NOT)) {
    auto operand = parseNot();
    return std::make_unique<UnaryExpression>(UnaryExpression::Operator::NOT,
                                             std::move(operand));
  }
  return parseComparison();
}

std::unique_ptr<Expression> KadeQLParser::parseAdditive() {
  auto expr = parseMultiplicative();
  while (check(TokenType::PLUS) || check(TokenType::MINUS)) {
    TokenType opTok = current_token_.type;
    advance();
    auto rhs = parseMultiplicative();
    BinaryExpression::Operator op = (opTok == TokenType::PLUS)
                                        ? BinaryExpression::Operator::ADD
                                        : BinaryExpression::Operator::SUB;
    expr =
        std::make_unique<BinaryExpression>(std::move(expr), op, std::move(rhs));
  }
  return expr;
}

std::unique_ptr<Expression> KadeQLParser::parseMultiplicative() {
  auto expr = parseUnarySign();
  while (check(TokenType::ASTERISK) || check(TokenType::SLASH)) {
    TokenType opTok = current_token_.type;
    advance();
    auto rhs = parseUnarySign();
    BinaryExpression::Operator op = (opTok == TokenType::ASTERISK)
                                        ? BinaryExpression::Operator::MUL
                                        : BinaryExpression::Operator::DIV;
    expr =
        std::make_unique<BinaryExpression>(std::move(expr), op, std::move(rhs));
  }
  return expr;
}

std::unique_ptr<Expression> KadeQLParser::parseUnarySign() {
  if (match(TokenType::MINUS)) {
    // Unary minus: parse another unary and translate to 0 - expr
    auto operand = parseUnarySign();
    auto zero = std::make_unique<LiteralExpression>(int64_t(0));
    return std::make_unique<BinaryExpression>(
        std::move(zero), BinaryExpression::Operator::SUB, std::move(operand));
  }
  if (match(TokenType::PLUS)) {
    // Unary plus: no-op
    return parseUnarySign();
  }
  return parsePrimary();
}

std::unique_ptr<UpdateStatement> KadeQLParser::parseUpdateStatement() {
  // UPDATE <table>
  Token table_token =
      consume(TokenType::IDENTIFIER, "Expected table name after UPDATE");
  std::string table_name = table_token.value;

  // SET
  consume(TokenType::SET, "Expected SET in UPDATE statement");

  // Parse assignments: col = expr (, col = expr)*
  std::vector<UpdateStatement::Assignment> assigns;
  while (true) {
    Token colTok =
        consume(TokenType::IDENTIFIER, "Expected column name in SET");
    consume(TokenType::EQUALS, "Expected '=' in assignment");
    auto expr = parseExpression();
    assigns.emplace_back(colTok.value, std::move(expr));
    if (!match(TokenType::COMMA))
      break;
  }

  // Optional WHERE
  std::unique_ptr<Expression> where_clause = nullptr;
  if (match(TokenType::WHERE)) {
    where_clause = parseExpression();
  }

  return std::make_unique<UpdateStatement>(
      std::move(table_name), std::move(assigns), std::move(where_clause));
}

std::unique_ptr<DeleteStatement> KadeQLParser::parseDeleteStatement() {
  // DELETE FROM <table>
  consume(TokenType::FROM, "Expected FROM after DELETE");
  Token table_token =
      consume(TokenType::IDENTIFIER, "Expected table name after FROM");
  std::string table_name = table_token.value;

  // Optional WHERE
  std::unique_ptr<Expression> where_clause = nullptr;
  if (match(TokenType::WHERE)) {
    where_clause = parseExpression();
  }

  return std::make_unique<DeleteStatement>(std::move(table_name),
                                           std::move(where_clause));
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
    // Check if this is a function call: IDENTIFIER followed by '('
    if (check(TokenType::LPAREN)) {
      advance(); // consume '('
      std::vector<std::unique_ptr<Expression>> args;
      if (!check(TokenType::RPAREN)) {
        args = parseExpressionList();
      }
      consume(TokenType::RPAREN, "Expected ')' after function arguments");
      return std::make_unique<FunctionCallExpression>(token.value,
                                                      std::move(args));
    }
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

std::vector<std::string> KadeQLParser::parseIdentifierList() {
  std::vector<std::string> identifiers;

  // Parse first identifier
  Token id = consume(TokenType::IDENTIFIER, "Expected identifier");
  identifiers.push_back(id.value);

  // Parse additional identifiers
  while (match(TokenType::COMMA)) {
    Token next =
        consume(TokenType::IDENTIFIER, "Expected identifier after ','");
    identifiers.push_back(next.value);
  }

  return identifiers;
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
