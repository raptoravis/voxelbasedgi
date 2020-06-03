#include "globalIllum.shi"
 
RWStructuredBuffer<Voxel> fineGridBuffer: register(u0);
RWStructuredBuffer<Voxel> coarseGridBuffer: register(u1);

[numthreads(8, 8, 8)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
	int3 voxelPos = dispatchThreadID.xyz;
	int gridIndex = GetGridIndex(voxelPos);

	Voxel gridElement;
	gridElement.colorOcclusionMask = 0;	
	gridElement.normalMasks = uint4(0, 0, 0, 0);

	// clear fine and coarse resolution voxel-grid
  fineGridBuffer[gridIndex] = gridElement;
	coarseGridBuffer[gridIndex] = gridElement;
}
