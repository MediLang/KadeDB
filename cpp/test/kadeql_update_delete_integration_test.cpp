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
  TableSchema users({
      Column{"name", ColumnType::String, /*nullable=*/false},
      Column{"age", ColumnType::Integer, /*nullable=*/false},
  });
  auto st = storage.createTable("users", users);
  assert(st.ok());

  // Seed data
  {
    auto stmt =
        parseQuery("INSERT INTO users (name, age) VALUES ('Alice', 30)");
    QueryExecutor exec(storage);
    auto res = exec.execute(*stmt);
    assert(res.hasValue());
  }
  {
    auto stmt = parseQuery("INSERT INTO users (name, age) VALUES ('Bob', 22)");
    QueryExecutor exec(storage);
    auto res = exec.execute(*stmt);
    assert(res.hasValue());
  }

  QueryExecutor exec(storage);

  // UPDATE users SET age = 31 WHERE name = 'Alice'
  {
    auto stmt = parseQuery("UPDATE users SET age = 31 WHERE name = 'Alice'");
    auto res = exec.execute(*stmt);
    assert(res.hasValue());
  }

  // Verify update
  {
    auto stmt = parseQuery("SELECT age FROM users WHERE name = 'Alice'");
    auto res = exec.execute(*stmt);
    assert(res.hasValue());
    const auto &rs = res.value();
    assert(rs.rowCount() == 1);
    assert(rs.at(0, 0).asInt() == 31);
  }

  // DELETE FROM users WHERE age < 30
  {
    auto stmt = parseQuery("DELETE FROM users WHERE age < 30");
    auto res = exec.execute(*stmt);
    assert(res.hasValue());
  }

  // Verify delete
  {
    auto stmt = parseQuery("SELECT name FROM users");
    auto res = exec.execute(*stmt);
    assert(res.hasValue());
    const auto &rs = res.value();
    assert(rs.rowCount() == 1);
    assert(rs.at(0, 0).asString() == std::string("Alice"));
  }

  std::cout << "KadeQL UPDATE/DELETE integration test passed" << std::endl;
  return 0;
}
