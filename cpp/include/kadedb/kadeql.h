#pragma once

/**
 * KadeQL - A minimal SQL-like query language for KadeDB
 *
 * This header provides the complete KadeQL parser implementation including:
 * - Tokenizer/Lexer for lexical analysis
 * - AST nodes for representing parsed queries
 * - Parser for converting query strings to AST
 *
 * Example usage:
 *
 * ```cpp
 * #include "kadedb/kadeql.h"
 *
 * using namespace kadedb::kadeql;
 *
 * try {
 *     KadeQLParser parser;
 *     auto statement = parser.parse("SELECT name, age FROM users WHERE age >
 * 18");
 *
 *     if (auto select = dynamic_cast<SelectStatement*>(statement.get())) {
 *         // Process SELECT statement
 *         std::cout << "Table: " << select->getTableName() << std::endl;
 *         for (const auto& col : select->getColumns()) {
 *             std::cout << "Column: " << col << std::endl;
 *         }
 *     }
 * } catch (const ParseError& e) {
 *     std::cerr << "Parse error: " << e.what() << std::endl;
 * }
 * ```
 *
 * Supported SQL subset:
 * - SELECT statements with column lists and WHERE clauses
 * - INSERT statements with VALUES
 * - Basic comparison operators (=, !=, <, >, <=, >=)
 * - Logical operators (AND, OR)
 * - String and numeric literals
 * - Identifiers for table and column names
 */

#include "kadedb/kadeql_ast.h"
#include "kadedb/kadeql_parser.h"
#include "kadedb/kadeql_tokenizer.h"

namespace kadedb {
namespace kadeql {

/**
 * Convenience function to parse a KadeQL query string
 * @param query The KadeQL query string to parse
 * @return Unique pointer to the parsed statement
 * @throws ParseError if the query is invalid
 */
inline std::unique_ptr<Statement> parseQuery(const std::string &query) {
  KadeQLParser parser;
  return parser.parse(query);
}

/**
 * Convenience function to tokenize a KadeQL query string
 * @param query The KadeQL query string to tokenize
 * @return Vector of tokens
 */
inline std::vector<Token> tokenizeQuery(const std::string &query) {
  Tokenizer tokenizer(query);
  std::vector<Token> tokens;

  while (tokenizer.hasMore()) {
    Token token = tokenizer.next();
    if (token.type != TokenType::END_OF_INPUT) {
      tokens.push_back(token);
    }
  }

  return tokens;
}

} // namespace kadeql
} // namespace kadedb
