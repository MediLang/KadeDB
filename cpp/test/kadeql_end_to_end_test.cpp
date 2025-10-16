#include "kadedb/kadeql.h"
#include "kadedb/query_executor.h"
#include "kadedb/schema.h"
#include "kadedb/storage.h"
#include "kadedb/value.h"

#include <cassert>
#include <iostream>

using namespace kadedb;
using namespace kadedb::kadeql;

int main() {
  InMemoryRelationalStorage storage;

  // Create schema and table
  TableSchema users({
      Column{"id", ColumnType::Integer, /*nullable=*/false, /*unique=*/true},
      Column{"name", ColumnType::String, /*nullable=*/false},
      Column{"age", ColumnType::Integer, /*nullable=*/false},
      Column{"email", ColumnType::String, /*nullable=*/true},
  });
  auto st = storage.createTable("users", users);
  assert(st.ok());

  QueryExecutor exec(storage);

  // Seed with multiple INSERT variations
  {
    auto stmt = parseQuery("INSERT INTO users (id, name, age, email) VALUES "
                           "(1, 'Alice', 30, 'a@x')");
    auto res = exec.execute(*stmt);
    assert(res.hasValue());
    assert(res.value().at(0, 0).asInt() == 1);
  }
  {
    auto stmt = parseQuery(
        "INSERT INTO users VALUES (2, 'Bob', 22, 'bob@example.com')");
    auto res = exec.execute(*stmt);
    assert(res.hasValue());
  }
  {
    auto stmt = parseQuery("INSERT INTO users (id, name, age, email) VALUES "
                           "(3, 'Carl', 40, 'c@x'), (4, 'Dana', 28, 'd@x')");
    auto res = exec.execute(*stmt);
    assert(res.hasValue());
    assert(res.value().at(0, 0).asInt() == 2);
  }

  // SELECT * sanity
  {
    auto stmt = parseQuery("SELECT * FROM users");
    auto res = exec.execute(*stmt);
    assert(res.hasValue());
    const auto &rs = res.value();
    assert(rs.columnCount() == 4);
    assert(rs.rowCount() == 4);
  }

  // SELECT projection + WHERE (AND/OR)
  {
    auto stmt = parseQuery("SELECT name FROM users WHERE (age > 25 AND name != "
                           "'Dana') OR name = 'Bob'");
    auto res = exec.execute(*stmt);
    assert(res.hasValue());
    const auto &rs = res.value();
    // Expected: Alice (30), Carl (40), Bob (22)
    assert(rs.rowCount() == 3);
  }

  // SELECT with NOT and reversed literal comparison (25 < age)
  {
    auto stmt =
        parseQuery("SELECT id FROM users WHERE NOT(name = 'Bob') AND 25 < age");
    auto res = exec.execute(*stmt);
    assert(res.hasValue());
    const auto &rs = res.value();
    // Expected: Alice(1,30), Carl(3,40), Dana(4,28) -> 3 rows
    assert(rs.rowCount() == 3);
  }

  // Error: unknown projection column
  {
    auto stmt = parseQuery("SELECT nope FROM users");
    auto res = exec.execute(*stmt);
    assert(!res.hasValue());
    assert(res.status().code() == StatusCode::InvalidArgument);
  }

  // Error: unknown column referenced in WHERE
  {
    auto stmt = parseQuery("SELECT name FROM users WHERE missing_col = 1");
    auto res = exec.execute(*stmt);
    assert(!res.hasValue());
    assert(res.status().code() == StatusCode::InvalidArgument);
  }

  // Error: INSERT with duplicate unique id
  {
    auto stmt = parseQuery(
        "INSERT INTO users (id, name, age) VALUES (1, 'Alicia', 31)");
    auto res = exec.execute(*stmt);
    assert(!res.hasValue());
  }

  std::cout << "KadeQL end-to-end test passed" << std::endl;
  return 0;
}
