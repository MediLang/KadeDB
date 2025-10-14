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

  // Seed data via executor + parser
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

  // 1) Flattening and dedup: ((A AND B) AND A) -> (A AND B)
  // A: age >= 30, B: age <= 40. Expect Alice(30) and Carol(40) => 2 rows.
  {
    auto stmt = parseQuery(
        "SELECT name FROM users WHERE (age >= 30 AND age <= 40) AND age >= 30");
    auto res = exec.execute(*stmt);
    assert(res.hasValue());
    const auto &rs = res.value();
    assert(rs.rowCount() == 2);
  }

  // 2) Double NOT: NOT NOT A -> A. Expect ONLY Bob for age = 22.
  {
    auto stmt = parseQuery("SELECT name FROM users WHERE NOT NOT (age = 22)");
    auto res = exec.execute(*stmt);
    assert(res.hasValue());
    const auto &rs = res.value();
    assert(rs.rowCount() == 1);
    assert(rs.at(0, 0).asString() == std::string("Bob"));
  }

  // 3) De Morgan: NOT (name = 'Alice' OR name = 'Bob') -> only Carol
  {
    auto stmt = parseQuery(
        "SELECT name FROM users WHERE NOT (name = 'Alice' OR name = 'Bob')");
    auto res = exec.execute(*stmt);
    assert(res.hasValue());
    const auto &rs = res.value();
    assert(rs.rowCount() == 1);
    assert(rs.at(0, 0).asString() == std::string("Carol"));
  }

  // 4) Deterministic ordering (result equivalence):
  //    (age = 22 OR name = 'Alice') vs (name = 'Alice' OR age = 22)
  //    Both should yield Alice and Bob => 2 rows.
  {
    auto stmt1 =
        parseQuery("SELECT name FROM users WHERE (age = 22 OR name = 'Alice')");
    auto res1 = exec.execute(*stmt1);
    assert(res1.hasValue());
    const auto &rs1 = res1.value();
    assert(rs1.rowCount() == 2);

    auto stmt2 =
        parseQuery("SELECT name FROM users WHERE (name = 'Alice' OR age = 22)");
    auto res2 = exec.execute(*stmt2);
    assert(res2.hasValue());
    const auto &rs2 = res2.value();
    assert(rs2.rowCount() == 2);
  }

  // 5) Constant folding: NOT (1 < 2) -> false, short-circuit with AND -> 0 rows
  {
    auto stmt =
        parseQuery("SELECT name FROM users WHERE (NOT (1 < 2)) AND age >= 0");
    auto res = exec.execute(*stmt);
    assert(res.hasValue());
    const auto &rs = res.value();
    assert(rs.rowCount() == 0);
  }

  // 6) Constant folding with OR true: (1 < 2) OR (age = 999) -> all rows (3)
  {
    auto stmt =
        parseQuery("SELECT name FROM users WHERE ((1 < 2) OR (age = 999))");
    auto res = exec.execute(*stmt);
    assert(res.hasValue());
    const auto &rs = res.value();
    assert(rs.rowCount() == 3);
  }

  // 7) Nested constants short-circuit: ((1 < 2) AND (age = 22)) OR (1 = 0)
  //    First conjunct true, so reduces to (age = 22) OR false -> Bob only
  {
    auto stmt = parseQuery(
        "SELECT name FROM users WHERE ((1 < 2) AND (age = 22)) OR (1 = 0)");
    auto res = exec.execute(*stmt);
    assert(res.hasValue());
    const auto &rs = res.value();
    assert(rs.rowCount() == 1);
    assert(rs.at(0, 0).asString() == std::string("Bob"));
  }

  std::cout << "KadeQL optimizer canonicalization tests passed" << std::endl;
  return 0;
}
