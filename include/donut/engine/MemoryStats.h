/*
* Copyright (c) 2025, NVIDIA CORPORATION. All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
* DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <nvrhi/nvrhi.h>

#include <cstdint>

namespace donut::engine
{
    class Scene;

    struct GpuMemoryUsage
    {
        uint64_t CapacityBytes = 0;
        uint64_t AllocationBytes = 0;
        uint32_t ResourceCount = 0;
        uint32_t UnknownAllocationCount = 0;

        [[nodiscard]] bool HasKnownAllocationBytes() const { return UnknownAllocationCount == 0; }
    };

    struct SceneGeometryMemoryStats
    {
        GpuMemoryUsage VertexBuffers;
        GpuMemoryUsage IndexBuffers;
    };

    struct TopLevelAccelStructPrebuildStats
    {
        uint64_t ResultBytes = 0;
        uint64_t ScratchBytes = 0;
        uint64_t UpdateScratchBytes = 0;
        bool Available = false;
    };

    // Returns the NVRHI allocation size for buffers when the backend exposes it. Capacity remains
    // available from IBuffer::getDesc().byteSize even when this returns false.
    bool TryGetBufferAllocationBytes(nvrhi::IDevice* device, nvrhi::IBuffer* buffer, uint64_t& outBytes);
    bool TryGetAccelStructAllocationBytes(nvrhi::IDevice* device, nvrhi::rt::IAccelStruct* accelStruct, uint64_t& outBytes);

    // Returns the allocation size for resources that expose a native buffer-like object but do not have
    // a dedicated NVRHI memory-requirements query, such as opacity micromap arrays on D3D12.
    bool TryGetNativeResourceAllocationBytes(nvrhi::IDevice* device, nvrhi::IResource* resource, uint64_t& outBytes);

    SceneGeometryMemoryStats GetSceneGeometryMemoryStats(nvrhi::IDevice* device, const Scene& scene);

    // Queries the build requirements for a TLAS with the supplied instance count. Scratch is the exact
    // build request, not a resident allocation: NVRHI suballocates it from an internal shared pool.
    TopLevelAccelStructPrebuildStats QueryTopLevelAccelStructPrebuildStats(
        nvrhi::IDevice* device,
        const nvrhi::rt::AccelStructDesc& tlasDesc,
        uint32_t instanceCount);
}
