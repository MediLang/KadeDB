#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace kadedb {
namespace kadeql {

/**
 * Token types for KadeQL lexical analysis
 */
enum class TokenType {
  // Keywords
  SELECT,
  FROM,
  WHERE,
  INSERT,
  INTO,
  VALUES,
  UPDATE,
  DELETE_,
  SET,
  NOT,

  // Identifiers and literals
  IDENTIFIER,
  STRING_LITERAL,
  NUMBER_LITERAL,

  // Operators
  EQUALS,        // =
  LESS_THAN,     // <
  GREATER_THAN,  // >
  LESS_EQUAL,    // <=
  GREATER_EQUAL, // >=
  NOT_EQUAL,     // !=
  AND,           // AND
  OR,            // OR
  PLUS,          // +
  MINUS,         // -
  SLASH,         // /

  // Delimiters
  COMMA,     // ,
  SEMICOLON, // ;
  LPAREN,    // (
  RPAREN,    // )
  ASTERISK,  // *

  // Special
  WHITESPACE,
  END_OF_INPUT,
  UNKNOWN
};

/**
 * Token structure containing type, value, and position information
 */
struct Token {
  TokenType type;
  std::string value;
  size_t line;
  size_t column;
  size_t position; // Absolute position in input

  Token() : type(TokenType::UNKNOWN), line(1), column(1), position(0) {}

  Token(TokenType t, const std::string &v, size_t l, size_t c, size_t p)
      : type(t), value(v), line(l), column(c), position(p) {}

  bool operator==(const Token &other) const {
    return type == other.type && value == other.value;
  }

  bool operator!=(const Token &other) const { return !(*this == other); }
};

/**
 * Tokenizer class for KadeQL lexical analysis
 * Converts input query strings into a sequence of tokens
 */
class Tokenizer {
public:
  /**
   * Constructor
   * @param input The KadeQL query string to tokenize
   */
  explicit Tokenizer(const std::string &input);

  /**
   * Get the next token from the input
   * @return The next token, or END_OF_INPUT if no more tokens
   */
  Token next();

  /**
   * Peek at the next token without consuming it
   * @return The next token without advancing position
   */
  Token peek();

  /**
   * Check if there are more tokens to process
   * @return true if more tokens are available
   */
  bool hasMore() const;

  /**
   * Reset the tokenizer to the beginning of the input
   */
  void reset();

  /**
   * Get current position information
   */
  size_t getCurrentLine() const { return current_line_; }
  size_t getCurrentColumn() const { return current_column_; }
  size_t getCurrentPosition() const { return current_pos_; }

  /**
   * Convert TokenType to string for debugging
   */
  static std::string tokenTypeToString(TokenType type);

private:
  std::string input_;
  size_t current_pos_;
  size_t current_line_;
  size_t current_column_;
  Token peeked_token_;
  bool has_peeked_;

  // Keyword mapping
  static const std::unordered_map<std::string, TokenType> keywords_;

  // Helper methods
  char currentChar() const;
  char peekChar(size_t offset = 1) const;
  void advance();
  void skipWhitespace();
  Token readString();
  Token readNumber();
  Token readIdentifierOrKeyword();
  Token readOperator();
  bool isAlpha(char c) const;
  bool isDigit(char c) const;
  bool isAlphaNumeric(char c) const;
  bool isWhitespace(char c) const;
  Token makeToken(TokenType type, const std::string &value) const;
  Token makeToken(TokenType type, char value) const;
};

} // namespace kadeql
} // namespace kadedb
