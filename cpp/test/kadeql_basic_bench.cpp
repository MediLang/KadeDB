#include "kadedb/kadeql.h"
#include "kadedb/query_executor.h"
#include "kadedb/schema.h"
#include "kadedb/storage.h"
#include "kadedb/value.h"

#include <chrono>
#include <iostream>

using namespace kadedb;
using namespace kadedb::kadeql;

static void bench_insert_and_select(size_t N) {
  InMemoryRelationalStorage storage;
  TableSchema users({
      Column{"id", ColumnType::Integer, /*nullable=*/false, /*unique=*/true},
      Column{"name", ColumnType::String, /*nullable=*/false},
      Column{"age", ColumnType::Integer, /*nullable=*/false},
  });
  auto st = storage.createTable("users", users);
  if (!st.ok()) {
    std::cerr << "createTable failed: " << st.message() << "\n";
    return;
  }

  QueryExecutor exec(storage);

  auto t0 = std::chrono::steady_clock::now();
  for (size_t i = 0; i < N; ++i) {
    std::string q = "INSERT INTO users (id, name, age) VALUES (" +
                    std::to_string(i) + ", 'u', " +
                    std::to_string(20 + (i % 50)) + ")";
    auto stmt = parseQuery(q);
    auto res = exec.execute(*stmt);
    if (!res.hasValue()) {
      std::cerr << "insert error: " << res.status().message() << "\n";
      return;
    }
  }
  auto t1 = std::chrono::steady_clock::now();

  // Simple SELECT with predicate
  auto stmt = parseQuery("SELECT id FROM users WHERE age >= 40");
  auto t2 = std::chrono::steady_clock::now();
  auto res = exec.execute(*stmt);
  auto t3 = std::chrono::steady_clock::now();

  auto ins_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
  auto parse_ms =
      std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
  auto sel_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(t3 - t2).count();

  std::cout << "bench N=" << N << " insert_ms=" << ins_ms
            << " parse_us=" << parse_ms << " select_ms=" << sel_ms;
  if (res.hasValue()) {
    std::cout << " rows=" << res.value().rowCount();
  } else {
    std::cout << " select_error='" << res.status().message() << "'";
  }
  std::cout << "\n";
}

int main() {
  bench_insert_and_select(1000);
  bench_insert_and_select(5000);
  return 0;
}
