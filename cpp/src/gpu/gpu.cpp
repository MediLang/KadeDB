#include "kadedb/gpu.h"

#include <algorithm>
#include <atomic>
#include <thread>
#include <unordered_map>
#include <vector>

namespace kadedb {

GpuStatus gpuStatus() {
  GpuStatus st;
#if defined(KADEDB_HAVE_CUDA)
  st.available = true;
  st.message = "CUDA enabled";
#else
  st.available = false;
  st.message = "CUDA not enabled";
#endif
  return st;
}

static bool evalInt64(int64_t lhs, GpuScanSpec::Op op, int64_t rhs) {
  switch (op) {
  case GpuScanSpec::Op::Eq:
    return lhs == rhs;
  case GpuScanSpec::Op::Ne:
    return lhs != rhs;
  case GpuScanSpec::Op::Lt:
    return lhs < rhs;
  case GpuScanSpec::Op::Le:
    return lhs <= rhs;
  case GpuScanSpec::Op::Gt:
    return lhs > rhs;
  case GpuScanSpec::Op::Ge:
    return lhs >= rhs;
  }
  return false;
}

std::vector<size_t> gpuScanFilterInt64(const GpuScanSpec &spec) {
  std::vector<size_t> out;
  if (!spec.column || spec.count == 0)
    return out;

  const unsigned hw = std::max(1u, std::thread::hardware_concurrency());
  const size_t threads = std::min<size_t>(hw, spec.count);
  if (threads <= 1) {
    out.reserve(spec.count / 10);
    for (size_t i = 0; i < spec.count; ++i) {
      if (evalInt64(spec.column[i], spec.op, spec.rhs))
        out.push_back(i);
    }
    return out;
  }

  std::vector<std::vector<size_t>> locals(threads);
  std::vector<std::thread> pool;
  pool.reserve(threads);

  for (size_t t = 0; t < threads; ++t) {
    const size_t start = (spec.count * t) / threads;
    const size_t end = (spec.count * (t + 1)) / threads;
    pool.emplace_back([&, t, start, end]() {
      auto &buf = locals[t];
      buf.reserve((end - start) / 10);
      for (size_t i = start; i < end; ++i) {
        if (evalInt64(spec.column[i], spec.op, spec.rhs))
          buf.push_back(i);
      }
    });
  }

  size_t total = 0;
  for (auto &th : pool)
    th.join();
  for (const auto &v : locals)
    total += v.size();
  out.reserve(total);
  for (auto &v : locals) {
    out.insert(out.end(), v.begin(), v.end());
  }
  return out;
}

GpuTimeBucketAggResult gpuTimeBucketSumCount(const GpuTimeBucketAggSpec &spec) {
  GpuTimeBucketAggResult out;
  if (!spec.timestamps || !spec.values || spec.count == 0)
    return out;
  if (spec.bucketWidth <= 0)
    return out;
  if (spec.endExclusive <= spec.startInclusive)
    return out;

  const unsigned hw = std::max(1u, std::thread::hardware_concurrency());
  const size_t threads = std::min<size_t>(hw, spec.count);

  struct Agg {
    double sum = 0.0;
    int64_t count = 0;
  };

  std::vector<std::unordered_map<int64_t, Agg>> locals(threads);
  std::vector<std::thread> pool;
  pool.reserve(threads);

  for (size_t t = 0; t < threads; ++t) {
    const size_t start = (spec.count * t) / threads;
    const size_t end = (spec.count * (t + 1)) / threads;
    pool.emplace_back([&, t, start, end]() {
      auto &map = locals[t];
      for (size_t i = start; i < end; ++i) {
        const int64_t ts = spec.timestamps[i];
        if (ts < spec.startInclusive || ts >= spec.endExclusive)
          continue;
        const int64_t offset = ts - spec.startInclusive;
        const int64_t b = spec.startInclusive +
                          (offset / spec.bucketWidth) * spec.bucketWidth;
        Agg &a = map[b];
        a.sum += spec.values[i];
        a.count += 1;
      }
    });
  }

  for (auto &th : pool)
    th.join();

  std::unordered_map<int64_t, Agg> merged;
  for (auto &m : locals) {
    for (const auto &kv : m) {
      Agg &dst = merged[kv.first];
      dst.sum += kv.second.sum;
      dst.count += kv.second.count;
    }
  }

  std::vector<int64_t> keys;
  keys.reserve(merged.size());
  for (const auto &kv : merged)
    keys.push_back(kv.first);
  std::sort(keys.begin(), keys.end());

  out.bucketStart = keys;
  out.sum.reserve(keys.size());
  out.count.reserve(keys.size());
  for (int64_t k : keys) {
    const Agg &a = merged.at(k);
    out.sum.push_back(a.sum);
    out.count.push_back(a.count);
  }

  return out;
}

} // namespace kadedb
