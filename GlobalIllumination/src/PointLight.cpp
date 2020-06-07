#include <stdafx.h>
#include <Demo.h>
#include <PointLight.h>

bool PointLight::Create(const Vector3 &position, float radius, const Color &color, float multiplier)
{
	bufferData.position = position;
	bufferData.radius = radius;
	bufferData.color = color;
	bufferData.multiplier = multiplier;
	hasShadow = false;
	cameraInVolume = false;

	CalculateMatrices();

	// shader for direct illumination
	lightShader = Demo::resourceManager->LoadShader("shaders/pointLight.sdr");
	if (!lightShader)
		return false;

	// shader for illumination of fine resolution voxel-grid
	lightGridShaders[FINE_GRID] = Demo::resourceManager->LoadShader("shaders/pointLightGrid.sdr", 1); // (Permutation 1 = FINE_GRID)
	if (!lightGridShaders[FINE_GRID])
		return false;

	// shader for illumination of coarse resolution voxel-grid
	lightGridShaders[COARSE_GRID] = Demo::resourceManager->LoadShader("shaders/pointLightGrid.sdr");
	if (!lightGridShaders[COARSE_GRID])
		return false;

	uniformBuffer = Demo::renderer->CreateUniformBuffer(sizeof(BufferData));
	if (!uniformBuffer)
		return false;

	UpdateUniformBuffer();

	performUpdate = false;

	RasterizerDesc rasterDesc;
	rasterDesc.cullMode = BACK_CULL;
	backCullRS = Demo::renderer->CreateRasterizerState(rasterDesc);
	if (!backCullRS)
		return false;
	rasterDesc.cullMode = FRONT_CULL;
	frontCullRS = Demo::renderer->CreateRasterizerState(rasterDesc);
	if (!frontCullRS)
		return false;

	DepthStencilDesc depthStencilDesc;
	depthStencilDesc.depthMask = false;

	// only illuminate actual geometry, not sky
	depthStencilDesc.stencilTest = true;
	depthStencilDesc.stencilRef = 1;
	depthStencilDesc.stencilPassOp = KEEP_STENCIL_OP;

	noDepthWriteDSS = Demo::renderer->CreateDepthStencilState(depthStencilDesc);
	if (!noDepthWriteDSS)
		return false;
	depthStencilDesc.depthTest = false;
	noDepthTestDSS = Demo::renderer->CreateDepthStencilState(depthStencilDesc);
	if (!noDepthTestDSS)
		return false;

	BlendDesc blendDesc;
	blendDesc.blend = true;
	blendBS = Demo::renderer->CreateBlendState(blendDesc);
	if (!blendBS)
		return false;

	// render direct illumination only into accumulation render-target of GBuffers
	RtConfigDesc rtcDesc;
	rtcDesc.numColorBuffers = 1;
	rtcDesc.flags = DS_READ_ONLY_RTCF;
	rtConfig = Demo::renderer->CreateRenderTargetConfig(rtcDesc);
	if (!rtConfig)
		return false;

	// cache pointer to GlobalIllum post-processor
	globalIllumPP = (GlobalIllum*)Demo::renderer->GetPostProcessor("GlobalIllum");
	if (!globalIllumPP)
		return false;

	index = Demo::renderer->GetNumLights();

	return true;
}

bool PointLight::IsSphereInVolume(const Vector3 &position, float radius) const
{
	float distance = (bufferData.position - position).GetLength();
	return (distance <= (bufferData.radius + radius));
}

void PointLight::CalculateMatrices()
{
	if (!performUpdate)
		return;

	// slightly increase radius to compensate for low tessellation of used sphere geometry
	float dilatedRadius = bufferData.radius + 10.0f;

	// calculate worldMatrix of sphere geometry 
	Vector3 scale(dilatedRadius, dilatedRadius, dilatedRadius);
	Matrix4 transMatrix, scaleMatrix;
	transMatrix.SetTranslation(bufferData.position);
	scaleMatrix.SetScale(scale);
	bufferData.worldMatrix = transMatrix * scaleMatrix;
}

