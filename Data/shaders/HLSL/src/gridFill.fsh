#include "globals.shi"
#include "globalIllum.shi"

Texture2D colorMap: register(COLOR_TEX_BP); 
SamplerState colorMapSampler: register(COLOR_SAM_BP);

RWStructuredBuffer<Voxel> gridBuffer: register(u1);

cbuffer CustomUB: register(CUSTOM1_UB_BP)
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

struct GS_Output
{
	float4 position: SV_POSITION;
	float3 positionWS: POS_WS;
  float2 texCoords: TEXCOORD;
#ifndef REFLECTION_GRID
	float3 normal: NORMAL;
#endif
};

// normalized directions of 4 faces of a regular tetrahedron
static const float3 faceVectors[4] =
{   
  float3(0.0f, -0.57735026f, 0.81649661f),
  float3(0.0f, -0.57735026f, -0.81649661f),
  float3(-0.81649661f, 0.57735026f, 0.0f),
	float3(0.81649661f, 0.57735026f, 0.0f) 
};

uint GetNormalIndex(in float3 normal, out float dotProduct)
{
	float4x3 faceMatrix;
	faceMatrix[0] = faceVectors[0];
	faceMatrix[1] = faceVectors[1];
	faceMatrix[2] = faceVectors[2];
	faceMatrix[3] = faceVectors[3];   
	float4 dotProducts = mul(faceMatrix, normal);
	float maximum = max(max(dotProducts.x, dotProducts.y), max(dotProducts.z, dotProducts.w));
	uint index;
	if(maximum == dotProducts.x)
		index = 0;
  else if(maximum == dotProducts.y)
		index = 1;
	else if(maximum == dotProducts.z)
		index = 2;
	else 
		index = 3;

	dotProduct = dotProducts[index];
	return index;
}

// Instead of outputting the rasterized information into the bound render-target, it will be
// written into a 3D structured buffer. In this way dynamically a voxel-grid can be generated.
// Among the variety of DX11 buffers the RWStructuredBuffer has been chosen, because this is 
// the only possibility to write out atomically information into multiple variables without
// having to use for each variable a separate buffer.
void main(GS_Output input) 
{
  // get surface color 
	float3 base = colorMap.Sample(colorMapSampler, input.texCoords).rgb; 
  
  // encode color in linear space into unsigned integer
  float3 baseLinear = lerp(base/12.92f, pow((base+0.055f)/1.055f, 2.4f), base>0.04045f);
  uint colorOcclusionMask = EncodeColor(baseLinear);

  // Since voxels are a simplified representation of the actual scene, high frequency information
	// gets lost. In order to amplify color bleeding in the final global illumination output, colors
	// with high difference in their color channels (called here contrast) are preferred. By writing
	// the contrast value (0-255) in the highest 8 bit of the color-mask, automatically colors with 
	// high contrast will dominate, since we write the results with an InterlockedMax into the voxel-
	// grids. The contrast value is calculated in SRGB space.
  float contrast = length(base.rrg-base.gbb)/(sqrt(2.0f)+base.r+base.g+base.b);
  uint iContrast = uint(contrast*127.0);
  colorOcclusionMask |= iContrast<<24u;

  // encode occlusion into highest bit
  colorOcclusionMask |= 1<<31u;

	// encode normal into unsigned integer
	float3 normal = normalize(input.normal);
	uint normalMask = EncodeNormal(normal);

	// Normals values also have to be carefully written into the voxels, since for example thin geometry 
	// can have opposite normals in one single voxel. Therefore it is determined, to which face of a 
	// tetrahedron the current normal is closest to. By writing the corresponding dotProduct value in the
	// highest 5 bit of the normal-mask, automatically the closest normal to the determined tetrahedron 
	// face will be selected, since we write the results with an InterlockeMax into the voxel-grids. 
	// According to the retrieved tetrahedron face the normals are written into the corresponding normal 
	// channel of the voxel. Later on, when the voxels are illuminated, the closest normal to the light-
	// vector is chosen, so that the best illumination can be obtained.
	float dotProduct;
  uint normalIndex = GetNormalIndex(normal,dotProduct);
	uint iDotProduct = uint(saturate(dotProduct)*31.0f);
	normalMask |= iDotProduct<<27u; 
	
	// get offset into the voxel-grid
#ifdef FINE_GRID
  float3 offset = (input.positionWS-customUB.snappedGridPositions[0].xyz)*customUB.gridCellSizes.y;
#else
	float3 offset = (input.positionWS-customUB.snappedGridPositions[1].xyz)*customUB.gridCellSizes.w;
#endif 
	offset = round(offset);

	// get position in the voxel-grid
	int3 voxelPos = int3(16, 16, 16)+int3(offset.x, offset.y, offset.z);
	
	// To avoid raise conditions between multiple threads, that write into the same location, atomic
	// functions have to be used. Only output voxels that are inside the boundaries of the grid.
	if((voxelPos.x > -1) && (voxelPos.x < 32) && (voxelPos.y > -1) && (voxelPos.y < 32) && (voxelPos.z > -1) && (voxelPos.z < 32))
	{
		// get index into the voxel-grid
		int gridIndex = GetGridIndex(voxelPos);

	  // output color/ occlusion
		InterlockedMax(gridBuffer[gridIndex].colorOcclusionMask, colorOcclusionMask);

		// output normal according to normal index
    InterlockedMax(gridBuffer[gridIndex].normalMasks[normalIndex], normalMask);
	}
}


