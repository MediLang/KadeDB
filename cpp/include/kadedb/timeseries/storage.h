#pragma once

#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "kadedb/result.h"
#include "kadedb/schema.h"
#include "kadedb/status.h"
#include "kadedb/storage.h" // Predicate

namespace kadedb {

enum class TimePartition { Hourly, Daily };

enum class TimeAggregation { Avg, Min, Max, Sum, Count };

class TimeSeriesStorage {
public:
  virtual ~TimeSeriesStorage() = default;

  virtual Status
  createSeries(const std::string &series, const TimeSeriesSchema &schema,
               TimePartition partition = TimePartition::Hourly) = 0;

  virtual Status dropSeries(const std::string &series) = 0;

  virtual std::vector<std::string> listSeries() const = 0;

  virtual Status append(const std::string &series, const Row &row) = 0;

  virtual Result<ResultSet>
  rangeQuery(const std::string &series, const std::vector<std::string> &columns,
             int64_t startInclusive, int64_t endExclusive,
             const std::optional<Predicate> &where = std::nullopt) = 0;

  virtual Result<ResultSet>
  aggregate(const std::string &series, const std::string &valueColumn,
            TimeAggregation agg, int64_t startInclusive, int64_t endExclusive,
            int64_t bucketWidth, TimeGranularity bucketGranularity,
            const std::optional<Predicate> &where = std::nullopt) = 0;
};

class InMemoryTimeSeriesStorage final : public TimeSeriesStorage {
public:
  InMemoryTimeSeriesStorage() = default;
  ~InMemoryTimeSeriesStorage() override = default;

  Status createSeries(const std::string &series, const TimeSeriesSchema &schema,
                      TimePartition partition) override;
  Status dropSeries(const std::string &series) override;
  std::vector<std::string> listSeries() const override;

  Status append(const std::string &series, const Row &row) override;

  Result<ResultSet> rangeQuery(const std::string &series,
                               const std::vector<std::string> &columns,
                               int64_t startInclusive, int64_t endExclusive,
                               const std::optional<Predicate> &where) override;

  Result<ResultSet> aggregate(const std::string &series,
                              const std::string &valueColumn,
                              TimeAggregation agg, int64_t startInclusive,
                              int64_t endExclusive, int64_t bucketWidth,
                              TimeGranularity bucketGranularity,
                              const std::optional<Predicate> &where) override;

private:
  struct SeriesData {
    TimeSeriesSchema schema;
    TableSchema tableSchema;
    TimePartition partition = TimePartition::Hourly;
    std::unordered_map<int64_t, std::vector<Row>> buckets;
  };

  std::unordered_map<std::string, SeriesData> series_;
  mutable std::mutex mtx_;
};

} // namespace kadedb
