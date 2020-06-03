#include "globals.shi"

GLOBAL_CAMERA_UB(cameraUB);

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
  float4 tangent: TANGENT;
};  

struct VS_Output
{
	float4 position: SV_POSITION;
  float2 texCoords: TEXCOORD;
	float3 normal: NORMAL;
	float3 tangent: TANGENT;
	float3 bitangent: BITANGENT;
};

VS_Output main(VS_Input input)
{
  VS_Output output;
	float4 positionWS = float4(input.position+customUB.worldPosition, 1.0f);
  output.position = mul(cameraUB.viewProjMatrix, positionWS);
	output.texCoords = input.texCoords;
        
  output.normal = input.normal;
	output.tangent = input.tangent.xyz;
	output.bitangent = cross(input.normal, input.tangent.xyz)*input.tangent.w; 
		
  return output;
}

