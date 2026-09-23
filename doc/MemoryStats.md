# Scene geometry memory statistics

`Scene::getGeometryMemoryStats()` returns current vertex/index buffer totals, deduplicating
shared buffer handles within each category. A handle used in both categories counts once
in each, so adding the categories does not give unique physical memory usage. It keeps
descriptor `capacityBytes` separate from backing-buffer `allocationBytes`, which sums only
successful queries. Check `hasKnownAllocationBytes()` before displaying a complete allocation
total: unavailable resources increment `unknownAllocationCount`, not a known zero.
`resourceCount` includes both known and unknown non-null buffers. Missing graphs, meshes,
buffer groups, and buffer handles contribute nothing; an empty category is known zero.
Re-query after scene resource replacement or removal; the statistics do not retain resources.
The free function `getSceneGeometryMemoryStats(device, scene)` provides the same aggregation
with an explicit device; a null device preserves capacity and counts but makes allocation
sizes unavailable for every counted buffer.

`tryGetResourceAllocationBytes` uses NVRHI's optional `queryResourceMemoryRequirements`
and leaves `outBytes` unchanged on failure, including a null device or resource.
See [NVRHI's optional memory queries](../nvrhi/doc/memory-queries.md) for resource/device
requirements, backend support and exceptions, allocation interpretation, and the native
`IDevice::queryTopLevelAccelStructPrebuildInfo` API. Application-specific pooled BLAS/OMM
attribution and statistics UI remain the consuming application's responsibility.

The headless [memory statistics regression](../tests/src/memory_stats.cpp) covers the public
helper and scene aggregation. Its build conditions and CTest registration are defined in
[tests/test-engine.cmake](../tests/test-engine.cmake).
