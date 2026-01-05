#include "kadedb/timeseries/storage.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace kadedb {
namespace {

static int64_t toSeconds(int64_t ts, TimeGranularity g) {
  switch (g) {
  case TimeGranularity::Nanoseconds:
    return ts / 1000000000LL;
  case TimeGranularity::Microseconds:
    return ts / 1000000LL;
  case TimeGranularity::Milliseconds:
    return ts / 1000LL;
  case TimeGranularity::Seconds:
    return ts;
  case TimeGranularity::Minutes:
    return ts * 60LL;
  case TimeGranularity::Hours:
    return ts * 3600LL;
  case TimeGranularity::Days:
    return ts * 86400LL;
  }
  return ts;
}

static int64_t bucketWidthSeconds(int64_t width, TimeGranularity g) {
  if (width <= 0)
    return 0;
  switch (g) {
  case TimeGranularity::Nanoseconds:
    return std::max<int64_t>(1, width / 1000000000LL);
  case TimeGranularity::Microseconds:
    return std::max<int64_t>(1, width / 1000000LL);
  case TimeGranularity::Milliseconds:
    return std::max<int64_t>(1, width / 1000LL);
  case TimeGranularity::Seconds:
    return width;
  case TimeGranularity::Minutes:
    return width * 60LL;
  case TimeGranularity::Hours:
    return width * 3600LL;
  case TimeGranularity::Days:
    return width * 86400LL;
  }
  return width;
}

static int64_t partitionBucketStartSeconds(int64_t timeSec, TimePartition p) {
  const int64_t div = (p == TimePartition::Daily) ? 86400LL : 3600LL;
  if (div <= 0)
    return timeSec;
  if (timeSec >= 0)
    return (timeSec / div) * div;
  // For negative timestamps, floor division
  int64_t q = timeSec / div;
  int64_t r = timeSec % div;
  if (r != 0)
    --q;
  return q * div;
}

static bool evalPredicateComparison(const TableSchema &schema, const Row &row,
                                    const Predicate &pred) {
  size_t idx = schema.findColumn(pred.column);
  if (idx == TableSchema::npos)
    return false;
  const Value *lhs = row.values()[idx].get();
  const Value *rhs = pred.rhs.get();
  if (!lhs || !rhs)
    return false;
  int cmp = lhs->compare(*rhs);
  switch (pred.op) {
  case Predicate::Op::Eq:
    return cmp == 0;
  case Predicate::Op::Ne:
    return cmp != 0;
  case Predicate::Op::Lt:
    return cmp < 0;
  case Predicate::Op::Le:
    return cmp <= 0;
  case Predicate::Op::Gt:
    return cmp > 0;
  case Predicate::Op::Ge:
    return cmp >= 0;
  }
  return false;
}

static bool evalPredicate(const TableSchema &schema, const Row &row,
                          const Predicate &pred) {
  using K = Predicate::Kind;
  switch (pred.kind) {
  case K::Comparison:
    return evalPredicateComparison(schema, row, pred);
  case K::And: {
    for (const auto &ch : pred.children) {
      if (!evalPredicate(schema, row, ch))
        return false;
    }
    return true;
  }
  case K::Or: {
    for (const auto &ch : pred.children) {
      if (evalPredicate(schema, row, ch))
        return true;
    }
    return false;
  }
  case K::Not: {
    if (pred.children.empty())
      return false;
    return !evalPredicate(schema, row, pred.children.front());
  }
  }
  return false;
}

static Result<ResultSet> projectionUnknownColumn(const std::string &name) {
  return Result<ResultSet>::err(
      Status::InvalidArgument("Unknown column in projection: " + name));
}

static std::vector<int64_t>
sortedBucketKeys(const std::unordered_map<int64_t, std::vector<Row>> &buckets) {
  std::vector<int64_t> keys;
  keys.reserve(buckets.size());
  for (const auto &kv : buckets)
    keys.push_back(kv.first);
  std::sort(keys.begin(), keys.end());
  return keys;
}

static size_t
totalRows(const std::unordered_map<int64_t, std::vector<Row>> &buckets) {
  size_t n = 0;
  for (const auto &kv : buckets)
    n += kv.second.size();
  return n;
}

static int64_t floorDiv(int64_t a, int64_t b) {
  // b must be > 0
  if (b <= 0)
    return 0;
  if (a >= 0)
    return a / b;
  // For negatives, ensure floor semantics
  int64_t q = a / b;
  int64_t r = a % b;
  if (r != 0)
    --q;
  return q;
}

} // namespace

