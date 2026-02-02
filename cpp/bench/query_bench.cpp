#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "kadedb/schema.h"
#include "kadedb/storage.h"
#include "kadedb/timeseries/storage.h"
#include "kadedb/value.h"

using namespace kadedb;

static int64_t parseInt64(const char *s, int64_t def) {
  if (!s)
    return def;
  try {
    return std::stoll(std::string(s));
  } catch (...) {
    return def;
  }
}

template <class Fun> static double time_ms(Fun &&f) {
  auto start = std::chrono::high_resolution_clock::now();
  f();
  auto end = std::chrono::high_resolution_clock::now();
  return std::chrono::duration<double, std::milli>(end - start).count();
}

static TableSchema makeRelSchema() {
  Column id{"id", ColumnType::Integer, false, true, {}};
  Column x{"x", ColumnType::Integer, false, false, {}};
  Column y{"y", ColumnType::Float, false, false, {}};
  return TableSchema({id, x, y}, std::optional<std::string>("id"));
}

static Row makeRelRow(int64_t id, int64_t x, double y) {
  Row r(3);
  r.set(0, ValueFactory::createInteger(id));
  r.set(1, ValueFactory::createInteger(x));
  r.set(2, ValueFactory::createFloat(y));
  return r;
}

static TimeSeriesSchema makeTsSchema() {
  TimeSeriesSchema s("timestamp", TimeGranularity::Seconds);
  s.addValueColumn(Column{"value", ColumnType::Float, false, false, {}});
  return s;
}

static Row makeTsRow(int64_t ts, double v) {
  Row r(2);
  r.set(0, ValueFactory::createInteger(ts));
  r.set(1, ValueFactory::createFloat(v));
  return r;
}

int main(int argc, char **argv) {
  const int64_t relRows = (argc > 1) ? parseInt64(argv[1], 200000) : 200000;
  const int64_t tsRows = (argc > 2) ? parseInt64(argv[2], 200000) : 200000;

  std::cout << "KadeDB Query Bench (CPU baseline)\n";
  std::cout << "Relational rows: " << relRows << "\n";
  std::cout << "Timeseries rows: " << tsRows << "\n\n";

  // ---- Relational scan/filter ----
  InMemoryRelationalStorage rel;
  (void)rel.createTable("t", makeRelSchema());

  std::mt19937_64 rng(42);
  std::uniform_int_distribution<int64_t> xd(0, 1000000);
  std::uniform_real_distribution<double> yd(0.0, 1.0);

  double ms_insert_rel = time_ms([&]() {
    for (int64_t i = 0; i < relRows; ++i) {
      (void)rel.insertRow("t", makeRelRow(i, xd(rng), yd(rng)));
    }
  });

  Predicate pred;
  pred.kind = Predicate::Kind::Comparison;
  pred.column = "x";
  pred.op = Predicate::Op::Lt;
  pred.rhs = ValueFactory::createInteger(100000); // ~10% selectivity

  double ms_select_rel = time_ms([&]() {
    auto res = rel.select("t", /*columns=*/{},
                          std::optional<Predicate>(std::move(pred)));
    if (!res.hasValue()) {
      std::cerr << "select failed: " << res.status().message() << "\n";
      std::exit(1);
    }
    volatile size_t sink = res.value().rowCount();
    (void)sink;
  });

  std::cout << "Relational:\n";
  std::cout << "  insert ms: " << std::fixed << std::setprecision(2)
            << ms_insert_rel << "\n";
  std::cout << "  select  ms: " << std::fixed << std::setprecision(2)
            << ms_select_rel << "\n\n";

  // ---- Time-series range + aggregation ----
  InMemoryTimeSeriesStorage ts;
  auto tsSchema = makeTsSchema();
  (void)ts.createSeries("s", tsSchema, TimePartition::Hourly);

  const int64_t baseTs = 1700000000;
  double ms_insert_ts = time_ms([&]() {
    for (int64_t i = 0; i < tsRows; ++i) {
      (void)ts.append("s", makeTsRow(baseTs + i, yd(rng)));
    }
  });

  const int64_t start = baseTs + 0;
  const int64_t end = baseTs + tsRows;

  double ms_range = time_ms([&]() {
    auto res = ts.rangeQuery("s", /*columns=*/{}, start, end, std::nullopt);
    if (!res.hasValue()) {
      std::cerr << "rangeQuery failed: " << res.status().message() << "\n";
      std::exit(1);
    }
    volatile size_t sink = res.value().rowCount();
    (void)sink;
  });

  double ms_agg = time_ms([&]() {
    auto res = ts.aggregate("s", "value", TimeAggregation::Sum, start, end,
                            /*bucketWidth=*/60, TimeGranularity::Seconds,
                            std::nullopt);
    if (!res.hasValue()) {
      std::cerr << "aggregate failed: " << res.status().message() << "\n";
      std::exit(1);
    }
    volatile size_t sink = res.value().rowCount();
    (void)sink;
  });

  std::cout << "TimeSeries:\n";
  std::cout << "  insert ms: " << std::fixed << std::setprecision(2)
            << ms_insert_ts << "\n";
  std::cout << "  range  ms: " << std::fixed << std::setprecision(2) << ms_range
            << "\n";
  std::cout << "  agg    ms: " << std::fixed << std::setprecision(2) << ms_agg
            << "\n";

  return 0;
}
