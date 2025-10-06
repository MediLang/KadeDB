#include "kadedb/kadeql.h"
#include "kadedb/query_executor.h"
#include "kadedb/schema.h"
#include "kadedb/storage.h"
#include "kadedb/value.h"

#include <cassert>
#include <iostream>
#include <memory>

using namespace kadedb;
using namespace kadedb::kadeql;

static void printStatus(const Status &st) {
  std::cout << "Status: code=" << static_cast<int>(st.code()) << ", message='"
            << st.message() << "'\n";
}

int main() {
  // Set up in-memory storage with a simple schema
  InMemoryRelationalStorage storage;
  TableSchema users({
      Column{"name", ColumnType::String, /*nullable=*/false},
      Column{"age", ColumnType::Integer, /*nullable=*/false, /*unique=*/true},
      Column{"email", ColumnType::String, /*nullable=*/true},
  });
  auto st = storage.createTable("users", users);
  assert(st.ok());

  QueryExecutor exec(storage);

  // 1) INSERT with explicit columns (single row)
  {
    auto stmt = parseQuery(
        "INSERT INTO users (name, age, email) VALUES ('Alice', 30, 'a@x.com')");
    auto res = exec.execute(*stmt);
    assert(res.hasValue());
    const auto &rs = res.value();
    assert(rs.rowCount() == 1);
    // affected/inserted should both be 1
    assert(rs.at(0, 0).asInt() == 1);
    assert(rs.at(0, 1).asInt() == 1);
  }

  // 2) INSERT implicit columns order (table order)
  {
    auto stmt =
        parseQuery("INSERT INTO users VALUES ('Bob', 22, 'bob@example.com')");
    auto res = exec.execute(*stmt);
    assert(res.hasValue());
  }

  // 3) Multi-row INSERT (explicit columns)
  {
    auto stmt = parseQuery(
        "INSERT INTO users (name, age, email) VALUES ('Carl', 40, 'c@x'),"
        "('Dana', 28, 'd@x')");
    auto res = exec.execute(*stmt);
    assert(res.hasValue());
    // affected/inserted should be 2
    assert(res.value().at(0, 0).asInt() == 2);
  }

  // Verify row count via SELECT *
  {
    auto stmt = parseQuery("SELECT * FROM users");
    auto res = exec.execute(*stmt);
    assert(res.hasValue());
    // 1 (Alice) + 1 (Bob) + 2 (Carl, Dana) = 4
    assert(res.value().rowCount() == 4);
  }

  // 4) Unknown column in INSERT should error (executor validation)
  {
    auto stmt = parseQuery(
        "INSERT INTO users (nope, age, email) VALUES ('X', 33, 'x@x')");
    auto res = exec.execute(*stmt);
    assert(!res.hasValue());
    printStatus(res.status());
  }

  // 5) VALUES arity mismatch with columns should be caught at parse-time
  // Parser enforces arity; expect ParseError
  try {
    auto bad =
        parseQuery("INSERT INTO users (name, age) VALUES ('Zed', 44, 'z@x')");
    (void)bad; // should not reach
    assert(false);
  } catch (const ParseError &) {
    // expected
  }

  // 6) Type mismatch (e.g., age as string) -> storage InvalidArgument
  {
    auto stmt =
        parseQuery("INSERT INTO users (name, age) VALUES ('Eve', 'oops')");
    auto res = exec.execute(*stmt);
    assert(!res.hasValue());
    printStatus(res.status());
  }

  // 7) Uniqueness violation on age (age is unique) -> FailedPrecondition
  {
    // We already inserted age=30 for Alice. Try to insert another with age=30
    auto stmt = parseQuery(
        "INSERT INTO users (name, age, email) VALUES ('Alicia', 30, 'ax2@x')");
    auto res = exec.execute(*stmt);
    assert(!res.hasValue());
    printStatus(res.status());
  }

  std::cout << "KadeQL INSERT integration tests passed" << std::endl;
  return 0;
}
