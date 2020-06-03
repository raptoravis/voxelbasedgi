#include "globals.shi"

Texture2D albedoGlossMap: register(COLOR_TEX_BP); 
Texture2D normalMap: register(NORMAL_TEX_BP); 
Texture2D depthMap: register(SPECULAR_TEX_BP); 
Texture2D shadowMap: register(CUSTOM0_TEX_BP); 
SamplerComparisonState shadowMapSampler: register(CUSTOM0_SAM_BP);

GLOBAL_CAMERA_UB(cameraUB);
GLOBAL_DIR_LIGHT_UB(dirLightUB);

struct VS_Output
{
	float4 position: SV_POSITION;
  float2 texCoords: TEXCOORD;
};

struct FS_Output
{
  float4 fragColor: SV_TARGET;
};

#define PCF_NUM_SAMPLES 16
#define SHADOW_FILTER_RADIUS 2.0f
#define SHADOW_BIAS 0.006f

// poisson disk samples
static const float2 filterKernel[PCF_NUM_SAMPLES] = 
{ 
  float2(-0.94201624f, -0.39906216f),
	float2(0.94558609f, -0.76890725f),
	float2(-0.094184101f, -0.92938870f),
	float2(0.34495938f, 0.29387760f),
	float2(-0.91588581f, 0.45771432f),
	float2(-0.81544232f, -0.87912464f),
	float2(-0.38277543f, 0.27676845f),
	float2(0.97484398f, 0.75648379f),
	float2(0.44323325f, -0.97511554f),
	float2(0.53742981f, -0.47373420f),
	float2(-0.26496911f, -0.41893023f),
	float2(0.79197514f, 0.19090188f),
	float2(-0.24188840f, 0.99706507f),
	float2(-0.81409955f, 0.91437590f),
	float2(0.19984126f, 0.78641367f),
	float2(0.14383161f, -0.14100790f)
};

// compute shadow-term using 16x PCF in combination with hardware shadow filtering
float ComputeShadowTerm(in float4 positionWS)
{
  const float4 result = mul(dirLightUB.shadowViewProjTexMatrix, positionWS);
	const float filterRadius = SHADOW_FILTER_RADIUS*dirLightUB.invShadowMapSize;
  const float compareDepth = result.z-SHADOW_BIAS;
	float shadowTerm = 0.0f;
	[unroll]
  for(uint i=0; i<PCF_NUM_SAMPLES; i++)
  {
    float2 offset = filterKernel[i]*filterRadius;
		float2 texCoords = result.xy+offset;
		texCoords.y = 1.0f-texCoords.y;
	  shadowTerm += shadowMap.SampleCmp(shadowMapSampler, texCoords, compareDepth);
  }
  shadowTerm /= PCF_NUM_SAMPLES;
	return shadowTerm;
}

FS_Output main(VS_Output input) 
{
  FS_Output output;

  int3 texCoords = int3(input.position.xy, 0);
	float4 albedoGloss = albedoGlossMap.Load(texCoords); 
	float3 bump = normalMap.Load(texCoords).xyz*2.0f-1.0f;
  float depth = depthMap.Load(texCoords).x; 
	
	// reconstruct world-space position from depth
  float4 projPosition = float4(input.texCoords, depth, 1.0f);
  projPosition.xy = (projPosition.xy*2.0f)-1.0f;
  projPosition.y = -projPosition.y;
	float4 position = mul(cameraUB.invViewProjMatrix, projPosition);
  position.xyz /= position.w;
  position.w = 1.0f;
	
	float3 lightVecN = -dirLightUB.direction;
	float3 viewVecN = normalize(cameraUB.position-position.xyz);
  float3 halfVecN = normalize(lightVecN+viewVecN); 
  float nDotL = saturate(dot(lightVecN, bump));

  // diffuse term
	float3 diffuseTerm = albedoGloss.rgb;

	// simple Blinn-Phong specular term
	const float shininess = 100.0f;
  float specular = pow(saturate(dot(halfVecN, bump)), shininess);
  float3 specularTerm = albedoGloss.aaa*specular;

	float shadowTerm = ComputeShadowTerm(position);
  float3 directIllum = (diffuseTerm+specularTerm)*dirLightUB.color.rgb*nDotL*dirLightUB.multiplier;
  directIllum *= shadowTerm;

	output.fragColor = float4(directIllum, 0.0f);
	
	return output;
}
 