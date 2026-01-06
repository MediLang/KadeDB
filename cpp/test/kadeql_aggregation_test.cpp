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
  std::cout << "=== KadeQL Aggregation Tests ===" << std::endl;

  InMemoryRelationalStorage storage;

  // Create a time-series-like table
  TableSchema metrics({
      Column{"timestamp", ColumnType::Integer, /*nullable=*/false},
      Column{"sensor_id", ColumnType::Integer, /*nullable=*/false},
      Column{"value", ColumnType::Integer, /*nullable=*/false},
  });
  auto st = storage.createTable("metrics", metrics);
  assert(st.ok());

  QueryExecutor exec(storage);

  // Insert test data: timestamps 100, 105, 110, 115, 120, 125
  // With values 10, 20, 30, 40, 50, 60
  {
    auto stmt = parseQuery("INSERT INTO metrics (timestamp, sensor_id, value) "
                           "VALUES (100, 1, 10)");
    auto res = exec.execute(*stmt);
    assert(res.hasValue());
  }
  {
    auto stmt = parseQuery("INSERT INTO metrics (timestamp, sensor_id, value) "
                           "VALUES (105, 1, 20)");
    auto res = exec.execute(*stmt);
    assert(res.hasValue());
  }
  {
    auto stmt = parseQuery("INSERT INTO metrics (timestamp, sensor_id, value) "
                           "VALUES (110, 1, 30)");
    auto res = exec.execute(*stmt);
    assert(res.hasValue());
  }
  {
    auto stmt = parseQuery("INSERT INTO metrics (timestamp, sensor_id, value) "
                           "VALUES (115, 1, 40)");
    auto res = exec.execute(*stmt);
    assert(res.hasValue());
  }
  {
    auto stmt = parseQuery("INSERT INTO metrics (timestamp, sensor_id, value) "
                           "VALUES (120, 1, 50)");
    auto res = exec.execute(*stmt);
    assert(res.hasValue());
  }
  {
    auto stmt = parseQuery("INSERT INTO metrics (timestamp, sensor_id, value) "
                           "VALUES (125, 1, 60)");
    auto res = exec.execute(*stmt);
    assert(res.hasValue());
  }

  // Test 1: TIME_BUCKET with 10-second intervals
  std::cout << "Test 1: TIME_BUCKET(timestamp, 10)..." << std::endl;
  {
    auto stmt =
        parseQuery("SELECT TIME_BUCKET(timestamp, 10) AS bucket FROM metrics");
    auto res = exec.execute(*stmt);
    assert(res.hasValue());
    const auto &rs = res.value();
    // Should have 3 buckets: 100, 110, 120
    assert(rs.rowCount() == 3);
    assert(rs.columnNames()[0] == "bucket");
    // Buckets should be 100, 110, 120 (sorted by map)
    assert(rs.at(0, 0).asInt() == 100);
    assert(rs.at(1, 0).asInt() == 110);
    assert(rs.at(2, 0).asInt() == 120);
    std::cout << "  PASSED" << std::endl;
  }

  // Test 2: FIRST(value, timestamp) - get first value ordered by timestamp
  std::cout << "Test 2: FIRST(value, timestamp)..." << std::endl;
  {
    auto stmt =
        parseQuery("SELECT FIRST(value, timestamp) AS first_val FROM metrics");
    auto res = exec.execute(*stmt);
    assert(res.hasValue());
    const auto &rs = res.value();
    assert(rs.rowCount() == 1);
    assert(rs.at(0, 0).asInt() == 10); // First value at timestamp 100
    std::cout << "  PASSED" << std::endl;
  }

  // Test 3: LAST(value, timestamp) - get last value ordered by timestamp
  std::cout << "Test 3: LAST(value, timestamp)..." << std::endl;
  {
    auto stmt =
        parseQuery("SELECT LAST(value, timestamp) AS last_val FROM metrics");
    auto res = exec.execute(*stmt);
    assert(res.hasValue());
    const auto &rs = res.value();
    assert(rs.rowCount() == 1);
    assert(rs.at(0, 0).asInt() == 60); // Last value at timestamp 125
    std::cout << "  PASSED" << std::endl;
  }

  // Test 4: Combined TIME_BUCKET with FIRST and LAST
  std::cout << "Test 4: TIME_BUCKET with FIRST and LAST..." << std::endl;
  {
    auto stmt = parseQuery("SELECT TIME_BUCKET(timestamp, 10) AS bucket, "
                           "FIRST(value, timestamp) AS first_val, "
                           "LAST(value, timestamp) AS last_val "
                           "FROM metrics");
    auto res = exec.execute(*stmt);
    assert(res.hasValue());
    const auto &rs = res.value();
    assert(rs.rowCount() == 3);

    // Bucket 100: timestamps 100, 105 -> values 10, 20
    assert(rs.at(0, 0).asInt() == 100); // bucket
    assert(rs.at(0, 1).asInt() == 10);  // first_val
    assert(rs.at(0, 2).asInt() == 20);  // last_val

    // Bucket 110: timestamps 110, 115 -> values 30, 40
    assert(rs.at(1, 0).asInt() == 110);
    assert(rs.at(1, 1).asInt() == 30);
    assert(rs.at(1, 2).asInt() == 40);

    // Bucket 120: timestamps 120, 125 -> values 50, 60
    assert(rs.at(2, 0).asInt() == 120);
    assert(rs.at(2, 1).asInt() == 50);
    assert(rs.at(2, 2).asInt() == 60);

    std::cout << "  PASSED" << std::endl;
  }

  // Test 5: TIME_BUCKET with WHERE clause
  std::cout << "Test 5: TIME_BUCKET with WHERE clause..." << std::endl;
  {
    auto stmt = parseQuery("SELECT TIME_BUCKET(timestamp, 10) AS bucket, "
                           "FIRST(value, timestamp) AS first_val "
                           "FROM metrics WHERE timestamp >= 110");
    auto res = exec.execute(*stmt);
    assert(res.hasValue());
    const auto &rs = res.value();
    assert(rs.rowCount() == 2); // Only buckets 110 and 120
    assert(rs.at(0, 0).asInt() == 110);
    assert(rs.at(1, 0).asInt() == 120);
    std::cout << "  PASSED" << std::endl;
  }

  // Test 6: FIRST with single argument (uses timestamp column by default)
  std::cout << "Test 6: FIRST(value) with implicit timestamp ordering..."
            << std::endl;
  {
    auto stmt = parseQuery("SELECT FIRST(value) AS first_val FROM metrics");
    auto res = exec.execute(*stmt);
    assert(res.hasValue());
    const auto &rs = res.value();
    assert(rs.rowCount() == 1);
    assert(rs.at(0, 0).asInt() == 10); // First value
    std::cout << "  PASSED" << std::endl;
  }

  // Test 7: Expression-based SELECT without aggregates (col AS alias)
  std::cout << "Test 7: SELECT col AS alias (expression mode, no aggregates)..."
            << std::endl;
  {
    auto stmt = parseQuery("SELECT value AS val, timestamp AS ts FROM metrics "
                           "WHERE timestamp = 100");
    auto res = exec.execute(*stmt);
    assert(res.hasValue());
    const auto &rs = res.value();
    assert(rs.rowCount() == 1);
    assert(rs.columnNames()[0] == "val");
    assert(rs.columnNames()[1] == "ts");
    assert(rs.at(0, 0).asInt() == 10);
    assert(rs.at(0, 1).asInt() == 100);
    std::cout << "  PASSED" << std::endl;
  }

  // Test 8: Parser test for function call syntax
  std::cout << "Test 8: Parser function call syntax..." << std::endl;
  {
    KadeQLParser parser;
    auto stmt =
        parser.parse("SELECT TIME_BUCKET(ts, 60), FIRST(val, ts) FROM data");
    auto select = dynamic_cast<SelectStatement *>(stmt.get());
    assert(select != nullptr);
    assert(select->isExpressionMode());
    assert(select->getSelectItems().size() == 2);
    std::cout << "  Parsed: " << select->toString() << std::endl;
    std::cout << "  PASSED" << std::endl;
  }

  std::cout << "\nAll KadeQL Aggregation tests passed!" << std::endl;
  return 0;
}
