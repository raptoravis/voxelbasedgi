#include "globals.shi"

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
	float2 position: POSITION;
	uint instanceID: INSTANCE_ID;
};

struct GS_Output
{
  float4 position: SV_POSITION;
  uint rtIndex : SV_RenderTargetArrayIndex; 
};

[maxvertexcount(3)]
void main(triangle VS_Output input[3], inout TriangleStream<GS_Output> outputStream)
{       
  [unroll]
	for(uint i=0; i<3; i++)
  {
    GS_Output output;
    output.position = float4(input[i].position, 0.0f, 1.0f);
    output.rtIndex = input[0].instanceID; // write 32 instances of primitive into 32 slices of 2D texture array
    outputStream.Append(output);
	}

	outputStream.RestartStrip();
}






