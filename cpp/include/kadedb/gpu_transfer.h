#pragma once

#include <cstddef>
#include <cstdint>

#include "kadedb/status.h"

namespace kadedb {

using GpuStreamHandle = void *;

bool gpuTransferAvailable();

Status gpuStreamCreate(GpuStreamHandle &out);
Status gpuStreamDestroy(GpuStreamHandle stream);

Status gpuMallocPinned(void *&out, size_t bytes);
Status gpuFreePinned(void *p);

Status gpuMallocDevice(void *&out, size_t bytes);
Status gpuFreeDevice(void *p);

Status gpuMemcpyHtoDAsync(void *dstDevice, const void *srcHost, size_t bytes,
                          GpuStreamHandle stream = nullptr);
Status gpuMemcpyDtoHAsync(void *dstHost, const void *srcDevice, size_t bytes,
                          GpuStreamHandle stream = nullptr);
Status gpuStreamSynchronize(GpuStreamHandle stream = nullptr);

} // namespace kadedb
