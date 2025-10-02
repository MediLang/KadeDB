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

  QueryExecutor exec(storage);

  // Seed data
  {
    auto stmt =
        parseQuery("INSERT INTO users (name, age) VALUES ('Alice', 30)");
    auto res = exec.execute(*stmt);
    assert(res.hasValue());
    const auto &rs = res.value();
    assert(rs.rowCount() == 1);
    assert(rs.at(0, 0).asInt() == 1); // affected
  }
  {
    auto stmt = parseQuery("INSERT INTO users (name, age) VALUES ('Bob', 22)");
    auto res = exec.execute(*stmt);
    assert(res.hasValue());
  }

  // 1) Computed arithmetic: SET age = age + 1 WHERE name = 'Alice'
  {
    auto stmt =
        parseQuery("UPDATE users SET age = age + 1 WHERE name = 'Alice'");
    auto res = exec.execute(*stmt);
    assert(res.hasValue());
    const auto &rs = res.value();
    assert(rs.rowCount() == 1);
    assert(rs.at(0, 0).asInt() == 1); // one row affected

    auto check = parseQuery("SELECT age FROM users WHERE name = 'Alice'");
    auto r2 = exec.execute(*check);
    assert(r2.hasValue());
    const auto &rs2 = r2.value();
    assert(rs2.rowCount() == 1);
    assert(rs2.at(0, 0).asInt() == 31);
  }

  // 2) Computed arithmetic with precedence: SET age = (age * 2) - 5 WHERE name
  // = 'Alice'
  {
    auto stmt =
        parseQuery("UPDATE users SET age = (age * 2) - 5 WHERE name = 'Alice'");
    auto res = exec.execute(*stmt);
    assert(res.hasValue());
    const auto &rs = res.value();
    assert(rs.rowCount() == 1);
    assert(rs.at(0, 0).asInt() == 1);

    auto check = parseQuery("SELECT age FROM users WHERE name = 'Alice'");
    auto r2 = exec.execute(*check);
    assert(r2.hasValue());
    const auto &rs2 = r2.value();
    assert(rs2.rowCount() == 1);
    // From previous 31 -> (31*2)-5 = 57
    assert(rs2.at(0, 0).asInt() == 57);
  }

  // 3) Division by zero error: SET age = age / 0 WHERE name = 'Bob'
  {
    auto stmt = parseQuery("UPDATE users SET age = age / 0 WHERE name = 'Bob'");
    auto res = exec.execute(*stmt);
    assert(!res.hasValue()); // should be error
  }

  // 4) String concatenation: SET name = name + '_2025' WHERE name = 'Alice'
  // Apply to the existing 'Alice' row (currently 'Alice' in table and 'Bob')
  {
    auto stmt = parseQuery(
        "UPDATE users SET name = name + '_2025' WHERE name = 'Alice'");
    auto res = exec.execute(*stmt);
    assert(res.hasValue());
    const auto &rs = res.value();
    assert(rs.rowCount() == 1);
    assert(rs.at(0, 0).asInt() == 1); // one row affected

    auto check = parseQuery("SELECT name FROM users WHERE name = 'Alice_2025'");
    auto r2 = exec.execute(*check);
    assert(r2.hasValue());
    const auto &rs2 = r2.value();
    assert(rs2.rowCount() == 1);
    assert(rs2.at(0, 0).asString() == std::string("Alice_2025"));
  }

  std::cout << "KadeQL computed UPDATE test passed" << std::endl;
  return 0;
}
