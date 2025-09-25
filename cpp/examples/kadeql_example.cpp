#include "kadedb/kadeql.h"
#include <iostream>
#include <string>

using namespace kadedb::kadeql;

void demonstrateTokenizer(const std::string &query) {
  std::cout << "=== Tokenizing: " << query << " ===" << std::endl;

  auto tokens = tokenizeQuery(query);
  for (const auto &token : tokens) {
    std::cout << "  " << Tokenizer::tokenTypeToString(token.type) << ": '"
              << token.value << "' (line " << token.line << ", col "
              << token.column << ")" << std::endl;
  }
  std::cout << std::endl;
}

void demonstrateParser(const std::string &query) {
  std::cout << "=== Parsing: " << query << " ===" << std::endl;

  try {
    auto statement = parseQuery(query);
    std::cout << "  Parsed successfully!" << std::endl;
    std::cout << "  AST: " << statement->toString() << std::endl;

    if (auto select = dynamic_cast<SelectStatement *>(statement.get())) {
      std::cout << "  Statement type: SELECT" << std::endl;
      std::cout << "  Table: " << select->getTableName() << std::endl;
      std::cout << "  Columns: ";
      for (size_t i = 0; i < select->getColumns().size(); ++i) {
        if (i > 0)
          std::cout << ", ";
        std::cout << select->getColumns()[i];
      }
      std::cout << std::endl;

      if (select->getWhereClause()) {
        std::cout << "  WHERE clause: " << select->getWhereClause()->toString()
                  << std::endl;
      }
    } else if (auto insert = dynamic_cast<InsertStatement *>(statement.get())) {
      std::cout << "  Statement type: INSERT" << std::endl;
      std::cout << "  Table: " << insert->getTableName() << std::endl;
      std::cout << "  Columns: ";
      if (insert->getColumns().empty()) {
        std::cout << "(implicit)";
      } else {
        for (size_t i = 0; i < insert->getColumns().size(); ++i) {
          if (i > 0)
            std::cout << ", ";
          std::cout << insert->getColumns()[i];
        }
      }
      std::cout << std::endl;
      std::cout << "  Values: " << insert->getValues().size() << " row(s)"
                << std::endl;
    }

  } catch (const ParseError &e) {
    std::cout << "  Parse error: " << e.what() << std::endl;
  }

  std::cout << std::endl;
}

int main() {
  std::cout << "KadeQL Parser Example" << std::endl;
  std::cout << "=====================" << std::endl << std::endl;

  // Example queries to demonstrate
  std::vector<std::string> queries = {
      "SELECT * FROM users",
      "SELECT name, age FROM users WHERE age > 18",
      "SELECT email FROM customers WHERE name = 'John' AND age >= 25",
      "INSERT INTO users VALUES ('Alice', 30, 'alice@example.com')",
      "INSERT INTO customers (name, email) VALUES ('Bob', 'bob@example.com')",
      "INSERT INTO products (name, price) VALUES ('Widget', 19.99), ('Gadget', "
      "29.99)"};

  for (const auto &query : queries) {
    demonstrateTokenizer(query);
    demonstrateParser(query);
    std::cout << "----------------------------------------" << std::endl;
  }

  // Interactive mode
  std::cout << std::endl
            << "Interactive Mode (enter 'quit' to exit):" << std::endl;
  std::string input;
  while (true) {
    std::cout << "KadeQL> ";
    if (!std::getline(std::cin, input)) {
      // Exit on EOF or input failure to prevent infinite loop when stdin is
      // closed/piped
      std::cout << std::endl << "(EOF) Exiting." << std::endl;
      break;
    }

    if (input == "quit" || input == "exit") {
      break;
    }

    if (!input.empty()) {
      demonstrateParser(input);
    }
  }

  std::cout << "Goodbye!" << std::endl;
  return 0;
}
