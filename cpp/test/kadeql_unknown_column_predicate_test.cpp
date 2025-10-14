#include "kadedb/kadeql.h"
#include "kadedb/query_executor.h"
#include "kadedb/schema.h"
#include "kadedb/status.h"
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

  QueryExecutor exec(storage);

  // SELECT with unknown column in WHERE should produce InvalidArgument
  {
    auto stmt = parseQuery("SELECT name FROM users WHERE unknown_col = 1");
    auto res = exec.execute(*stmt);
    assert(!res.hasValue());
    assert(res.status().code() == StatusCode::InvalidArgument);
  }

  // UPDATE with unknown column in WHERE should produce InvalidArgument
  {
    auto stmt = parseQuery("UPDATE users SET age = 10 WHERE missing = 0");
    auto res = exec.execute(*stmt);
    assert(!res.hasValue());
    assert(res.status().code() == StatusCode::InvalidArgument);
  }

  // DELETE with unknown column in WHERE should produce InvalidArgument
  {
    auto stmt = parseQuery("DELETE FROM users WHERE nope = 42");
    auto res = exec.execute(*stmt);
    assert(!res.hasValue());
    assert(res.status().code() == StatusCode::InvalidArgument);
  }

  std::cout << "Unknown column predicate validation tests passed" << std::endl;
  return 0;
}
