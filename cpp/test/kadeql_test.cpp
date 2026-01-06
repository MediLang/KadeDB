#include "kadedb/kadeql.h"
#include <cassert>
#include <iostream>
#include <memory>

using namespace kadedb::kadeql;

void testTokenizer() {
  std::cout << "Testing Tokenizer..." << std::endl;

  // Test basic tokenization
  std::string query = "SELECT name, age FROM users WHERE age > 18";
  Tokenizer tokenizer(query);

  std::vector<Token> expected_tokens = {
      {TokenType::SELECT, "SELECT", 1, 1, 0},
      {TokenType::IDENTIFIER, "name", 1, 8, 7},
      {TokenType::COMMA, ",", 1, 12, 11},
      {TokenType::IDENTIFIER, "age", 1, 14, 13},
      {TokenType::FROM, "FROM", 1, 18, 17},
      {TokenType::IDENTIFIER, "users", 1, 23, 22},
      {TokenType::WHERE, "WHERE", 1, 29, 28},
      {TokenType::IDENTIFIER, "age", 1, 35, 34},
      {TokenType::GREATER_THAN, ">", 1, 39, 38},
      {TokenType::NUMBER_LITERAL, "18", 1, 41, 40}};

  for (const auto &expected : expected_tokens) {
    Token actual = tokenizer.next();
    assert(actual.type == expected.type);
    assert(actual.value == expected.value);
    std::cout << "  Token: " << Tokenizer::tokenTypeToString(actual.type)
              << " = '" << actual.value << "'" << std::endl;
  }

  std::cout << "Tokenizer tests passed!" << std::endl;
}

void testSelectParser() {
  std::cout << "Testing SELECT Parser..." << std::endl;

  // Test simple SELECT
  {
    KadeQLParser parser;
    auto stmt1 = parser.parse("SELECT * FROM users");
    auto select1 = dynamic_cast<SelectStatement *>(stmt1.get());
    assert(select1 != nullptr);
    assert(select1->getColumns().size() == 1);
    assert(select1->getColumns()[0] == "*");
    assert(select1->getTableName() == "users");
    assert(select1->getWhereClause() == nullptr);
    std::cout << "  Simple SELECT: " << select1->toString() << std::endl;
  }

  // Test SELECT with columns
  {
    KadeQLParser parser;
    auto stmt2 = parser.parse("SELECT name, age FROM users");
    auto select2 = dynamic_cast<SelectStatement *>(stmt2.get());
    assert(select2 != nullptr);
    assert(select2->getColumns().size() == 2);
    assert(select2->getColumns()[0] == "name");
    assert(select2->getColumns()[1] == "age");
    std::cout << "  Column SELECT: " << select2->toString() << std::endl;
  }

  // Test SELECT with WHERE
  {
    KadeQLParser parser;
    auto stmt3 = parser.parse("SELECT name FROM users WHERE age > 18");
    auto select3 = dynamic_cast<SelectStatement *>(stmt3.get());
    assert(select3 != nullptr);
    assert(select3->getWhereClause() != nullptr);
    std::cout << "  WHERE SELECT: " << select3->toString() << std::endl;
  }

  // Test SELECT with BETWEEN in WHERE
  {
    KadeQLParser parser;
    auto stmt5 = parser.parse(
        "SELECT name FROM users WHERE timestamp BETWEEN 10 AND 20");
    auto select5 = dynamic_cast<SelectStatement *>(stmt5.get());
    assert(select5 != nullptr);
    assert(select5->getWhereClause() != nullptr);
    std::cout << "  BETWEEN SELECT: " << select5->toString() << std::endl;
  }

  // Test SELECT with trailing semicolon
  {
    KadeQLParser parser;
    auto stmt4 = parser.parse("SELECT * FROM users;");
    auto select4 = dynamic_cast<SelectStatement *>(stmt4.get());
    assert(select4 != nullptr);
    assert(select4->getColumns().size() == 1);
    assert(select4->getColumns()[0] == "*");
    assert(select4->getTableName() == "users");
    assert(select4->getWhereClause() == nullptr);
    std::cout << "  Trailing semicolon SELECT: " << select4->toString()
              << std::endl;
  }

  std::cout << "SELECT parser tests passed!" << std::endl;
}

void testInsertParser() {
  std::cout << "Testing INSERT Parser..." << std::endl;

  // Test simple INSERT
  {
    KadeQLParser parser;
    auto stmt1 = parser.parse("INSERT INTO users VALUES ('John', 25)");
    auto insert1 = dynamic_cast<InsertStatement *>(stmt1.get());
    assert(insert1 != nullptr);
    assert(insert1->getTableName() == "users");
    assert(insert1->getColumns().empty()); // No explicit columns
    assert(insert1->getValues().size() == 1);
    assert(insert1->getValues()[0].size() == 2);
    std::cout << "  Simple INSERT: " << insert1->toString() << std::endl;
  }

  // Test INSERT with columns
  {
    KadeQLParser parser;
    auto stmt2 =
        parser.parse("INSERT INTO users (name, age) VALUES ('Alice', 30)");
    auto insert2 = dynamic_cast<InsertStatement *>(stmt2.get());
    assert(insert2 != nullptr);
    assert(insert2->getColumns().size() == 2);
    assert(insert2->getColumns()[0] == "name");
    assert(insert2->getColumns()[1] == "age");
    std::cout << "  Column INSERT: " << insert2->toString() << std::endl;
  }

  std::cout << "INSERT parser tests passed!" << std::endl;
}

void testErrorHandling() {
  std::cout << "Testing Error Handling..." << std::endl;

  try {
    KadeQLParser parser;
    parser.parse("INVALID QUERY");
    assert(false); // Should not reach here
  } catch (const ParseError &e) {
    std::cout << "  Caught expected error: " << e.what() << std::endl;
  }

  try {
    KadeQLParser parser;
    parser.parse("SELECT FROM"); // Missing column list
    assert(false);               // Should not reach here
  } catch (const ParseError &e) {
    std::cout << "  Caught expected error: " << e.what() << std::endl;
  }

  std::cout << "Error handling tests passed!" << std::endl;
}

int main() {
  try {
    std::cout << "=== KadeQL Parser Tests ===" << std::endl;

    testTokenizer();
    std::cout << std::endl;

    testSelectParser();
    std::cout << std::endl;

    testInsertParser();
    std::cout << std::endl;

    testErrorHandling();
    std::cout << std::endl;

    std::cout << "All tests passed!" << std::endl;
    return 0;

  } catch (const std::exception &e) {
    std::cerr << "Test failed with exception: " << e.what() << std::endl;
    return 1;
  }
}
