#include "globals.shi"
#include "globalIllum.shi"

#if defined(TEXTURE)
Texture2D albedoGlossMap: register(COLOR_TEX_BP); 
#endif
Texture2D normalMap: register(NORMAL_TEX_BP); 
Texture2D depthMap: register(SPECULAR_TEX_BP); 

// FINE_GRID
Texture2DArray customMap0: register(CUSTOM0_TEX_BP); // redSHCoeffs 
SamplerState customMap0Sampler: register(CUSTOM0_SAM_BP);
Texture2DArray customMap1: register(CUSTOM1_TEX_BP); // greenSHCoeffs 
Texture2DArray customMap2: register(CUSTOM2_TEX_BP); // blueSHCoeffs

// COARSE_GRID
Texture2DArray customMap3: register(CUSTOM3_TEX_BP); // redSHCoeffs 
Texture2DArray customMap4: register(CUSTOM4_TEX_BP); // greenSHCoeffs 
Texture2DArray customMap5: register(CUSTOM5_TEX_BP); // blueSHCoeffs

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
		float4 globalIllumParams; // x = flux amplifier, y = occlusion amplifier, z = diffuse GI-contribution power
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

// After calculating the texCoords into the 2D texture arrays, the SH-coeffs are trilinearly sampled and
// finally a SH-lighting is done to generate the diffuse global illumination.
float3 GetDiffuseIllum(in float3 offset, in float4 surfaceNormalLobe, in Texture2DArray redSHCoeffsMap,
											 in Texture2DArray greenSHCoeffsMap, in Texture2DArray blueSHCoeffsMap)
{
	// get texCoords into 2D texture arrays
  float3 texCoords = float3(16.5f, 16.5f, 16.0f)+offset;
	texCoords.xy /= 32.0f;

	// Since hardware already does the filtering in each 2D texture slice, manually only the filtering into
	// the third dimension has to be done.
  int lowZ = floor(texCoords.z);
  int highZ = min(lowZ+1, 32-1);
  float highZWeight = texCoords.z-lowZ;
  float lowZWeight = 1.0f-highZWeight;
  float3 texCoordsLow = float3(texCoords.x, texCoords.y, lowZ);
  float3 texCoordsHigh = float3(texCoords.x, texCoords.y, highZ);

	// sample red/ green/ blue SH-coeffs trilinearly from the 2D texture arrays
  float4 redSHCoeffs = lowZWeight*redSHCoeffsMap.Sample(customMap0Sampler, texCoordsLow) + highZWeight*redSHCoeffsMap.Sample(customMap0Sampler, texCoordsHigh);
  float4 greenSHCoeffs = lowZWeight*greenSHCoeffsMap.Sample(customMap0Sampler, texCoordsLow) + highZWeight*greenSHCoeffsMap.Sample(customMap0Sampler, texCoordsHigh);
  float4 blueSHCoeffs = lowZWeight*blueSHCoeffsMap.Sample(customMap0Sampler, texCoordsLow) + highZWeight*blueSHCoeffsMap.Sample(customMap0Sampler, texCoordsHigh);

	// Do diffuse SH-lighting by simply calculating the dot-product between the SH-coeffs from the virtual 
	// point lights and the surface SH-coeffs.
	float3 vDiffuse;
	vDiffuse.r = dot(redSHCoeffs, surfaceNormalLobe);
	vDiffuse.g = dot(greenSHCoeffs, surfaceNormalLobe);
	vDiffuse.b = dot(blueSHCoeffs, surfaceNormalLobe);

  return vDiffuse;
}

FS_Output main(VS_Output input) 
{
  FS_Output output;
  int3 texCoords = int3(input.position.xy, 0);
	float3 normal = normalMap.Load(texCoords).xyz*2.0f-1.0f; 
  float depth = depthMap.Load(texCoords).x; 

  // reconstruct world-space position from depth
  float4 projPosition = float4(input.texCoords, depth, 1.0f);
  projPosition.xy = (projPosition.xy*2.0f)-1.0f;
  projPosition.y = -projPosition.y;
	float4 position = mul(cameraUB.invViewProjMatrix, projPosition);
  position.xyz /= position.w;

#if defined(TEXTURE)
	float4 albedoGloss = albedoGlossMap.Load(texCoords); 
#endif

	// get surface SH-coeffs
  float4 surfaceNormalLobe = ClampedCosineCoeffs(normal);

	// get offset into fine resolution grid
	float3 offset = (position.xyz-customUB.snappedGridPositions[0].xyz)*customUB.gridCellSizes.y;

	// The distance for lerping between fine and coarse resolution grid has to be calculated with
	// the unsnapped grid-center, in order to avoid artifacts in the lerp area.
	float3 lerpOffset = (position.xyz-customUB.gridPositions[0].xyz)*customUB.gridCellSizes.y;
	float lerpDist = length(lerpOffset);

	// get diffuse global illumination from fine resolution grid
	float3 fineDiffuseIllum = GetDiffuseIllum(offset, surfaceNormalLobe, customMap0, customMap1, customMap2);
	
	// get offset into coarse resolution grid
	offset = (position.xyz-customUB.snappedGridPositions[1].xyz)*customUB.gridCellSizes.w;	

	// get diffuse global illumination from coarse resolution grid
	float3 coarseDiffuseIllum = GetDiffuseIllum(offset, surfaceNormalLobe, customMap3, customMap4, customMap5);
	
	// lerp between results from both grids
	float factor = saturate((lerpDist-12.0f)*0.25f);
	float3 diffuseIllum = lerp(fineDiffuseIllum, coarseDiffuseIllum, factor);

	diffuseIllum = max(diffuseIllum, float3(0.0f, 0.0f, 0.0f));
	diffuseIllum /= PI;
	diffuseIllum = pow(diffuseIllum, customUB.globalIllumParams.z);

#ifdef TEXTURE
  float3 outputColor = diffuseIllum*albedoGloss.rgb;
#else
  float3 outputColor = diffuseIllum;
#endif

	output.fragColor = float4(outputColor, 1.0f);

	return output;
}