Status InMemoryTimeSeriesStorage::createSeries(const std::string &series,
                                               const TimeSeriesSchema &schema,
                                               TimePartition partition) {
  std::lock_guard<std::mutex> lk(mtx_);
  if (series_.find(series) != series_.end())
    return Status::AlreadyExists("Series already exists: " + series);

  SeriesData sd;
  sd.schema = schema;
  sd.partition = partition;

  auto cols = schema.allColumns();
  sd.tableSchema = TableSchema(std::move(cols));

  series_.emplace(series, std::move(sd));
  return Status::OK();
}

Status InMemoryTimeSeriesStorage::dropSeries(const std::string &series) {
  std::lock_guard<std::mutex> lk(mtx_);
  auto it = series_.find(series);
  if (it == series_.end())
    return Status::NotFound("Unknown series: " + series);
  series_.erase(it);
  return Status::OK();
}

std::vector<std::string> InMemoryTimeSeriesStorage::listSeries() const {
  std::lock_guard<std::mutex> lk(mtx_);
  std::vector<std::string> out;
  out.reserve(series_.size());
  for (const auto &kv : series_)
    out.push_back(kv.first);
  return out;
}

Status InMemoryTimeSeriesStorage::append(const std::string &series,
                                         const Row &row) {
  std::lock_guard<std::mutex> lk(mtx_);
  auto it = series_.find(series);
  if (it == series_.end())
    return Status::NotFound("Unknown series: " + series);

  auto &sd = it->second;
  if (auto err = SchemaValidator::validateRow(sd.tableSchema, row);
      !err.empty()) {
    return Status::InvalidArgument(err);
  }

  size_t tsIdx = sd.tableSchema.findColumn(sd.schema.timestampColumn());
  if (tsIdx == TableSchema::npos)
    return Status::FailedPrecondition("Timestamp column missing from schema");

  const Value *tsv = row.values()[tsIdx].get();
  if (!tsv || tsv->type() != ValueType::Integer)
    return Status::InvalidArgument("Timestamp value must be an integer");

  int64_t ts = static_cast<const IntegerValue &>(*tsv).asInt();
  int64_t tsec = toSeconds(ts, sd.schema.granularity());
  int64_t bstart = partitionBucketStartSeconds(tsec, sd.partition);

  sd.buckets[bstart].push_back(row.clone());

  const auto &ret = sd.schema.retentionPolicy();

  if (ret.ttlSeconds > 0) {
    int64_t cutoff = tsec - static_cast<int64_t>(ret.ttlSeconds);
    auto keys = sortedBucketKeys(sd.buckets);
    for (int64_t k : keys) {
      if (k + 86400LL < cutoff) {
        sd.buckets.erase(k);
        continue;
      }
      auto bit = sd.buckets.find(k);
      if (bit == sd.buckets.end())
        continue;
      auto &vec = bit->second;
      vec.erase(
          std::remove_if(vec.begin(), vec.end(),
                         [&](const Row &r) {
                           const Value *v = r.values()[tsIdx].get();
                           if (!v || v->type() != ValueType::Integer)
                             return true;
                           int64_t rsec = toSeconds(
                               static_cast<const IntegerValue &>(*v).asInt(),
                               sd.schema.granularity());
                           return rsec < cutoff;
                         }),
          vec.end());
      if (vec.empty())
        sd.buckets.erase(k);
    }
  }

  if (ret.maxRows > 0 && ret.dropOldest) {
    while (totalRows(sd.buckets) > ret.maxRows) {
      auto keys = sortedBucketKeys(sd.buckets);
      if (keys.empty())
        break;
      int64_t oldest = keys.front();
      auto bit = sd.buckets.find(oldest);
      if (bit == sd.buckets.end())
        break;
      auto &vec = bit->second;
      if (!vec.empty()) {
        vec.erase(vec.begin());
      }
      if (vec.empty()) {
        sd.buckets.erase(bit);
      }
    }
  }

  return Status::OK();
}

