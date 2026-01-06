#include "kadedb/timeseries/storage.h"

#include "kadedb/schema.h"
#include "kadedb/value.h"

#include <iostream>
#include <optional>
#include <string>
#include <vector>

using namespace kadedb;

static void printResultSet(const ResultSet &rs) {
  for (size_t c = 0; c < rs.columnCount(); ++c) {
    if (c)
      std::cout << ", ";
    std::cout << rs.columnNames()[c];
  }
  std::cout << "\n";

  for (size_t r = 0; r < rs.rowCount(); ++r) {
    for (size_t c = 0; c < rs.columnCount(); ++c) {
      if (c)
        std::cout << ", ";
      std::cout << rs.at(r, c).toString();
    }
    std::cout << "\n";
  }
}

int main() {
  InMemoryTimeSeriesStorage ts;

  TimeSeriesSchema schema("timestamp", TimeGranularity::Seconds);

  Column sensor;
  sensor.name = "sensor_id";
  sensor.type = ColumnType::Integer;
  sensor.nullable = false;
  schema.addTagColumn(sensor);

  Column value;
  value.name = "value";
  value.type = ColumnType::Integer;
  value.nullable = false;
  schema.addValueColumn(value);

  auto st = ts.createSeries("metrics", schema, TimePartition::Hourly);
  if (!st.ok()) {
    std::cerr << "createSeries failed: " << st.message() << "\n";
    return 1;
  }

  TableSchema table(schema.allColumns());

  auto makeRow = [&](int64_t t, int64_t sid, int64_t v) {
    Row r(table.columns().size());
    r.set(table.findColumn("timestamp"), ValueFactory::createInteger(t));
    r.set(table.findColumn("sensor_id"), ValueFactory::createInteger(sid));
    r.set(table.findColumn("value"), ValueFactory::createInteger(v));
    return r;
  };

  ts.append("metrics", makeRow(100, 1, 10));
  ts.append("metrics", makeRow(105, 1, 20));
  ts.append("metrics", makeRow(110, 2, 30));
  ts.append("metrics", makeRow(115, 2, 40));

  std::cout << "=== rangeQuery: [100, 116) ===\n";
  {
    auto res = ts.rangeQuery("metrics", {}, 100, 116, std::nullopt);
    if (!res.hasValue()) {
      std::cerr << "rangeQuery failed: " << res.status().message() << "\n";
      return 1;
    }
    printResultSet(res.value());
  }

  std::cout << "\n=== rangeQuery with predicate: sensor_id = 2 ===\n";
  {
    Predicate p;
    p.kind = Predicate::Kind::Comparison;
    p.column = "sensor_id";
    p.op = Predicate::Op::Eq;
    p.rhs = ValueFactory::createInteger(2);
    std::optional<Predicate> where;
    where.emplace(std::move(p));

    auto res = ts.rangeQuery("metrics", {"timestamp", "sensor_id", "value"}, 0,
                             1000, where);
    if (!res.hasValue()) {
      std::cerr << "rangeQuery failed: " << res.status().message() << "\n";
      return 1;
    }
    printResultSet(res.value());
  }

  std::cout << "\n=== aggregate: AVG(value), 10-second buckets from 100 ===\n";
  {
    auto res = ts.aggregate("metrics", "value", TimeAggregation::Avg,
                            /*startInclusive=*/100, /*endExclusive=*/130,
                            /*bucketWidth=*/10, TimeGranularity::Seconds,
                            std::nullopt);
    if (!res.hasValue()) {
      std::cerr << "aggregate failed: " << res.status().message() << "\n";
      return 1;
    }
    printResultSet(res.value());
  }

  return 0;
}
