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

#include <donut/engine/MemoryStats.h>
#include <donut/engine/Scene.h>
#include <donut/engine/SceneTypes.h>

#include <unordered_set>

#if DONUT_WITH_DX12
#include <d3d12.h>
#include <wrl/client.h>
#endif

namespace donut::engine
{
namespace
{
    void AddBuffer(nvrhi::IDevice* device, nvrhi::IBuffer* buffer, GpuMemoryUsage& stats,
        std::unordered_set<nvrhi::IBuffer*>& visited)
    {
        if (buffer == nullptr || !visited.insert(buffer).second)
            return;

        stats.ResourceCount++;
        stats.CapacityBytes += buffer->getDesc().byteSize;

        uint64_t allocationBytes = 0;
        if (TryGetBufferAllocationBytes(device, buffer, allocationBytes))
            stats.AllocationBytes += allocationBytes;
        else
            stats.UnknownAllocationCount++;
    }
}

bool TryGetBufferAllocationBytes(nvrhi::IDevice* device, nvrhi::IBuffer* buffer, uint64_t& outBytes)
{
    if (device == nullptr || buffer == nullptr)
        return false;

    const nvrhi::MemoryRequirements requirements = device->getBufferMemoryRequirements(buffer);
    if (requirements.size == 0)
        return false;

    outBytes = requirements.size;
    return true;
}

bool TryGetAccelStructAllocationBytes(nvrhi::IDevice* device, nvrhi::rt::IAccelStruct* accelStruct, uint64_t& outBytes)
{
    if (device == nullptr || accelStruct == nullptr)
        return false;

    const nvrhi::MemoryRequirements requirements = device->getAccelStructMemoryRequirements(accelStruct);
    if (requirements.size == 0)
        return false;

    outBytes = requirements.size;
    return true;
}

bool TryGetNativeResourceAllocationBytes(nvrhi::IDevice* device, nvrhi::IResource* resource, uint64_t& outBytes)
{
    if (device == nullptr || resource == nullptr)
        return false;

#if DONUT_WITH_DX12
    if (device->getGraphicsAPI() != nvrhi::GraphicsAPI::D3D12)
        return false;

    ID3D12Resource* d3dResource = resource->getNativeObject(nvrhi::ObjectTypes::D3D12_Resource);
    if (d3dResource == nullptr)
        return false;

    ID3D12Device* d3dDevice = device->getNativeObject(nvrhi::ObjectTypes::D3D12_Device);
    if (d3dDevice == nullptr)
        return false;

    const D3D12_RESOURCE_DESC desc = d3dResource->GetDesc();
    const D3D12_RESOURCE_ALLOCATION_INFO info = d3dDevice->GetResourceAllocationInfo(0, 1, &desc);
    if (info.SizeInBytes == UINT64_MAX)
        return false;

    outBytes = info.SizeInBytes;
    return true;
#else
    (void)outBytes;
    return false;
#endif
}

SceneGeometryMemoryStats GetSceneGeometryMemoryStats(nvrhi::IDevice* device, const Scene& scene)
{
    SceneGeometryMemoryStats stats;

    const std::shared_ptr<SceneGraph> sceneGraph = scene.GetSceneGraph();
    if (sceneGraph == nullptr)
        return stats;

    std::unordered_set<nvrhi::IBuffer*> vertexBuffers;
    std::unordered_set<nvrhi::IBuffer*> indexBuffers;

    for (const std::shared_ptr<MeshInfo>& mesh : sceneGraph->GetMeshes())
    {
        if (mesh == nullptr || mesh->buffers == nullptr)
            continue;

        AddBuffer(device, mesh->buffers->vertexBuffer, stats.VertexBuffers, vertexBuffers);
        AddBuffer(device, mesh->buffers->indexBuffer, stats.IndexBuffers, indexBuffers);
    }

    return stats;
}

TopLevelAccelStructPrebuildStats QueryTopLevelAccelStructPrebuildStats(
    nvrhi::IDevice* device,
    const nvrhi::rt::AccelStructDesc& tlasDesc,
    uint32_t instanceCount)
{
    TopLevelAccelStructPrebuildStats stats;

#if DONUT_WITH_DX12
    if (device == nullptr || device->getGraphicsAPI() != nvrhi::GraphicsAPI::D3D12 || !tlasDesc.isTopLevel)
        return stats;

    ID3D12Device* d3dDevice = device->getNativeObject(nvrhi::ObjectTypes::D3D12_Device);
    if (d3dDevice == nullptr)
        return stats;

    Microsoft::WRL::ComPtr<ID3D12Device5> d3dDevice5;
    if (FAILED(d3dDevice->QueryInterface(IID_PPV_ARGS(&d3dDevice5))))
        return stats;

    nvrhi::rt::AccelStructBuildFlags buildFlags = tlasDesc.buildFlags & ~nvrhi::rt::AccelStructBuildFlags::AllowEmptyInstances;

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
    inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    inputs.InstanceDescs = 0;
    inputs.NumDescs = instanceCount;
    inputs.Flags = (D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS)buildFlags;

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info = {};
    d3dDevice5->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);

    stats.ResultBytes = info.ResultDataMaxSizeInBytes;
    stats.ScratchBytes = info.ScratchDataSizeInBytes;
    stats.UpdateScratchBytes = info.UpdateScratchDataSizeInBytes;
    stats.Available = true;
#else
    (void)device; (void)tlasDesc; (void)instanceCount;
#endif

    return stats;
}
}
