#include "globals.shi"
#include "globalIllum.shi"

// current SH-coeffs
Texture2DArray customMap0: register(CUSTOM0_TEX_BP); // redSHCoeffs
Texture2DArray customMap1: register(CUSTOM1_TEX_BP); // greenSHCoeffs
Texture2DArray customMap2: register(CUSTOM2_TEX_BP); // blueSHCoeffs

// last SH-Coeffs
Texture2DArray customMap3: register(CUSTOM3_TEX_BP); // redSHCoeffs
Texture2DArray customMap4: register(CUSTOM4_TEX_BP); // greenSHCoeffs
Texture2DArray customMap5: register(CUSTOM5_TEX_BP); // blueSHCoeffs

RWTexture2DArray<float4> redOutputTexture: register(u0); // redSHCoeffs
RWTexture2DArray<float4> greenOutputTexture: register(u1); // greenSHCoeffs
RWTexture2DArray<float4> blueOutputTexture: register(u2); // blueSHCoeffs

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

#define LERP_FACTOR 0.03f

[numthreads(8, 8, 8)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{ 
	// get position of current cell in grid of current frame
  int3 elementPos = int3(dispatchThreadID.xyz);
   
	// get SH-coeffs of current frame
	float4 currentRedSHCoeffs = customMap0.Load(int4(elementPos, 0));
  float4 currentGreenSHCoeffs = customMap1.Load(int4(elementPos, 0));
	float4 currentBlueSHCoeffs = customMap2.Load(int4(elementPos, 0));

	// initialize SH-coeffs of last frame with values from current frame
  float4 lastRedSHCoeffs = currentRedSHCoeffs;
  float4 lastGreenSHCoeffs = currentGreenSHCoeffs;
	float4 lastBlueSHCoeffs = currentBlueSHCoeffs;

	// get offset into the grid of current frame
	int3 currentOffset = elementPos-int3(16, 16, 16);

	// get world-space position of current cell
#ifdef FINE_GRID
	float3 position = (float3(currentOffset.x, currentOffset.y, currentOffset.z)*customUB.gridCellSizes.x)+customUB.snappedGridPositions[0];
#else
	float3 position = (float3(currentOffset.x, currentOffset.y, currentOffset.z)*customUB.gridCellSizes.z)+customUB.snappedGridPositions[1];
#endif

	// get offset into the grid of last frame
#ifdef FINE_GRID
  float3 lastOffset = (position-customUB.lastSnappedGridPositions[0].xyz)*customUB.gridCellSizes.y;
#else
	float3 lastOffset = (position-customUB.lastSnappedGridPositions[1].xyz)*customUB.gridCellSizes.w;
#endif 
	lastOffset = round(lastOffset);

	// get position of current cell in grid of last frame
	int3 samplePos = int3(16, 16, 16)+int3(lastOffset.x, lastOffset.y, lastOffset.z);

	// overwrite SH-coeffs of last frame if samplePos within bounds
	if((samplePos.x > -1) && (samplePos.x < 32) && (samplePos.y > -1) && (samplePos.y < 32) && (samplePos.z > -1) && (samplePos.z < 32))
	{
    lastRedSHCoeffs = customMap3.Load(int4(samplePos, 0));
    lastGreenSHCoeffs = customMap4.Load(int4(samplePos, 0));
	  lastBlueSHCoeffs = customMap5.Load(int4(samplePos, 0));
	}
	
	// perform linear interpolation between SH-coeffs of current and last frame
	// (w-component of lastSnappedGridPositions contains frame-interval)
	float factor = customUB.lastSnappedGridPositions[0].w*0.0625f*LERP_FACTOR; 
	float4 finalRedSHCoeffs = lerp(lastRedSHCoeffs, currentRedSHCoeffs, factor);
	float4 finalGreenSHCoeffs = lerp(lastGreenSHCoeffs, currentGreenSHCoeffs, factor);
	float4 finalBlueSHCoeffs = lerp(lastBlueSHCoeffs, currentBlueSHCoeffs, factor);

	// write out interpolated SH-coeffs
	redOutputTexture[elementPos] = finalRedSHCoeffs;
	greenOutputTexture[elementPos] = finalGreenSHCoeffs;
	blueOutputTexture[elementPos] = finalBlueSHCoeffs;
}
