#include "globals.shi"

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
  float2 texCoords: TEXCOORD; 
  float3 normal: NORMAL; 
};  

struct VS_Output
{
	float3 position: POSITION;
  float2 texCoords: TEXCOORD;
	float3 normal: NORMAL;
};

VS_Output main(VS_Input input)
{
  VS_Output output;
	output.position = input.position+customUB.worldPosition;
	output.texCoords = input.texCoords;
	output.normal = input.normal;
  return output;
}

