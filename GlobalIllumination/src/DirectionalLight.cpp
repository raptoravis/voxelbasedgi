#include <stdafx.h>
#include <Demo.h>
#include <DirectionalLight.h>

bool DirectionalLight::Create(const Vector3 &direction, const Color &color, float multiplier)
{
	bufferData.direction = direction.GetNormalized();
	bufferData.color = color;
	bufferData.multiplier = multiplier;
	hasShadow = true;
	
	frustumRatio = 0.18f; // portion of complete shadow frustum, that will be used
	
	shadowTexMatrix.Set(0.5f, 0.0f, 0.0f, 0.0f,
		                  0.0f, 0.5f, 0.0f, 0.0f,
											0.0f, 0.0f, 1.0f, 0.0f,
											0.5f, 0.5f, 0.0f, 1.0f);

	CalculateFrustum();
	CalculateMatrices();

	// shader for direct illumination
	lightShader = Demo::resourceManager->LoadShader("shaders/dirLight.sdr");
	if(!lightShader)
		return false;

	// shader for shadow map generation
	shadowMapShader = Demo::resourceManager->LoadShader("shaders/shadowMapDir.sdr");
	if(!shadowMapShader)
		return false;

	// shader for illumination of fine resolution voxel-grid
	lightGridShaders[FINE_GRID] = Demo::resourceManager->LoadShader("shaders/dirLightGrid.sdr", 1); // (Permutation 1 = FINE_GRID)
	if(!lightGridShaders[FINE_GRID])
		return false;

	// shader for illumination of coarse resolution voxel-grid
	lightGridShaders[COARSE_GRID] = Demo::resourceManager->LoadShader("shaders/dirLightGrid.sdr");
	if(!lightGridShaders[COARSE_GRID])
		return false;

	uniformBuffer = Demo::renderer->CreateUniformBuffer(sizeof(BufferData));
	if(!uniformBuffer)
		return false;
	
	UpdateUniformBuffer();

	RasterizerDesc rasterDesc;
	noneCullRS = Demo::renderer->CreateRasterizerState(rasterDesc);
	if(!noneCullRS)
		return false;
	rasterDesc.cullMode = BACK_CULL;
	backCullRS = Demo::renderer->CreateRasterizerState(rasterDesc);
	if(!backCullRS)
		return false;

	DepthStencilDesc depthStencilDesc;
	defaultDSS = Demo::renderer->CreateDepthStencilState(depthStencilDesc);
	if(!defaultDSS)
		return false;
	depthStencilDesc.depthTest = false;
	depthStencilDesc.depthMask = false;

	// only illuminate actual geometry, not sky
	depthStencilDesc.stencilTest = true; 
  depthStencilDesc.stencilRef = 1;
	depthStencilDesc.stencilPassOp = KEEP_STENCIL_OP;

	noDepthTestDSS = Demo::renderer->CreateDepthStencilState(depthStencilDesc);
	if(!noDepthTestDSS)
		return false;
	
	BlendDesc blendDesc;
	blendDesc.colorMask = 0;
	noColorBS = Demo::renderer->CreateBlendState(blendDesc);
	if(!noColorBS)
		return false;
	blendDesc.colorMask = ALL_COLOR_MASK;
	blendDesc.blend = true;
	blendBS = Demo::renderer->CreateBlendState(blendDesc);
	if(!blendBS)
		return false;

	// render direct illumination only into accumulation render-target of GBuffers
  RtConfigDesc rtcDesc;
	rtcDesc.numColorBuffers = 1;
  rtcDesc.flags = DS_READ_ONLY_RTCF;
	rtConfig = Demo::renderer->CreateRenderTargetConfig(rtcDesc);
	if(!rtConfig)
		return false;

  // cache pointer to GlobalIllum post-processor
	globalIllumPP = (GlobalIllum*)Demo::renderer->GetPostProcessor("GlobalIllum");
	if(!globalIllumPP)
		return false; 
	
	index = Demo::renderer->GetNumLights();

	return true;
}

