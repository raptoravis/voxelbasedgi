#include "globals.shi"

struct VS_Input
{
  float3 position: POSITION; 
  float2 texCoords: TEXCOORD;
  float4 color: COLOR;
};

struct VS_Output
{
	float3 position: POSITION;
  float2 texCoords: TEXCOORD;
	float4 color: COLOR;
};

VS_Output main(VS_Input input)
{
  VS_Output output;                                        
	output.position = input.position;
	output.texCoords = input.texCoords;
	output.color = input.color;
	return output;
}