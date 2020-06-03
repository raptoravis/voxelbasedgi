#include "globals.shi"

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

struct VS_Output
{
	float3 position: POSITION;
  float2 texCoords: TEXCOORD;
	float3 normal: NORMAL;
};

struct GS_Output
{
	float4 position: SV_POSITION;
	float3 positionWS: POS_WS;
  float2 texCoords: TEXCOORD;
	float3 normal: NORMAL;
};

static const float3 viewDirections[3] =
{ 
  float3(0.0f, 0.0f, -1.0f), // back to front
  float3(-1.0f, 0.0f, 0.0f), // right to left
  float3(0.0f, -1.0f, 0.0f)  // top to down 
}; 

uint GetViewIndex(in float3 normal)
{
  float3x3 directionMatrix;
  directionMatrix[0] = -viewDirections[0];
  directionMatrix[1] = -viewDirections[1];
  directionMatrix[2] = -viewDirections[2];
  float3 dotProducts = abs(mul(directionMatrix, normal));
	float maximum = max(max(dotProducts.x, dotProducts.y), dotProducts.z);
	uint index;
	if(maximum == dotProducts.x)
		index = 0;
  else if(maximum == dotProducts.y)
		index = 1;
	else 
		index = 2;
	return index;
}

[maxvertexcount(3)]
void main(triangle VS_Output input[3], inout TriangleStream<GS_Output> outputStream)
{
	float3 faceNormal = normalize(input[0].normal+input[1].normal+input[2].normal);

	// Get view, at which the current triangle is most visible, in order to achieve highest
	// possible rasterization of the primitive.
  uint viewIndex = GetViewIndex(faceNormal);

	GS_Output output[3];
	[unroll]
	for(uint i=0; i<3; i++)
	{
#if defined(FINE_GRID) 
		output[i].position = mul(customUB.gridViewProjMatrices[viewIndex], float4(input[i].position, 1.0f)); 
#else
    output[i].position = mul(customUB.gridViewProjMatrices[3+viewIndex], float4(input[i].position, 1.0f)); 
#endif
		output[i].positionWS = input[i].position; // position in world space
		output[i].texCoords = input[i].texCoords;
		output[i].normal = input[i].normal;	
	}

	// Bloat triangle in normalized device space with the texel size of the currently bound 
	// render-target. In this way pixels, which would have been discarded due to the low 
	// resolution of the currently bound render-target, will still be rasterized. 
  float2 side0N = normalize(output[1].position.xy-output[0].position.xy);
	float2 side1N = normalize(output[2].position.xy-output[1].position.xy);
	float2 side2N = normalize(output[0].position.xy-output[2].position.xy);
	const float texelSize = 1.0f/64.0f; 
	output[0].position.xy += normalize(-side0N+side2N)*texelSize;
	output[1].position.xy += normalize(side0N-side1N)*texelSize;
	output[2].position.xy += normalize(side1N-side2N)*texelSize;

  [unroll]
	for(uint j=0; j<3; j++)
	  outputStream.Append(output[j]);

  outputStream.RestartStrip();
}