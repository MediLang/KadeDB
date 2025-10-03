#include "kadedb/kadeql.h"
#include "kadedb/query_executor.h"
#include "kadedb/schema.h"
#include "kadedb/storage.h"

#include <cassert>
#include <iostream>
#include <set>
#include <string>

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

  QueryExecutor exec(storage);

  // Seed
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

  // Mixed AND/OR nesting:
  // (age >= 20 AND name != 'Alice') OR (NOT (age < 30) AND name = 'Alice')
  // This should include Bob (first branch) and Alice (second branch)
  {
    auto stmt =
        parseQuery("SELECT name FROM users WHERE (age >= 20 AND name != "
                   "'Alice') OR (NOT (age < 30) AND name = 'Alice')");
    auto res = exec.execute(*stmt);
    assert(res.hasValue());
    const auto &rs = res.value();
    assert(rs.columnCount() == 1);
    assert(rs.rowCount() == 2);
    std::set<std::string> names;
    for (size_t i = 0; i < rs.rowCount(); ++i) {
      names.insert(rs.at(i, 0).asString());
    }
    assert(names.count("Alice") == 1);
    assert(names.count("Bob") == 1);
  }

  // Projection error on unknown column via executor
  {
    auto stmt = parseQuery("SELECT unknown FROM users");
    auto res = exec.execute(*stmt);
    assert(!res.hasValue());
    assert(res.status().code() == StatusCode::InvalidArgument);
  }

  // WHERE with reversed literal/identifier order: 25 < age
  {
    auto stmt = parseQuery("SELECT name FROM users WHERE 25 < age");
    auto res = exec.execute(*stmt);
    assert(res.hasValue());
    const auto &rs = res.value();
    assert(rs.columnCount() == 1);
    assert(rs.rowCount() == 1);
    assert(rs.at(0, 0).asString() == std::string("Alice"));
  }

  std::cout << "KadeQL mixed predicates, projection error, and reversed order "
               "tests passed"
            << std::endl;
  return 0;
}
