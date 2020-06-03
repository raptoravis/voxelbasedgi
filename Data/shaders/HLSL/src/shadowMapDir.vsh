#include "globals.shi"

GLOBAL_DIR_LIGHT_UB(dirLightUB);

cbuffer CustomUB: register(CUSTOM0_UB_BP)
{
	struct
	{
    float3 worldPosition;
	}customUB;
};

struct VS_Input 
{ 
  float3 position: POSITION; 
};  

struct VS_Output
{
	float4 position: SV_POSITION;
};

VS_Output main(VS_Input input)
{
  VS_Output output;  
	float4 positionWS = float4(input.position+customUB.worldPosition, 1.0f);
	output.position = mul(dirLightUB.shadowViewProjMatrix, positionWS);
	return output;
}