Result<ResultSet> InMemoryTimeSeriesStorage::rangeQuery(
    const std::string &series, const std::vector<std::string> &columns,
    int64_t startInclusive, int64_t endExclusive,
    const std::optional<Predicate> &where) {
  std::lock_guard<std::mutex> lk(mtx_);
  auto it = series_.find(series);
  if (it == series_.end())
    return Result<ResultSet>::err(
        Status::NotFound("Unknown series: " + series));

  const auto &sd = it->second;
  size_t tsIdx = sd.tableSchema.findColumn(sd.schema.timestampColumn());
  if (tsIdx == TableSchema::npos)
    return Result<ResultSet>::err(
        Status::FailedPrecondition("Timestamp column missing from schema"));

  int64_t startSec = toSeconds(startInclusive, sd.schema.granularity());
  int64_t endSec = toSeconds(endExclusive, sd.schema.granularity());
  if (endSec < startSec)
    return Result<ResultSet>::err(
        Status::InvalidArgument("Invalid time range: end < start"));

  std::vector<size_t> projIdx;
  std::vector<std::string> outNames;
  std::vector<ColumnType> outTypes;

  const auto &cols = sd.tableSchema.columns();

  if (columns.empty()) {
    projIdx.resize(cols.size());
    for (size_t i = 0; i < cols.size(); ++i) {
      projIdx[i] = i;
      outNames.push_back(cols[i].name);
      outTypes.push_back(cols[i].type);
    }
  } else {
    projIdx.reserve(columns.size());
    outNames.reserve(columns.size());
    outTypes.reserve(columns.size());
    for (const auto &name : columns) {
      size_t idx = sd.tableSchema.findColumn(name);
      if (idx == TableSchema::npos)
        return projectionUnknownColumn(name);
      projIdx.push_back(idx);
      outNames.push_back(cols[idx].name);
      outTypes.push_back(cols[idx].type);
    }
  }

  ResultSet rs(outNames, outTypes);

  int64_t firstBucket = partitionBucketStartSeconds(startSec, sd.partition);
  int64_t lastBucket = partitionBucketStartSeconds(
      (endSec <= startSec) ? startSec : (endSec - 1), sd.partition);

  const int64_t step =
      (sd.partition == TimePartition::Daily) ? 86400LL : 3600LL;

  for (int64_t b = firstBucket; b <= lastBucket; b += step) {
    auto bit = sd.buckets.find(b);
    if (bit == sd.buckets.end())
      continue;

    for (const auto &r : bit->second) {
      const Value *v = r.values()[tsIdx].get();
      if (!v || v->type() != ValueType::Integer)
        continue;

      int64_t ts = static_cast<const IntegerValue &>(*v).asInt();
      int64_t tsec = toSeconds(ts, sd.schema.granularity());
      if (tsec < startSec || tsec >= endSec)
        continue;

      if (where && !evalPredicate(sd.tableSchema, r, *where))
        continue;

      std::vector<std::unique_ptr<Value>> cells;
      cells.reserve(projIdx.size());
      for (size_t idx : projIdx) {
        const auto &cv = r.values()[idx];
        cells.push_back(cv ? cv->clone() : nullptr);
      }
      rs.addRow(ResultRow(std::move(cells)));
    }
  }

  return Result<ResultSet>::ok(std::move(rs));
}

