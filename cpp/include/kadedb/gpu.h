#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace kadedb {

struct GpuStatus {
  bool available = false;
  std::string message;
};

GpuStatus gpuStatus();

struct GpuScanSpec {
  const int64_t *column = nullptr;
  size_t count = 0;
  int64_t rhs = 0;

  enum class Op { Eq, Ne, Lt, Le, Gt, Ge };
  Op op = Op::Eq;
};

// Returns indices of rows that match the predicate.
// This is a placeholder API: initial implementation uses CPU fallback.
std::vector<size_t> gpuScanFilterInt64(const GpuScanSpec &spec);

struct GpuTimeBucketAggSpec {
  const int64_t *timestamps = nullptr;
  const double *values = nullptr;
  size_t count = 0;

  int64_t startInclusive = 0;
  int64_t endExclusive = 0;
  int64_t bucketWidth = 1;
};

struct GpuTimeBucketAggResult {
  std::vector<int64_t> bucketStart;
  std::vector<double> sum;
  std::vector<int64_t> count;
};

GpuTimeBucketAggResult gpuTimeBucketSumCount(const GpuTimeBucketAggSpec &spec);

} // namespace kadedb
