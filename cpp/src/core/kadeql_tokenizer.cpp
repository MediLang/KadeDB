#include "kadedb/kadeql_tokenizer.h"
#include <cctype>
#include <stdexcept>

namespace kadedb {
namespace kadeql {

// Initialize keyword mapping
const std::unordered_map<std::string, TokenType> Tokenizer::keywords_ = {
    {"SELECT", TokenType::SELECT}, {"FROM", TokenType::FROM},
    {"WHERE", TokenType::WHERE},   {"INSERT", TokenType::INSERT},
    {"INTO", TokenType::INTO},     {"VALUES", TokenType::VALUES},
    {"AND", TokenType::AND},       {"OR", TokenType::OR}};

Tokenizer::Tokenizer(const std::string &input)
    : input_(input), current_pos_(0), current_line_(1), current_column_(1),
      has_peeked_(false) {}

Token Tokenizer::next() {
  if (has_peeked_) {
    has_peeked_ = false;
    return peeked_token_;
  }

  skipWhitespace();

  if (current_pos_ >= input_.length()) {
    return makeToken(TokenType::END_OF_INPUT, "");
  }

  char c = currentChar();

  // String literals
  if (c == '\'' || c == '"') {
    return readString();
  }

  // Numbers
  if (isDigit(c)) {
    return readNumber();
  }

  // Identifiers and keywords
  if (isAlpha(c) || c == '_') {
    return readIdentifierOrKeyword();
  }

  // Operators and delimiters
  switch (c) {
  case '=':
    advance();
    return makeToken(TokenType::EQUALS, '=');
  case '<':
    advance();
    if (currentChar() == '=') {
      advance();
      return makeToken(TokenType::LESS_EQUAL, "<=");
    }
    return makeToken(TokenType::LESS_THAN, '<');
  case '>':
    advance();
    if (currentChar() == '=') {
      advance();
      return makeToken(TokenType::GREATER_EQUAL, ">=");
    }
    return makeToken(TokenType::GREATER_THAN, '>');
  case '!':
    advance();
    if (currentChar() == '=') {
      advance();
      return makeToken(TokenType::NOT_EQUAL, "!=");
    }
    return makeToken(TokenType::UNKNOWN, '!');
  case ',':
    advance();
    return makeToken(TokenType::COMMA, ',');
  case ';':
    advance();
    return makeToken(TokenType::SEMICOLON, ';');
  case '(':
    advance();
    return makeToken(TokenType::LPAREN, '(');
  case ')':
    advance();
    return makeToken(TokenType::RPAREN, ')');
  case '*':
    advance();
    return makeToken(TokenType::ASTERISK, '*');
  default:
    advance();
    return makeToken(TokenType::UNKNOWN, c);
  }
}

Token Tokenizer::peek() {
  if (!has_peeked_) {
    peeked_token_ = next();
    has_peeked_ = true;
  }
  return peeked_token_;
}

bool Tokenizer::hasMore() const {
  // Create a temporary tokenizer to check if there are more non-whitespace
  // tokens
  Tokenizer temp(input_);
  temp.current_pos_ = current_pos_;
  temp.current_line_ = current_line_;
  temp.current_column_ = current_column_;

  temp.skipWhitespace();
  return temp.current_pos_ < input_.length();
}

void Tokenizer::reset() {
  current_pos_ = 0;
  current_line_ = 1;
  current_column_ = 1;
  has_peeked_ = false;
}

std::string Tokenizer::tokenTypeToString(TokenType type) {
  switch (type) {
  case TokenType::SELECT:
    return "SELECT";
  case TokenType::FROM:
    return "FROM";
  case TokenType::WHERE:
    return "WHERE";
  case TokenType::INSERT:
    return "INSERT";
  case TokenType::INTO:
    return "INTO";
  case TokenType::VALUES:
    return "VALUES";
  case TokenType::IDENTIFIER:
    return "IDENTIFIER";
  case TokenType::STRING_LITERAL:
    return "STRING_LITERAL";
  case TokenType::NUMBER_LITERAL:
    return "NUMBER_LITERAL";
  case TokenType::EQUALS:
    return "EQUALS";
  case TokenType::LESS_THAN:
    return "LESS_THAN";
  case TokenType::GREATER_THAN:
    return "GREATER_THAN";
  case TokenType::LESS_EQUAL:
    return "LESS_EQUAL";
  case TokenType::GREATER_EQUAL:
    return "GREATER_EQUAL";
  case TokenType::NOT_EQUAL:
    return "NOT_EQUAL";
  case TokenType::AND:
    return "AND";
  case TokenType::OR:
    return "OR";
  case TokenType::COMMA:
    return "COMMA";
  case TokenType::SEMICOLON:
    return "SEMICOLON";
  case TokenType::LPAREN:
    return "LPAREN";
  case TokenType::RPAREN:
    return "RPAREN";
  case TokenType::ASTERISK:
    return "ASTERISK";
  case TokenType::WHITESPACE:
    return "WHITESPACE";
  case TokenType::END_OF_INPUT:
    return "END_OF_INPUT";
  case TokenType::UNKNOWN:
    return "UNKNOWN";
  default:
    return "UNKNOWN";
  }
}

char Tokenizer::currentChar() const {
  if (current_pos_ >= input_.length()) {
    return '\0';
  }
  return input_[current_pos_];
}

char Tokenizer::peekChar(size_t offset) const {
  size_t pos = current_pos_ + offset;
  if (pos >= input_.length()) {
    return '\0';
  }
  return input_[pos];
}

void Tokenizer::advance() {
  if (current_pos_ < input_.length()) {
    if (input_[current_pos_] == '\n') {
      current_line_++;
      current_column_ = 1;
    } else {
      current_column_++;
    }
    current_pos_++;
  }
}

void Tokenizer::skipWhitespace() {
  while (current_pos_ < input_.length() && isWhitespace(currentChar())) {
    advance();
  }
}

Token Tokenizer::readString() {
  char quote_char = currentChar();
  std::string value;
  size_t start_line = current_line_;
  size_t start_column = current_column_;
  size_t start_pos = current_pos_;

  advance(); // Skip opening quote

  while (current_pos_ < input_.length() && currentChar() != quote_char) {
    if (currentChar() == '\\') {
      advance();
      if (current_pos_ < input_.length()) {
        char escaped = currentChar();
        switch (escaped) {
        case 'n':
          value += '\n';
          break;
        case 't':
          value += '\t';
          break;
        case 'r':
          value += '\r';
          break;
        case '\\':
          value += '\\';
          break;
        case '\'':
          value += '\'';
          break;
        case '"':
          value += '"';
          break;
        default:
          value += escaped;
          break;
        }
        advance();
      }
    } else {
      value += currentChar();
      advance();
    }
  }

  if (current_pos_ >= input_.length()) {
    throw std::runtime_error("Unterminated string literal at line " +
                             std::to_string(start_line) + ", column " +
                             std::to_string(start_column));
  }

  advance(); // Skip closing quote

  return Token(TokenType::STRING_LITERAL, value, start_line, start_column,
               start_pos);
}

Token Tokenizer::readNumber() {
  std::string value;
  size_t start_line = current_line_;
  size_t start_column = current_column_;
  size_t start_pos = current_pos_;

  while (current_pos_ < input_.length() &&
         (isDigit(currentChar()) || currentChar() == '.')) {
    value += currentChar();
    advance();
  }

  return Token(TokenType::NUMBER_LITERAL, value, start_line, start_column,
               start_pos);
}

Token Tokenizer::readIdentifierOrKeyword() {
  std::string value;
  size_t start_line = current_line_;
  size_t start_column = current_column_;
  size_t start_pos = current_pos_;

  while (current_pos_ < input_.length() &&
         (isAlphaNumeric(currentChar()) || currentChar() == '_')) {
    value += currentChar();
    advance();
  }

  // Convert to uppercase for keyword lookup
  std::string upper_value = value;
  for (char &c : upper_value) {
    c = std::toupper(c);
  }

  auto it = keywords_.find(upper_value);
  TokenType type = (it != keywords_.end()) ? it->second : TokenType::IDENTIFIER;

  return Token(type, value, start_line, start_column, start_pos);
}

Token Tokenizer::readOperator() {
  // This method is currently unused but could be extended for complex operators
  return makeToken(TokenType::UNKNOWN, "");
}

bool Tokenizer::isAlpha(char c) const { return std::isalpha(c); }

bool Tokenizer::isDigit(char c) const { return std::isdigit(c); }

bool Tokenizer::isAlphaNumeric(char c) const { return std::isalnum(c); }

bool Tokenizer::isWhitespace(char c) const { return std::isspace(c); }

Token Tokenizer::makeToken(TokenType type, const std::string &value) const {
  return Token(type, value, current_line_, current_column_, current_pos_);
}

Token Tokenizer::makeToken(TokenType type, char value) const {
  return Token(type, std::string(1, value), current_line_, current_column_,
               current_pos_);
}

} // namespace kadeql
} // namespace kadedb
