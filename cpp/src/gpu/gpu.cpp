#include "kadedb/gpu.h"

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

  // CPU fallback for now.
  out.reserve(spec.count / 10);
  for (size_t i = 0; i < spec.count; ++i) {
    if (evalInt64(spec.column[i], spec.op, spec.rhs)) {
      out.push_back(i);
    }
  }
  return out;
}

} // namespace kadedb