void DirectionalLight::CalculateFrustum()
{
	// get corners of camera frustum in view-space  
	const Camera *camera = Demo::renderer->GetCamera(MAIN_CAMERA_ID);
	Vector3 frustumCornersVS[8] =
	{
		Vector3(-1.0f, 1.0f, 0.0f),
		Vector3(1.0f, 1.0f, 0.0f),
		Vector3(1.0f, -1.0f, 0.0f),
		Vector3(-1.0f, -1.0f, 0.0f),
		Vector3(-1.0f, 1.0f, 1.0f),
		Vector3(1.0f, 1.0f, 1.0f),
		Vector3(1.0f, -1.0f, 1.0f),
		Vector3(-1.0f, -1.0f, 1.0f)
	};
	for(unsigned int i=0; i<8; i++)
		frustumCornersVS[i] = camera->GetInvProjMatrix()*frustumCornersVS[i];

  // get corners of shadow frustum in view space
	Vector3 shadowFrustumCornersVS[8];
	for(unsigned int i=0; i<4; i++)
	{
		Vector3 cornerRay = frustumCornersVS[i+4]-frustumCornersVS[i];
		Vector3 farCornerRay = cornerRay*frustumRatio;
		shadowFrustumCornersVS[i] = frustumCornersVS[i];
		shadowFrustumCornersVS[i+4] = frustumCornersVS[i]+farCornerRay;
	}

	// calculate radius of bounding-sphere for the shadow frustum
	Vector3 centerVS(0.0f, 0.0f, 0.0f);
	for(unsigned int i=0; i<8; i++)
		centerVS += shadowFrustumCornersVS[i];
	centerVS /= 8.0f;
	frustumRadius = 0.0f;
	for(unsigned int i=0; i<8; i++)
	{
		float distance = (shadowFrustumCornersVS[i]-centerVS).GetLength();
		if(distance > frustumRadius)
			frustumRadius = distance;
	}

	// calculate shadowProjMatrix
	shadowProjMatrix.SetOrtho(-frustumRadius, frustumRadius, -frustumRadius, frustumRadius, 0.0f, frustumRadius*2.0f); 
}

void DirectionalLight::CalculateMatrices()
{
	// get light-position
	const Camera *camera = Demo::renderer->GetCamera(MAIN_CAMERA_ID);
	float distance = camera->GetNearFarClipDistance()*frustumRatio*0.5f;
	Vector3 center = camera->GetPosition()+(camera->GetDirection()*distance);
	Vector3 position = center-(bufferData.direction*frustumRadius);

	// calculate shadowViewProjMatrix 
	Matrix4 transMatrix, rotMatrix, shadowViewMatrix;
	transMatrix.SetTranslation(position);
	rotMatrix.SetRotation(bufferData.direction);
	shadowViewMatrix = transMatrix*rotMatrix;
	shadowViewMatrix = shadowViewMatrix.GetInverse();
	bufferData.shadowViewProjMatrix = shadowProjMatrix*shadowViewMatrix;

	// prevent flickering of shadow map when main camera is moving
	Vector3 shadowOrigin(0.0f, 0.0f, 0.0f);
	shadowOrigin = bufferData.shadowViewProjMatrix*shadowOrigin;
	float shadowMapSize = (float)Demo::renderer->GetRenderTarget(SHADOW_MAP_RT_ID)->GetWidth();
	bufferData.invShadowMapSize = 1.0f/shadowMapSize;
	shadowOrigin *= shadowMapSize*0.5f;
	Vector3 roundedOrigin = shadowOrigin.GetFloored();
	Vector3 roundOffset = roundedOrigin-shadowOrigin;   
	roundOffset *= 2.0f*bufferData.invShadowMapSize;
	Matrix4 roundedShadowProjMatrix = shadowProjMatrix;
	roundedShadowProjMatrix.entries[12] += roundOffset.x;
	roundedShadowProjMatrix.entries[13] += roundOffset.y;
	bufferData.shadowViewProjMatrix = roundedShadowProjMatrix*shadowViewMatrix;

	// calculate shadowViewProjTexMatrix
	bufferData.shadowViewProjTexMatrix = shadowTexMatrix*bufferData.shadowViewProjMatrix;
}

