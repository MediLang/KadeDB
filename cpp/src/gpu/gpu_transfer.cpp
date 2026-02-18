#include "kadedb/gpu_transfer.h"

#include <string>

namespace kadedb {

bool gpuTransferAvailable() {
#if defined(KADEDB_HAVE_CUDA)
  return true;
#else
  return false;
#endif
}

#if defined(KADEDB_HAVE_CUDA)
#include <cuda_runtime_api.h>

static Status cudaToStatus(cudaError_t e, const char *what) {
  if (e == cudaSuccess)
    return Status::OK();
  std::string msg = std::string(what) + ": " + cudaGetErrorString(e);
  return Status::Internal(msg);
}

Status gpuStreamCreate(GpuStreamHandle &out) {
  cudaStream_t s{};
  auto e = cudaStreamCreateWithFlags(&s, cudaStreamNonBlocking);
  if (e != cudaSuccess)
    return cudaToStatus(e, "cudaStreamCreateWithFlags");
  out = reinterpret_cast<GpuStreamHandle>(s);
  return Status::OK();
}

Status gpuStreamDestroy(GpuStreamHandle stream) {
  if (!stream)
    return Status::OK();
  cudaStream_t s = reinterpret_cast<cudaStream_t>(stream);
  return cudaToStatus(cudaStreamDestroy(s), "cudaStreamDestroy");
}

Status gpuMallocPinned(void *&out, size_t bytes) {
  out = nullptr;
  if (bytes == 0)
    return Status::InvalidArgument("bytes must be > 0");
  void *p = nullptr;
  auto e = cudaMallocHost(&p, bytes);
  if (e != cudaSuccess)
    return cudaToStatus(e, "cudaMallocHost");
  out = p;
  return Status::OK();
}

Status gpuFreePinned(void *p) {
  if (!p)
    return Status::OK();
  return cudaToStatus(cudaFreeHost(p), "cudaFreeHost");
}

Status gpuMallocDevice(void *&out, size_t bytes) {
  out = nullptr;
  if (bytes == 0)
    return Status::InvalidArgument("bytes must be > 0");
  void *p = nullptr;
  auto e = cudaMalloc(&p, bytes);
  if (e != cudaSuccess)
    return cudaToStatus(e, "cudaMalloc");
  out = p;
  return Status::OK();
}

Status gpuFreeDevice(void *p) {
  if (!p)
    return Status::OK();
  return cudaToStatus(cudaFree(p), "cudaFree");
}

Status gpuMemcpyHtoDAsync(void *dstDevice, const void *srcHost, size_t bytes,
                          GpuStreamHandle stream) {
  if (!dstDevice || !srcHost)
    return Status::InvalidArgument("null pointer");
  cudaStream_t s = reinterpret_cast<cudaStream_t>(stream);
  return cudaToStatus(
      cudaMemcpyAsync(dstDevice, srcHost, bytes, cudaMemcpyHostToDevice, s),
      "cudaMemcpyAsync(HtoD)");
}

Status gpuMemcpyDtoHAsync(void *dstHost, const void *srcDevice, size_t bytes,
                          GpuStreamHandle stream) {
  if (!dstHost || !srcDevice)
    return Status::InvalidArgument("null pointer");
  cudaStream_t s = reinterpret_cast<cudaStream_t>(stream);
  return cudaToStatus(
      cudaMemcpyAsync(dstHost, srcDevice, bytes, cudaMemcpyDeviceToHost, s),
      "cudaMemcpyAsync(DtoH)");
}

Status gpuStreamSynchronize(GpuStreamHandle stream) {
  // nullptr maps to the default stream (stream 0) by CUDA convention.
  cudaStream_t s = reinterpret_cast<cudaStream_t>(stream);
  return cudaToStatus(cudaStreamSynchronize(s), "cudaStreamSynchronize");
}

#else

static Status notAvailable() {
  return Status::FailedPrecondition("CUDA transfer utilities not available");
}

Status gpuStreamCreate(GpuStreamHandle &out) {
  out = nullptr;
  return notAvailable();
}

Status gpuStreamDestroy(GpuStreamHandle) { return Status::OK(); }

Status gpuMallocPinned(void *&out, size_t) {
  out = nullptr;
  return notAvailable();
}

Status gpuFreePinned(void *) { return Status::OK(); }

Status gpuMallocDevice(void *&out, size_t) {
  out = nullptr;
  return notAvailable();
}

Status gpuFreeDevice(void *) { return Status::OK(); }

Status gpuMemcpyHtoDAsync(void *, const void *, size_t, GpuStreamHandle) {
  return notAvailable();
}

Status gpuMemcpyDtoHAsync(void *, const void *, size_t, GpuStreamHandle) {
  return notAvailable();
}

Status gpuStreamSynchronize(GpuStreamHandle) { return Status::OK(); }

#endif

} // namespace kadedb