Result<ResultSet> InMemoryTimeSeriesStorage::aggregate(
    const std::string &series, const std::string &valueColumn,
    TimeAggregation agg, int64_t startInclusive, int64_t endExclusive,
    int64_t bucketWidth, TimeGranularity bucketGranularity,
    const std::optional<Predicate> &where) {
  std::lock_guard<std::mutex> lk(mtx_);
  auto it = series_.find(series);
  if (it == series_.end())
    return Result<ResultSet>::err(
        Status::NotFound("Unknown series: " + series));

  const auto &sd = it->second;
  size_t tsIdx = sd.tableSchema.findColumn(sd.schema.timestampColumn());
  if (tsIdx == TableSchema::npos)
    return Result<ResultSet>::err(
        Status::FailedPrecondition("Timestamp column missing from schema"));

  size_t valIdx = sd.tableSchema.findColumn(valueColumn);
  if (valIdx == TableSchema::npos)
    return Result<ResultSet>::err(
        Status::InvalidArgument("Unknown value column: " + valueColumn));

  int64_t startSec = toSeconds(startInclusive, sd.schema.granularity());
  int64_t endSec = toSeconds(endExclusive, sd.schema.granularity());
  if (endSec < startSec)
    return Result<ResultSet>::err(
        Status::InvalidArgument("Invalid time range: end < start"));

  int64_t widthSec = bucketWidthSeconds(bucketWidth, bucketGranularity);
  if (widthSec <= 0)
    return Result<ResultSet>::err(
        Status::InvalidArgument("bucketWidth must be > 0"));

  struct AggState {
    bool any = false;
    double sum = 0.0;
    double min = std::numeric_limits<double>::infinity();
    double max = -std::numeric_limits<double>::infinity();
    int64_t count = 0;
  };

  std::unordered_map<int64_t, AggState> acc;

  int64_t firstBucket = partitionBucketStartSeconds(startSec, sd.partition);
  int64_t lastBucket = partitionBucketStartSeconds(
      (endSec <= startSec) ? startSec : (endSec - 1), sd.partition);
  const int64_t step =
      (sd.partition == TimePartition::Daily) ? 86400LL : 3600LL;

  for (int64_t b = firstBucket; b <= lastBucket; b += step) {
    auto bit = sd.buckets.find(b);
    if (bit == sd.buckets.end())
      continue;

    for (const auto &r : bit->second) {
      const Value *tv = r.values()[tsIdx].get();
      if (!tv || tv->type() != ValueType::Integer)
        continue;
      int64_t ts = static_cast<const IntegerValue &>(*tv).asInt();
      int64_t tsec = toSeconds(ts, sd.schema.granularity());
      if (tsec < startSec || tsec >= endSec)
        continue;

      if (where && !evalPredicate(sd.tableSchema, r, *where))
        continue;

      int64_t offset = tsec - startSec;
      int64_t bucketStart = startSec + floorDiv(offset, widthSec) * widthSec;

      AggState &st = acc[bucketStart];
      st.any = true;
      st.count += 1;

      if (agg == TimeAggregation::Count)
        continue;

      const Value *vv = r.values()[valIdx].get();
      if (!vv)
        continue;
      if (!(vv->type() == ValueType::Integer || vv->type() == ValueType::Float))
        continue;

      double d = (vv->type() == ValueType::Integer)
                     ? static_cast<double>(vv->asInt())
                     : vv->asFloat();

      st.sum += d;
      if (d < st.min)
        st.min = d;
      if (d > st.max)
        st.max = d;
    }
  }

  std::vector<int64_t> bucketStarts;
  bucketStarts.reserve(acc.size());
  for (const auto &kv : acc)
    bucketStarts.push_back(kv.first);
  std::sort(bucketStarts.begin(), bucketStarts.end());

  std::vector<std::string> colNames = {"bucket_start", "value"};
  std::vector<ColumnType> colTypes = {
      ColumnType::Integer, (agg == TimeAggregation::Count) ? ColumnType::Integer
                                                           : ColumnType::Float};

  ResultSet rs(std::move(colNames), std::move(colTypes));

  for (int64_t bs : bucketStarts) {
    const AggState &st = acc.at(bs);
    std::vector<std::unique_ptr<Value>> row;
    row.reserve(2);
    row.push_back(ValueFactory::createInteger(bs));

    switch (agg) {
    case TimeAggregation::Count:
      row.push_back(ValueFactory::createInteger(st.count));
      break;
    case TimeAggregation::Sum:
      row.push_back(ValueFactory::createFloat(st.sum));
      break;
    case TimeAggregation::Min:
      row.push_back(
          ValueFactory::createFloat(std::isfinite(st.min) ? st.min : 0.0));
      break;
    case TimeAggregation::Max:
      row.push_back(
          ValueFactory::createFloat(std::isfinite(st.max) ? st.max : 0.0));
      break;
    case TimeAggregation::Avg:
      row.push_back(ValueFactory::createFloat(
          st.count > 0 ? (st.sum / static_cast<double>(st.count)) : 0.0));
      break;
    }

    rs.addRow(ResultRow(std::move(row)));
  }

  return Result<ResultSet>::ok(std::move(rs));
}

} // namespace kadedb