void DirectionalLight::UpdateUniformBuffer()
{
	uniformBuffer->Update(&bufferData);
}

void DirectionalLight::Update()
{	
	if(!active)
    return;
	CalculateMatrices(); 
	UpdateUniformBuffer();
}

void DirectionalLight::SetupShadowMapSurface(DrawCmd &drawCmd)
{
	drawCmd.renderTarget = Demo::renderer->GetRenderTarget(SHADOW_MAP_RT_ID);
	drawCmd.light = this;
  drawCmd.rasterizerState = backCullRS;	
	drawCmd.depthStencilState = defaultDSS;
	drawCmd.blendState = noColorBS;
  drawCmd.shader = shadowMapShader; 
}

void DirectionalLight::AddLitSurface()
{
	if(!active)
		return;
  DX11_RenderTarget *gBuffersRT = Demo::renderer->GetRenderTarget(GBUFFERS_RT_ID);
	GpuCmd gpuCmd(DRAW_CM);
	gpuCmd.order = ILLUM_CO;
	gpuCmd.draw.renderTarget = gBuffersRT;
	gpuCmd.draw.renderTargetConfig = rtConfig;
	gpuCmd.draw.camera = Demo::renderer->GetCamera(MAIN_CAMERA_ID);
  gpuCmd.draw.primitiveType = TRIANGLES_PRIMITIVE;
	gpuCmd.draw.numElements = 3;
	gpuCmd.draw.textures[COLOR_TEX_ID] = gBuffersRT->GetTexture(1); // albedoGloss
	gpuCmd.draw.textures[NORMAL_TEX_ID] = gBuffersRT->GetTexture(2); // normal 
  gpuCmd.draw.textures[SPECULAR_TEX_ID] = gBuffersRT->GetDepthStencilTexture(); // depth
	gpuCmd.draw.textures[CUSTOM0_TEX_ID] = Demo::renderer->GetRenderTarget(SHADOW_MAP_RT_ID)->GetDepthStencilTexture(); // shadow map
  gpuCmd.draw.samplers[CUSTOM0_TEX_ID] = Demo::renderer->GetSampler(SHADOW_MAP_SAMPLER_ID);
	gpuCmd.draw.light = this;
	gpuCmd.draw.rasterizerState = noneCullRS;
	gpuCmd.draw.depthStencilState = noDepthTestDSS;
	gpuCmd.draw.blendState = blendBS;
	gpuCmd.draw.shader = lightShader;
	Demo::renderer->AddGpuCmd(gpuCmd);
}

void DirectionalLight::AddGridSurfaces()
{
	if(!active)
		return;

	// illuminate fine and coarse resolution voxel-grid of GlobalIllum post-processor
	for(unsigned int i=0; i<2; i++)
	{
		GpuCmd gpuCmd(DRAW_CM);
		gpuCmd.order = GRID_ILLUM_CO;
		gpuCmd.draw.light = this;
		gpuCmd.draw.textures[COLOR_TEX_ID] = Demo::renderer->GetRenderTarget(SHADOW_MAP_RT_ID)->GetDepthStencilTexture(); // shadow map
    gpuCmd.draw.samplers[COLOR_TEX_ID] = Demo::renderer->GetSampler(SHADOW_MAP_SAMPLER_ID);
		gpuCmd.draw.shader = lightGridShaders[i];
		globalIllumPP->SetupLightGridSurface(gpuCmd.draw, (gridTypes)i);
		Demo::renderer->AddGpuCmd(gpuCmd); 
	}
}