# Scene geometry memory statistics

`Scene::getGeometryMemoryStats()` returns current vertex/index buffer totals, deduplicating
shared buffer handles within each category. It keeps descriptor `capacityBytes` separate
from backing-buffer `allocationBytes`. Check `hasKnownAllocationBytes()` before displaying
an allocation total: unavailable resources increment `unknownAllocationCount`, not a known zero.
Re-query after scene resource replacement or removal; the statistics do not retain resources.

`tryGetResourceAllocationBytes` uses NVRHI's optional `queryResourceMemoryRequirements`.
D3D11 reports unavailable safely; D3D12 and Vulkan report requirements for exposed backing
buffers, including acceleration structures and opacity micromaps. These numbers are not
residency, driver overhead, or unique shared-heap allocation. Pool reservations and aliases
between categories need attribution by their owner in the consuming application.

TLAS prebuild queries are exposed directly by
`IDevice::queryTopLevelAccelStructPrebuildInfo` in NVRHI, rather than Donut. Scratch build
requests are not resident scratch-pool capacity. Unsupported backends return `false` and
leave the output unchanged; see NVRHI's `doc/memory-queries.md` for backend support and tests.
