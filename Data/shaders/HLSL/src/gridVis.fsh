#include "globals.shi"
#include "globalIllum.shi"

Texture2D depthMap: register(COLOR_TEX_BP); 

StructuredBuffer<Voxel> fineGridBuffer: register(CUSTOM0_SB_BP);
StructuredBuffer<Voxel> coarseGridBuffer: register(CUSTOM1_SB_BP);

GLOBAL_CAMERA_UB(cameraUB);

cbuffer CustomUB: register(CUSTOM0_UB_BP)
{
	struct
	{
    matrix gridViewProjMatrices[6];
		float4 gridCellSizes; 
	  float4 gridPositions[2];
		float4 snappedGridPositions[2];
		float4 lastSnappedGridPositions[2];
		float4 globalIllumParams;
	}customUB;
};

struct VS_Output
{
	float4 position: SV_POSITION;
  float2 texCoords: TEXCOORD;
};

struct FS_Output
{
  float4 fragColor: SV_TARGET;
};

FS_Output main(VS_Output input) 
{
  FS_Output output;
	float depth = depthMap.Load(int3(input.position.xy, 0)).x;  
	
  // reconstruct world-space position from depth
  float4 projPosition = float4(input.texCoords, depth, 1.0f);
  projPosition.xy = (projPosition.xy*2.0f)-1.0f;
  projPosition.y = -projPosition.y;
	float4 position = mul(cameraUB.invViewProjMatrix, projPosition);
  position.xyz /= position.w;

  float3 color = float3(0.5f, 0.5f, 0.5f);

	// find for the current pixel best voxel representation
	uint gridRes = 0;	
	float3 offset = (position.xyz-customUB.snappedGridPositions[0].xyz)*customUB.gridCellSizes.y;
	float squaredDist = dot(offset, offset);
	if(squaredDist > (15.0f*15.0f))
	{
    offset = (position.xyz-customUB.snappedGridPositions[1].xyz)*customUB.gridCellSizes.w;
    squaredDist = dot(offset, offset);
		gridRes = (squaredDist <= (15.0f*15.0f)) ? 1 : 2;
	}

	// if voxel could be retrieved, get color
	if(gridRes < 2)
	{
		// get index of current voxel
		offset = round(offset);
		int3 voxelPos = int3(16, 16, 16)+int3(offset.x, offset.y, offset.z);
		int gridIndex = GetGridIndex(voxelPos);

		// get voxel
		Voxel voxel;
		if(gridRes == 0)
			voxel = fineGridBuffer[gridIndex];
		else
			voxel = coarseGridBuffer[gridIndex];

		// decode color
	  color = DecodeColor(voxel.colorOcclusionMask);
	}

	output.fragColor = float4(color, 1.0f);

	return output;
}