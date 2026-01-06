#include "kadedb/timeseries/storage.h"

#include "kadedb/schema.h"
#include "kadedb/value.h"

#include <cassert>
#include <iostream>

using namespace kadedb;

static TimeSeriesSchema
makeSchema(TimeGranularity g,
           std::optional<RetentionPolicy> ret = std::nullopt) {
  TimeSeriesSchema schema("timestamp", g);

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

  if (ret.has_value()) {
    schema.setRetentionPolicy(*ret);
  }

  return schema;
}

static Row makeRow(const TableSchema &ts, int64_t timestamp, int64_t sensorId,
                   int64_t value) {
  Row r(ts.columns().size());
  r.set(ts.findColumn("timestamp"), ValueFactory::createInteger(timestamp));
  r.set(ts.findColumn("sensor_id"), ValueFactory::createInteger(sensorId));
  r.set(ts.findColumn("value"), ValueFactory::createInteger(value));
  return r;
}

int main() {
  std::cout << "timeseries_test starting\n";

  InMemoryTimeSeriesStorage ts;

  // createSeries + listSeries + dropSeries
  {
    auto schema = makeSchema(TimeGranularity::Seconds);
    auto st = ts.createSeries("cpu", schema, TimePartition::Hourly);
    assert(st.ok());

    auto st2 = ts.createSeries("cpu", schema, TimePartition::Hourly);
    assert(!st2.ok());
    assert(st2.code() == StatusCode::AlreadyExists);

    auto names = ts.listSeries();
    bool found = false;
    for (const auto &n : names) {
      if (n == std::string("cpu")) {
        found = true;
        break;
      }
    }
    assert(found);

    auto stDrop = ts.dropSeries("cpu");
    assert(stDrop.ok());

    auto stDrop2 = ts.dropSeries("cpu");
    assert(!stDrop2.ok());
    assert(stDrop2.code() == StatusCode::NotFound);
  }

  // Basic append + rangeQuery + projection
  {
    auto schema = makeSchema(TimeGranularity::Seconds);
    auto st = ts.createSeries("cpu", schema, TimePartition::Hourly);
    assert(st.ok());

    TableSchema table(schema.allColumns());

    // timestamps are already seconds
    assert(ts.append("cpu", makeRow(table, 100, 1, 10)).ok());
    assert(ts.append("cpu", makeRow(table, 105, 1, 20)).ok());
    assert(ts.append("cpu", makeRow(table, 110, 2, 30)).ok());

    // [100, 110): should include 100 and 105 only
    {
      auto res = ts.rangeQuery("cpu", /*columns=*/{}, 100, 110, std::nullopt);
      assert(res.hasValue());
      const ResultSet &rs = res.value();
      assert(rs.rowCount() == 2);
      assert(rs.findColumn("timestamp") != ResultSet::npos);
      assert(rs.findColumn("sensor_id") != ResultSet::npos);
      assert(rs.findColumn("value") != ResultSet::npos);
    }

    // Projection: only value
    {
      auto res = ts.rangeQuery("cpu", {"value"}, 100, 200, std::nullopt);
      assert(res.hasValue());
      const ResultSet &rs = res.value();
      assert(rs.columnCount() == 1);
      assert(rs.columnNames()[0] == "value");
      assert(rs.rowCount() == 3);
    }

    // Predicate: sensor_id == 1
    {
      Predicate p;
      p.kind = Predicate::Kind::Comparison;
      p.column = "sensor_id";
      p.op = Predicate::Op::Eq;
      p.rhs = ValueFactory::createInteger(1);

      std::optional<Predicate> where;
      where.emplace(std::move(p));

      auto res = ts.rangeQuery("cpu", /*columns=*/{}, 0, 1000, where);
      assert(res.hasValue());
      const ResultSet &rs = res.value();
      assert(rs.rowCount() == 2);
    }

    // aggregate SUM by 10-second buckets starting at startInclusive
    {
      auto res = ts.aggregate("cpu", "value", TimeAggregation::Sum,
                              /*startInclusive=*/100, /*endExclusive=*/130,
                              /*bucketWidth=*/10, TimeGranularity::Seconds,
                              std::nullopt);
      assert(res.hasValue());
      const ResultSet &rs = res.value();
      assert(rs.columnCount() == 2);
      assert(rs.columnNames()[0] == "bucket_start");
      assert(rs.columnNames()[1] == "value");
      // data points at 100,105,110 -> buckets 100 and 110
      assert(rs.rowCount() == 2);
      assert(rs.at(0, 0).asInt() == 100);
      assert(rs.at(1, 0).asInt() == 110);
      // SUM: bucket 100 has 10+20 = 30, bucket 110 has 30
      assert(rs.at(0, 1).asFloat() == 30.0);
      assert(rs.at(1, 1).asFloat() == 30.0);
    }

    ts.dropSeries("cpu");
  }

  // Retention: ttlSeconds
  {
    RetentionPolicy rp;
    rp.ttlSeconds = 10;
    rp.maxRows = 0;
    rp.dropOldest = true;

    auto schema = makeSchema(TimeGranularity::Seconds, rp);
    auto st = ts.createSeries("mem", schema, TimePartition::Hourly);
    assert(st.ok());
    TableSchema table(schema.allColumns());

    // Append rows at 0, 5, 20 seconds. The append at 20 should expire < 10
    // cutoff = 20 - 10 = 10 => drop rows with tsec < 10
    assert(ts.append("mem", makeRow(table, 0, 1, 1)).ok());
    assert(ts.append("mem", makeRow(table, 5, 1, 1)).ok());
    assert(ts.append("mem", makeRow(table, 20, 1, 1)).ok());

    auto res = ts.rangeQuery("mem", /*columns=*/{}, -1000, 1000, std::nullopt);
    assert(res.hasValue());
    const ResultSet &rs = res.value();
    assert(rs.rowCount() == 1);
    assert(rs.at(0, rs.findColumn("timestamp")).asInt() == 20);

    ts.dropSeries("mem");
  }

  // Retention: maxRows dropOldest
  {
    RetentionPolicy rp;
    rp.ttlSeconds = 0;
    rp.maxRows = 2;
    rp.dropOldest = true;

    auto schema = makeSchema(TimeGranularity::Seconds, rp);
    auto st = ts.createSeries("disk", schema, TimePartition::Hourly);
    assert(st.ok());
    TableSchema table(schema.allColumns());

    assert(ts.append("disk", makeRow(table, 1, 1, 11)).ok());
    assert(ts.append("disk", makeRow(table, 2, 1, 22)).ok());
    assert(ts.append("disk", makeRow(table, 3, 1, 33)).ok());

    auto res = ts.rangeQuery("disk", /*columns=*/{}, 0, 100, std::nullopt);
    assert(res.hasValue());
    const ResultSet &rs = res.value();
    // maxRows=2, so oldest should be dropped => keep timestamps 2 and 3
    assert(rs.rowCount() == 2);

    // Verify timestamp 1 not present
    size_t tsIdx = rs.findColumn("timestamp");
    bool has1 = false;
    bool has2 = false;
    bool has3 = false;
    for (size_t i = 0; i < rs.rowCount(); ++i) {
      int64_t t = rs.at(i, tsIdx).asInt();
      has1 |= (t == 1);
      has2 |= (t == 2);
      has3 |= (t == 3);
    }
    assert(!has1);
    assert(has2);
    assert(has3);

    ts.dropSeries("disk");
  }

  std::cout << "timeseries_test passed\n";
  return 0;
}
