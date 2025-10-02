#include "kadedb/kadeql.h"
#include "kadedb/query_executor.h"
#include "kadedb/schema.h"
#include "kadedb/storage.h"

#include <cassert>
#include <iostream>

using namespace kadedb;
using namespace kadedb::kadeql;

int main() {
  InMemoryRelationalStorage storage;
  TableSchema users({
      Column{"name", ColumnType::String, /*nullable=*/false},
      Column{"age", ColumnType::Integer, /*nullable=*/false},
  });
  auto st = storage.createTable("users", users);
  assert(st.ok());

  // Seed data
  QueryExecutor exec(storage);
  {
    auto stmt =
        parseQuery("INSERT INTO users (name, age) VALUES ('Alice', 30)");
    auto res = exec.execute(*stmt);
    assert(res.hasValue());
  }
  {
    auto stmt = parseQuery("INSERT INTO users (name, age) VALUES ('Bob', 22)");
    auto res = exec.execute(*stmt);
    assert(res.hasValue());
  }
  {
    auto stmt =
        parseQuery("INSERT INTO users (name, age) VALUES ('Carol', 40)");
    auto res = exec.execute(*stmt);
    assert(res.hasValue());
  }

  // NOT precedence: NOT age < 30  -> select users with age >= 30
  {
    auto stmt = parseQuery("SELECT name FROM users WHERE NOT age < 30");
    auto res = exec.execute(*stmt);
    assert(res.hasValue());
    const auto &rs = res.value();
    assert(rs.rowCount() == 2);
  }

  // Parentheses with NOT and OR: NOT (name = 'Alice' OR name = 'Bob') -> Carol
  // only
  {
    auto stmt = parseQuery(
        "SELECT name FROM users WHERE NOT (name = 'Alice' OR name = 'Bob')");
    auto res = exec.execute(*stmt);
    assert(res.hasValue());
    const auto &rs = res.value();
    assert(rs.rowCount() == 1);
    assert(rs.at(0, 0).asString() == std::string("Carol"));
  }

  // Double NOT: NOT NOT (age = 22) -> Bob only
  {
    auto stmt = parseQuery("SELECT name FROM users WHERE NOT NOT (age = 22)");
    auto res = exec.execute(*stmt);
    assert(res.hasValue());
    const auto &rs = res.value();
    assert(rs.rowCount() == 1);
    assert(rs.at(0, 0).asString() == std::string("Bob"));
  }

  std::cout << "KadeQL NOT predicate tests passed" << std::endl;
  return 0;
}
