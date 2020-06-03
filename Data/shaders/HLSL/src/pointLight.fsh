#include "globals.shi"

Texture2D albedoGlossMap: register(COLOR_TEX_BP);
Texture2D normalMap: register(NORMAL_TEX_BP); 
Texture2D depthMap: register(SPECULAR_TEX_BP); 

GLOBAL_CAMERA_UB(cameraUB);
GLOBAL_POINT_LIGHT_UB(pointLightUB);

struct VS_Output
{
	float4 position: SV_POSITION;
	float3 screenPos: SCREEN_POS;
};

struct FS_Output
{
  float4 fragColor: SV_TARGET;
};

FS_Output main(VS_Output input) 
{
  FS_Output output;

  int3 texCoords = int3(input.position.xy, 0); 
	float4 albedoGloss = albedoGlossMap.Load(texCoords); 
	float3 bump = normalMap.Load(texCoords).xyz*2.0f-1.0f; 
  float depth = depthMap.Load(texCoords).x;  

	// reconstruct world-space position from depth
  float2 sceneTC = input.screenPos.xy/input.screenPos.z; 
  float4 projPosition = float4(sceneTC, depth, 1.0f);
  projPosition.xy = (projPosition.xy*2.0f)-1.0f;
  projPosition.y = -projPosition.y;
	float4 position = mul(cameraUB.invViewProjMatrix, projPosition);
  position.xyz /= position.w;
	
	float3 lightVec = pointLightUB.position-position.xyz;
	float3 viewVecN = normalize(cameraUB.position-position.xyz);
	
	float lightVecLen = length(lightVec);
	float att = saturate(1.0f-(lightVecLen/pointLightUB.radius));
  float3 lightVecN = lightVec/lightVecLen;

  float3 halfVecN = normalize(lightVecN+viewVecN); 
  float nDotL = saturate(dot(lightVecN, bump));

  // diffuse term
	float3 diffuseTerm = albedoGloss.rgb;

  // simple Blinn-Phong specular term
	const float shininess = 100.0f;
  float specular = pow(saturate(dot(halfVecN, bump)), shininess);
  float3 specularTerm = albedoGloss.aaa*specular;

  float3 directIllum = (diffuseTerm+specularTerm)*pointLightUB.color.rgb*nDotL*pointLightUB.multiplier;
  directIllum *= att;

	output.fragColor = float4(directIllum, 0.0f);

	return output;
}
 