void PointLight::UpdateUniformBuffer()
{
	if (!performUpdate)
		return;
	uniformBuffer->Update(&bufferData);
}

void PointLight::Update()
{
	if (!active)
		return;
	cameraInVolume = IsSphereInVolume(Demo::renderer->GetCamera(MAIN_CAMERA_ID)->GetPosition(), 10.0f);
	CalculateMatrices();
	UpdateUniformBuffer();
	performUpdate = false;
}

void PointLight::SetPosition(const Vector3 &position)
{
	if (bufferData.position == position)
		return;
	bufferData.position = position;
	performUpdate = true;
}

void PointLight::SetRadius(float radius)
{
	if (IS_EQUAL(bufferData.radius, radius))
		return;
	bufferData.radius = radius;
	performUpdate = true;
}

void PointLight::SetColor(const Color &color)
{
	if (bufferData.color == color)
		return;
	bufferData.color = color;
	performUpdate = true;
}

void PointLight::SetMultiplier(float multiplier)
{
	if (IS_EQUAL(bufferData.multiplier, multiplier))
		return;
	bufferData.multiplier = multiplier;
	performUpdate = true;
}

void PointLight::AddLitSurface()
{
	if (!active)
		return;
	const Mesh *sphereMesh = Demo::renderer->GetMesh(UNIT_SPHERE_MESH_ID);
	DX11_RenderTarget *gBuffersRT = Demo::renderer->GetRenderTarget(GBUFFERS_RT_ID);
	GpuCmd gpuCmd(DRAW_CM);
	gpuCmd.order = ILLUM_CO;
	gpuCmd.draw.renderTarget = gBuffersRT;
	gpuCmd.draw.renderTargetConfig = rtConfig;
	gpuCmd.draw.camera = Demo::renderer->GetCamera(MAIN_CAMERA_ID);
	gpuCmd.draw.light = this;
	gpuCmd.draw.vertexLayout = sphereMesh->vertexLayout;
	gpuCmd.draw.vertexBuffer = sphereMesh->vertexBuffer;
	gpuCmd.draw.indexBuffer = sphereMesh->indexBuffer;
	gpuCmd.draw.primitiveType = sphereMesh->primitiveType;
	gpuCmd.draw.firstIndex = 0;
	gpuCmd.draw.numElements = sphereMesh->indexBuffer->GetIndexCount();
	gpuCmd.draw.textures[COLOR_TEX_ID] = gBuffersRT->GetTexture(1); // albedoGloss
	gpuCmd.draw.textures[NORMAL_TEX_ID] = gBuffersRT->GetTexture(2); // normal
	gpuCmd.draw.textures[SPECULAR_TEX_ID] = gBuffersRT->GetDepthStencilTexture(); // depth

	  // When camera not in light volume, do depth-testing + back-face culling, otherwise disable
	  // depth-testing and do front-face culling.
	if (!cameraInVolume)
	{
		gpuCmd.draw.rasterizerState = backCullRS;
		gpuCmd.draw.depthStencilState = noDepthWriteDSS;
	}
	else
	{
		gpuCmd.draw.rasterizerState = frontCullRS;
		gpuCmd.draw.depthStencilState = noDepthTestDSS;
	}

	gpuCmd.draw.blendState = blendBS;
	gpuCmd.draw.shader = lightShader;
	Demo::renderer->AddGpuCmd(gpuCmd);
}

void PointLight::AddGridSurfaces()
{
	if (!active)
		return;

	// illuminate fine and coarse resolution voxel-grid of GlobalIllum post-processor
	for (unsigned int i = 0; i < 2; i++)
	{
		GpuCmd gpuCmd(DRAW_CM);
		gpuCmd.order = GRID_ILLUM_CO;
		gpuCmd.draw.light = this;
		gpuCmd.draw.shader = lightGridShaders[i];
		globalIllumPP->SetupLightGridSurface(gpuCmd.draw, (gridTypes)i);
		Demo::renderer->AddGpuCmd(gpuCmd);
	}
}