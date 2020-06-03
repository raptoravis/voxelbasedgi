#include "globals.shi"

GLOBAL_CAMERA_UB(cameraUB);
GLOBAL_POINT_LIGHT_UB(pointLightUB);

struct VS_Input
{
  float3 position: POSITION;
};

struct VS_Output
{
	float4 position: SV_POSITION;
	float3 screenPos: SCREEN_POS;
};

VS_Output main(VS_Input input)
{
  VS_Output output;           
	float4 positionWS = mul(pointLightUB.worldMatrix, float4(input.position, 1.0f));
	output.position = mul(cameraUB.viewProjMatrix, positionWS);
  output.screenPos = float3((float2(output.position.x, -output.position.y)+output.position.ww)*0.5f, output.position.w);
	return output;
}