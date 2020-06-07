#include <stdafx.h>
#include <Demo.h>
#include <DX11_Renderer.h>

void DX11_Renderer::Destroy()
{
	SAFE_DELETE_PLIST(samplers);
	SAFE_DELETE_PLIST(rasterizerStates);
	SAFE_DELETE_PLIST(depthStencilStates);
	SAFE_DELETE_PLIST(blendStates);
	SAFE_DELETE_PLIST(renderTargetConfigs);
	SAFE_DELETE_PLIST(renderTargets);
	SAFE_DELETE_PLIST(vertexLayouts);
	SAFE_DELETE_PLIST(vertexBuffers);
	SAFE_DELETE_PLIST(indexBuffers);
	SAFE_DELETE_PLIST(uniformBuffers);
	SAFE_DELETE_PLIST(structuredBuffers);
	SAFE_DELETE_PLIST(cameras);
	SAFE_DELETE_PLIST(lights);
	SAFE_DELETE_PLIST(meshes);
	SAFE_DELETE_PLIST(postProcessors);

	// reset device-context to default settings
	if (deviceContext)
		deviceContext->ClearState();

	// release swap-chain
	SAFE_RELEASE(swapChain);

	// release device-context
	SAFE_RELEASE(deviceContext);

	// release device
	SAFE_RELEASE(device);
}

bool DX11_Renderer::Create()
{
	// create device/ device-context/ swap chain
	DXGI_SWAP_CHAIN_DESC desc;
	ZeroMemory(&desc, sizeof(DXGI_SWAP_CHAIN_DESC));
	desc.BufferDesc.Width = SCREEN_WIDTH;
	desc.BufferDesc.Height = SCREEN_HEIGHT;
	desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.BufferCount = 2;
	desc.OutputWindow = Demo::window->GetHWnd();
	desc.Windowed = TRUE;
	desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	UINT createDeviceFlags = D3D11_CREATE_DEVICE_SINGLETHREADED;
#ifdef _DEBUG
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif 

	if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, NULL, 0,
		D3D11_SDK_VERSION, &desc, &swapChain, &device, NULL, &deviceContext) != S_OK)
	{
		MessageBox(NULL, "Failed to create a DX11 device!", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		Destroy();
		return false;
	}

	// check, if required feature-level is supported
	D3D_FEATURE_LEVEL featureLevel = device->GetFeatureLevel();
	if (featureLevel < D3D_FEATURE_LEVEL_11_0)
	{
		MessageBox(NULL, "Feature Level 11.0 not supported!", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		Destroy();
		return false;
	}

	// disable switching mode by ALT+Enter
	IDXGIDevice *deviceDXGI = NULL;
	if (device->QueryInterface(__uuidof(IDXGIDevice), (void **)&deviceDXGI) != S_OK)
	{
		Destroy();
		return false;
	}
	IDXGIAdapter *adapterDXGI = NULL;
	if (deviceDXGI->GetParent(__uuidof(IDXGIAdapter), (void **)&adapterDXGI) != S_OK)
	{
		SAFE_RELEASE(deviceDXGI);
		Destroy();
		return false;
	}
	IDXGIFactory *factoryDXGI = NULL;
	if (adapterDXGI->GetParent(__uuidof(IDXGIFactory), (void **)&factoryDXGI) != S_OK)
	{
		SAFE_RELEASE(deviceDXGI);
		SAFE_RELEASE(adapterDXGI);
		Destroy();
		return false;
	}
	if (factoryDXGI->MakeWindowAssociation(Demo::window->GetHWnd(), DXGI_MWA_NO_ALT_ENTER) != S_OK)
	{
		SAFE_RELEASE(deviceDXGI);
		SAFE_RELEASE(adapterDXGI);
		SAFE_RELEASE(factoryDXGI);
		Destroy();
		return false;
	}
	SAFE_RELEASE(deviceDXGI);
	SAFE_RELEASE(adapterDXGI);
	SAFE_RELEASE(factoryDXGI);

	if (!CreateDefaultObjects())
		return false;

	// pre-allocate some GPU commands, to prevent initial stutters
	gpuCmds.Resize(256);

	return true;
}

bool DX11_Renderer::CreateDefaultObjects()
{
	// create frequently used samplers

	// LINEAR_SAMPLER
	SamplerDesc samplerDesc;
	if (!CreateSampler(samplerDesc))
		return false;

	// TRILINEAR_SAMPLER
	samplerDesc.filter = MIN_MAG_MIP_LINEAR_FILTER;
	samplerDesc.adressU = REPEAT_TEX_ADDRESS;
	samplerDesc.adressV = REPEAT_TEX_ADDRESS;
	samplerDesc.adressW = REPEAT_TEX_ADDRESS;
	if (!CreateSampler(samplerDesc))
		return false;

	// SHADOW_MAP_SAMPLER
	samplerDesc.filter = COMP_MIN_MAG_LINEAR_FILTER;
	samplerDesc.adressU = CLAMP_TEX_ADRESS;
	samplerDesc.adressV = CLAMP_TEX_ADRESS;
	samplerDesc.adressW = CLAMP_TEX_ADRESS;
	samplerDesc.compareFunc = LEQUAL_COMP_FUNC;
	if (!CreateSampler(samplerDesc))
		return false;


	// create frequently used render-targets

	// BACK_BUFFER_RT 
	if (!CreateBackBufferRt())
		return false;

	{
		// GBUFFERS_RT	
		// 1. frameBufferTextures[0]:
		//    accumulation buffer
		// 2. frameBufferTextures[1]:
		//    RGB-channel: albedo, Alpha-channel: specular intensity
		// 3. frameBufferTextures[2]:
		//    RGB-channel: normal, Alpha-channel: unused
		RenderTargetDesc rtDesc;
		rtDesc.width = SCREEN_WIDTH;
		rtDesc.height = SCREEN_HEIGHT;
		rtDesc.colorBufferDescs[0].format = TEX_FORMAT_RGBA16F;
		rtDesc.colorBufferDescs[1].format = TEX_FORMAT_RGBA8_SRGB;
		rtDesc.colorBufferDescs[2].format = TEX_FORMAT_RGB10A2;
		rtDesc.depthStencilBufferDesc.format = TEX_FORMAT_DEPTH24;
		if (!CreateRenderTarget(rtDesc))
			return false;
	}

	{
		// SHADOW_MAP_RT
		RenderTargetDesc rtDesc;
		rtDesc.width = 1024;
		rtDesc.height = 1024;
		rtDesc.depthStencilBufferDesc.format = TEX_FORMAT_DEPTH24;
		if (!CreateRenderTarget(rtDesc))
			return false;
	}


	// create render-states, frequently used by post-processors
	RasterizerDesc rasterDesc;
	noneCullRS = CreateRasterizerState(rasterDesc);
	if (!noneCullRS)
		return false;

	DepthStencilDesc depthStencilDesc;
	depthStencilDesc.depthTest = false;
	depthStencilDesc.depthMask = false;
	noDepthTestDSS = CreateDepthStencilState(depthStencilDesc);
	if (!noDepthTestDSS)
		return false;

	BlendDesc blendDesc;
	defaultBS = CreateBlendState(blendDesc);
	if (!defaultBS)
		return false;


	// create frequently used cameras

	// MAIN_CAMERA
	if (!CreateCamera(80.0f, (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT, 2.0f, 5000.0f))
		return false;


	// create frequently used meshes

	// UNIT_SPHERE_MESH
	if (!CreateUnitSphere())
		return false;


	return true;
}

bool DX11_Renderer::CreateUnitSphere()
{
	// Create low tessellated unit sphere, used for example for rendering deferred point-lights.
	const unsigned int tessellation = 18;
	const unsigned int numSphereVertices = 2 + (((tessellation / 2) - 2)*(tessellation / 2) * 4);
	const unsigned int numSphereIndices = ((tessellation) * 6) + (((tessellation / 2) - 2)*(tessellation / 2) * 12);
	VertexElementDesc vertexElementDescs[1] = { POSITION_ELEMENT, R32G32B32_FLOAT_EF, 0 };
	MeshDesc meshDesc;
	meshDesc.primitiveType = TRIANGLES_PRIMITIVE;
	meshDesc.vertexElementDescs = vertexElementDescs;
	meshDesc.numVertexElementDescs = 1;
	meshDesc.dynamic = false;
	meshDesc.numVertices = numSphereVertices;
	meshDesc.numIndices = numSphereIndices;
	Mesh *sphereMesh = CreateMesh(meshDesc);
	if (!sphereMesh)
		return false;

	Vector3 *sphereVertices = new Vector3[numSphereVertices];
	if (!sphereVertices)
		return false;
	unsigned int *sphereIndices = new unsigned int[numSphereIndices];
	if (!sphereIndices)
	{
		SAFE_DELETE_ARRAY(sphereVertices);
		return false;
	}

	// create vertices
	unsigned int vertexIndex = 0;
	sphereVertices[vertexIndex++].Set(0.0f, -1.0f, 0.0f);
	const unsigned int count = tessellation / 2 - 1;
	for (unsigned int i = 1; i < count; i++)
	{
		Vector4 theta;
		theta.x = ((i*TWOPI) / tessellation) - PIDIV2;
		theta.y = (((i + 1)*TWOPI) / tessellation) - PIDIV2;
		Vector4 sinTheta;
		sinTheta.x = sin(theta.x);
		sinTheta.y = sin(theta.y);
		Vector4 cosTheta;
		cosTheta.x = cos(theta.x);
		cosTheta.y = cos(theta.y);
		for (unsigned int j = 0; j < tessellation; j += 2)
		{
			theta.z = (j*TWOPI) / tessellation;
			theta.w = ((j + 1)*TWOPI) / tessellation;
			sinTheta.z = sin(theta.z);
			sinTheta.w = sin(theta.w);
			cosTheta.z = cos(theta.z);
			cosTheta.w = cos(theta.w);
			Vector3 vertex;
			vertex.x = cosTheta.x*cosTheta.z;
			vertex.y = sinTheta.x;
			vertex.z = cosTheta.x*sinTheta.z;
			sphereVertices[vertexIndex++] = vertex;
			vertex.x = cosTheta.y*cosTheta.z;
			vertex.y = sinTheta.y;
			vertex.z = cosTheta.y*sinTheta.z;
			sphereVertices[vertexIndex++] = vertex;
			vertex.x = cosTheta.y*cosTheta.w;
			vertex.y = sinTheta.y;
			vertex.z = cosTheta.y*sinTheta.w;
			sphereVertices[vertexIndex++] = vertex;
			vertex.x = cosTheta.x*cosTheta.w;
			vertex.y = sinTheta.x;
			vertex.z = cosTheta.x*sinTheta.w;
			sphereVertices[vertexIndex++] = vertex;
		}
	}
	sphereVertices[vertexIndex++].Set(0.0f, 1.0f, 0.0f);

	// create lower cap indices
	unsigned int index = 0;
	vertexIndex = 1;
	for (unsigned int i = 0; i < tessellation; i++)
	{
		sphereIndices[index++] = 0;
		sphereIndices[index++] = vertexIndex;
		if ((i % 2) == 0)
			vertexIndex += 3;
		else
			vertexIndex += 1;
		sphereIndices[index++] = (i < (tessellation - 1)) ? vertexIndex : 1;
	}

	// create middle sphere indices
	vertexIndex = 1;
	for (unsigned int i = 1; i < count; i++)
	{
		const unsigned int startIndex = vertexIndex;
		const unsigned int indexCount = tessellation / 2;
		for (unsigned int j = 0; j < indexCount; j++)
		{
			sphereIndices[index++] = vertexIndex++;
			sphereIndices[index++] = vertexIndex++;
			sphereIndices[index++] = vertexIndex;
			sphereIndices[index++] = vertexIndex++;
			sphereIndices[index++] = vertexIndex++;
			sphereIndices[index++] = vertexIndex - 4;
			int nextIndex = (j == (tessellation / 2 - 1)) ? startIndex : vertexIndex;
			sphereIndices[index++] = vertexIndex - 1;
			sphereIndices[index++] = vertexIndex - 2;
			sphereIndices[index++] = nextIndex + 1;
			sphereIndices[index++] = nextIndex + 1;
			sphereIndices[index++] = nextIndex;
			sphereIndices[index++] = vertexIndex - 1;
		}
	}

	// create upper cap indices	
	unsigned int lastIndex = vertexIndex;
	vertexIndex -= 2;
	for (unsigned int i = 0; i < tessellation; i++)
	{
		sphereIndices[index++] = lastIndex;
		sphereIndices[index++] = vertexIndex;
		if ((i % 2) == 0)
			vertexIndex -= 1;
		else
			vertexIndex -= 3;
		sphereIndices[index++] = (i < (tessellation - 1)) ? vertexIndex : (lastIndex - 2);
	}

	sphereMesh->vertexBuffer->AddVertices(numSphereVertices, (float*)sphereVertices);
	sphereMesh->vertexBuffer->Update();
	sphereMesh->indexBuffer->AddIndices(numSphereIndices, sphereIndices);
	sphereMesh->indexBuffer->Update();

	SAFE_DELETE_ARRAY(sphereVertices);
	SAFE_DELETE_ARRAY(sphereIndices);

	return true;
}

DX11_Sampler* DX11_Renderer::CreateSampler(const SamplerDesc &desc)
{
	for (unsigned int i = 0; i < samplers.GetSize(); i++)
	{
		if (samplers[i]->GetDesc() == desc)
			return samplers[i];
	}
	DX11_Sampler *sampler = new DX11_Sampler;
	if (!sampler)
		return NULL;
	if (!sampler->Create(desc))
	{
		SAFE_DELETE(sampler);
		return NULL;
	}
	samplers.AddElement(&sampler);
	return sampler;
}

DX11_RasterizerState* DX11_Renderer::CreateRasterizerState(const RasterizerDesc &desc)
{
	for (unsigned int i = 0; i < rasterizerStates.GetSize(); i++)
	{
		if (rasterizerStates[i]->GetDesc() == desc)
			return rasterizerStates[i];
	}
	DX11_RasterizerState *rasterizerState = new DX11_RasterizerState;
	if (!rasterizerState)
		return NULL;
	if (!rasterizerState->Create(desc))
	{
		SAFE_DELETE(rasterizerState);
		return NULL;
	}
	rasterizerStates.AddElement(&rasterizerState);
	return rasterizerState;
}

DX11_DepthStencilState* DX11_Renderer::CreateDepthStencilState(const DepthStencilDesc &desc)
{
	for (unsigned int i = 0; i < depthStencilStates.GetSize(); i++)
	{
		if (depthStencilStates[i]->GetDesc() == desc)
			return depthStencilStates[i];
	}
	DX11_DepthStencilState *depthStencilState = new DX11_DepthStencilState;
	if (!depthStencilState)
		return NULL;
	if (!depthStencilState->Create(desc))
	{
		SAFE_DELETE(depthStencilState);
		return NULL;
	}
	depthStencilStates.AddElement(&depthStencilState);
	return depthStencilState;
}

DX11_BlendState* DX11_Renderer::CreateBlendState(const BlendDesc &desc)
{
	for (unsigned int i = 0; i < blendStates.GetSize(); i++)
	{
		if (blendStates[i]->GetDesc() == desc)
			return blendStates[i];
	}
	DX11_BlendState *blendState = new DX11_BlendState;
	if (!blendState)
		return NULL;
	if (!blendState->Create(desc))
	{
		SAFE_DELETE(blendState);
		return NULL;
	}
	blendStates.AddElement(&blendState);
	return blendState;
}

DX11_RenderTargetConfig* DX11_Renderer::CreateRenderTargetConfig(const RtConfigDesc &desc)
{
	for (unsigned int i = 0; i < renderTargetConfigs.GetSize(); i++)
	{
		if (renderTargetConfigs[i]->GetDesc() == desc)
			return renderTargetConfigs[i];
	}
	DX11_RenderTargetConfig *renderTargetConfig = new DX11_RenderTargetConfig;
	if (!renderTargetConfig)
		return NULL;
	if (!renderTargetConfig->Create(desc))
	{
		SAFE_DELETE(renderTargetConfig);
		return NULL;
	}
	renderTargetConfigs.AddElement(&renderTargetConfig);
	return renderTargetConfig;
}

DX11_RenderTarget* DX11_Renderer::CreateRenderTarget(const RenderTargetDesc &desc)
{
	DX11_RenderTarget *renderTarget = new DX11_RenderTarget;
	if (!renderTarget)
		return NULL;
	if (!renderTarget->Create(desc))
	{
		SAFE_DELETE(renderTarget);
		return NULL;
	}
	renderTargets.AddElement(&renderTarget);
	return renderTarget;
}

DX11_RenderTarget* DX11_Renderer::CreateBackBufferRt()
{
	DX11_RenderTarget *backBuffer = new DX11_RenderTarget;
	if (!backBuffer)
		return NULL;
	if (!backBuffer->CreateBackBuffer())
	{
		SAFE_DELETE(backBuffer);
		return NULL;
	}
	renderTargets.AddElement(&backBuffer);
	return backBuffer;
}

DX11_VertexLayout* DX11_Renderer::CreateVertexLayout(const VertexElementDesc *vertexElementDescs, unsigned int numVertexElementDescs)
{
	for (unsigned int i = 0; i < vertexLayouts.GetSize(); i++)
	{
		if (vertexLayouts[i]->IsEqual(vertexElementDescs, numVertexElementDescs))
			return vertexLayouts[i];
	}
	DX11_VertexLayout *vertexLayout = new DX11_VertexLayout;
	if (!vertexLayout)
		return NULL;
	if (!vertexLayout->Create(vertexElementDescs, numVertexElementDescs))
	{
		SAFE_DELETE(vertexLayout);
		return NULL;
	}
	vertexLayouts.AddElement(&vertexLayout);
	return vertexLayout;
}

DX11_VertexBuffer* DX11_Renderer::CreateVertexBuffer(unsigned int vertexSize, unsigned int maxVertexCount, bool dynamic)
{
	DX11_VertexBuffer *vertexBuffer = new DX11_VertexBuffer();
	if (!vertexBuffer)
		return NULL;
	if (!vertexBuffer->Create(vertexSize, maxVertexCount, dynamic))
	{
		SAFE_DELETE(vertexBuffer);
		return NULL;
	}
	vertexBuffers.AddElement(&vertexBuffer);
	return vertexBuffer;
}

DX11_IndexBuffer* DX11_Renderer::CreateIndexBuffer(unsigned int maxIndexCount, bool dynamic)
{
	DX11_IndexBuffer *indexBuffer = new DX11_IndexBuffer;
	if (!indexBuffer)
		return NULL;
	if (!indexBuffer->Create(maxIndexCount, dynamic))
	{
		SAFE_DELETE(indexBuffer);
		return NULL;
	}
	indexBuffers.AddElement(&indexBuffer);
	return indexBuffer;
}

DX11_UniformBuffer* DX11_Renderer::CreateUniformBuffer(unsigned int bufferSize)
{
	DX11_UniformBuffer *uniformBuffer = new DX11_UniformBuffer;
	if (!uniformBuffer)
		return NULL;
	if (!uniformBuffer->Create(bufferSize))
	{
		SAFE_DELETE(uniformBuffer);
		return NULL;
	}
	uniformBuffers.AddElement(&uniformBuffer);
	return uniformBuffer;
}

DX11_StructuredBuffer* DX11_Renderer::CreateStructuredBuffer(unsigned int elementCount, unsigned int elementSize)
{
	DX11_StructuredBuffer *structuredBuffer = new DX11_StructuredBuffer;
	if (!structuredBuffer)
		return NULL;
	if (!structuredBuffer->Create(elementCount, elementSize))
	{
		SAFE_DELETE(structuredBuffer);
		return NULL;
	}
	structuredBuffers.AddElement(&structuredBuffer);
	return structuredBuffer;
}

Camera* DX11_Renderer::CreateCamera(float fovy, float aspectRatio, float nearClipDistance, float farClipDistance)
{
	Camera *camera = new Camera;
	if (!camera)
		return NULL;
	if (!camera->Init(fovy, aspectRatio, nearClipDistance, farClipDistance))
	{
		SAFE_DELETE(camera);
		return NULL;
	}
	cameras.AddElement(&camera);
	return camera;
}

PointLight* DX11_Renderer::CreatePointLight(const Vector3 &position, float radius, const Color &color, float multiplier)
{
	PointLight *pointLight = new PointLight;
	if (!pointLight)
		return false;
	if (!pointLight->Create(position, radius, color, multiplier))
	{
		SAFE_DELETE(pointLight);
		return NULL;
	}
	lights.AddElement((ILight**)(&pointLight));
	return pointLight;
}

DirectionalLight* DX11_Renderer::CreateDirectionalLight(const Vector3 &direction, const Color &color, float multiplier)
{
	DirectionalLight *directionalLight = new DirectionalLight;
	if (!directionalLight)
		return false;
	if (!directionalLight->Create(direction, color, multiplier))
	{
		SAFE_DELETE(directionalLight);
		return NULL;
	}
	lights.AddElement((ILight**)(&directionalLight));
	return directionalLight;
}

Mesh* DX11_Renderer::CreateMesh(const MeshDesc &desc)
{
	Mesh *mesh = new Mesh;
	if (!mesh)
		return NULL;
	if (!mesh->Create(desc))
	{
		SAFE_DELETE(mesh);
		return NULL;
	}
	meshes.AddElement(&mesh);
	return mesh;
}

IPostProcessor* DX11_Renderer::GetPostProcessor(const char *name) const
{
	if (!name)
		return NULL;
	for (unsigned int i = 0; i < postProcessors.GetSize(); i++)
	{
		if (strcmp(name, postProcessors[i]->GetName()) == 0)
			return postProcessors[i];
	}
	return NULL;
}

void DX11_Renderer::UpdateLights()
{
	for (unsigned int i = 0; i < lights.GetSize(); i++)
		lights[i]->Update();
}

void DX11_Renderer::SetupPostProcessSurface(DrawCmd &drawCmd)
{
	drawCmd.primitiveType = TRIANGLES_PRIMITIVE;
	drawCmd.numElements = 3;
	drawCmd.rasterizerState = noneCullRS;
	drawCmd.depthStencilState = noDepthTestDSS;
	drawCmd.blendState = defaultBS;
}

void DX11_Renderer::AddGpuCmd(const GpuCmd &gpuCmd)
{
	int index = gpuCmds.AddElements(1, &gpuCmd);
	gpuCmds[index].ID = index;
}

void DX11_Renderer::ClearFrame()
{
	gpuCmds.Clear();
	for (unsigned int i = 0; i < renderTargets.GetSize(); i++)
		renderTargets[i]->Reset();
}

void DX11_Renderer::ExecutePostProcessors()
{
	for (unsigned int i = 0; i < postProcessors.GetSize(); i++)
		postProcessors[i]->Execute();
}

void DX11_Renderer::SetDrawStates(const DrawCmd &cmd)
{
	if (cmd.rasterizerState != lastGpuCmd.draw.rasterizerState)
	{
		assert(cmd.rasterizerState != NULL);
		cmd.rasterizerState->Set();
		lastGpuCmd.draw.rasterizerState = cmd.rasterizerState;
	}

	if (cmd.depthStencilState != lastGpuCmd.draw.depthStencilState)
	{
		assert(cmd.depthStencilState != NULL);
		cmd.depthStencilState->Set();
		lastGpuCmd.draw.depthStencilState = cmd.depthStencilState;
	}

	if (cmd.blendState != lastGpuCmd.draw.blendState)
	{
		assert(cmd.blendState != NULL);
		cmd.blendState->Set();
		lastGpuCmd.draw.blendState = cmd.blendState;
	}

	if (cmd.vertexLayout != lastGpuCmd.draw.vertexLayout)
	{
		if (cmd.vertexLayout)
			cmd.vertexLayout->Bind();
		lastGpuCmd.draw.vertexLayout = cmd.vertexLayout;
	}

	if (cmd.vertexBuffer != lastGpuCmd.draw.vertexBuffer)
	{
		if (cmd.vertexBuffer)
			cmd.vertexBuffer->Bind();
		lastGpuCmd.draw.vertexBuffer = cmd.vertexBuffer;
	}

	if (cmd.indexBuffer != lastGpuCmd.draw.indexBuffer)
	{
		if (cmd.indexBuffer)
			cmd.indexBuffer->Bind();
		lastGpuCmd.draw.indexBuffer = cmd.indexBuffer;
	}
}

void DX11_Renderer::SetShaderStates(const ShaderCmd &cmd, gpuCmdOrders order)
{
	if ((cmd.renderTarget != lastGpuCmd.draw.renderTarget) || (cmd.renderTargetConfig != lastGpuCmd.draw.renderTargetConfig))
	{
		if (cmd.renderTarget)
			cmd.renderTarget->Bind(cmd.renderTargetConfig);
		lastGpuCmd.draw.renderTarget = cmd.renderTarget;
		lastGpuCmd.draw.renderTargetConfig = cmd.renderTargetConfig;
	}

	if (cmd.shader != lastGpuCmd.draw.shader)
	{
		assert(cmd.shader != NULL);
		cmd.shader->Bind();
		lastGpuCmd.draw.shader = cmd.shader;
	}

	if (cmd.light != lastGpuCmd.draw.light)
	{
		if (cmd.light && cmd.renderTarget)
		{
			if (order == SHADOW_CO)
				cmd.renderTarget->Clear(DEPTH_CLEAR_BIT);
		}
		lastGpuCmd.draw.light = cmd.light;
	}
}

void DX11_Renderer::SetShaderParams(const ShaderCmd &cmd)
{
	// set camera uniform-buffer
	if (cmd.camera)
		cmd.shader->SetUniformBuffer(CAMERA_UB_BP, cmd.camera->GetUniformBuffer());

	// set light uniform-buffer
	if (cmd.light)
		cmd.shader->SetUniformBuffer(LIGHT_UB_BP, cmd.light->GetUniformBuffer());

	// set custom uniform-buffers
	for (unsigned int i = 0; i < NUM_CUSTOM_UNIFORM_BUFFER_BP; i++)
	{
		if (cmd.customUBs[i])
			cmd.shader->SetUniformBuffer((uniformBufferBP)(CUSTOM0_UB_BP + i), cmd.customUBs[i]);
	}

	// set custom structured-buffers
	for (unsigned int i = 0; i < NUM_STRUCTURED_BUFFER_BP; i++)
	{
		if (cmd.customSBs[i])
			cmd.shader->SetStructuredBuffer((structuredBufferBP)(CUSTOM0_SB_BP + i), cmd.customSBs[i]);
	}

	for (unsigned int i = 0; i < NUM_TEXTURE_BP; i++)
	{
		if (cmd.textures[i])
			cmd.shader->SetTexture((textureBP)i, cmd.textures[i], cmd.samplers[i]);
	}
}

// compare-function passed to qsort
static int CompareGpuCmds(const void *a, const void *b)
{
	const GpuCmd *cA = (GpuCmd*)a;
	const GpuCmd *cB = (GpuCmd*)b;

	// interleave shadow map generation + direct illumination + illumination of voxel-grid
	if (((cA->order >= SHADOW_CO) && (cA->order <= GRID_ILLUM_CO)) &&
		((cB->order >= SHADOW_CO) && (cB->order <= GRID_ILLUM_CO)))
	{
		assert((cA->GetMode() == DRAW_CM) || (cB->GetMode() == COMPUTE_CM));
		const ILight *lightA = cA->draw.light;
		const ILight *lightB = cB->draw.light;
		assert((lightA != NULL) || (lightB != NULL));

		if (lightA->GetIndex() < lightB->GetIndex())
			return -1;
		else if (lightA->GetIndex() > lightB->GetIndex())
			return 1;
	}

	if (cA->order < cB->order)
		return -1;
	else if (cA->order > cB->order)
		return 1;
	if (cA->GetID() < cB->GetID())
		return -1;
	else if (cA->GetID() > cB->GetID())
		return 1;
	return 0;
}

void DX11_Renderer::ExecuteGpuCmds()
{
	ExecutePostProcessors();
	gpuCmds.Sort(CompareGpuCmds);
	for (unsigned int i = 0; i < gpuCmds.GetSize(); i++)
	{
		switch (gpuCmds[i].mode)
		{
		case DRAW_CM:
			SetDrawStates(gpuCmds[i].draw);
			SetShaderStates(gpuCmds[i].draw, gpuCmds[i].order);
			SetShaderParams(gpuCmds[i].draw);
			Draw(gpuCmds[i].draw);
			break;

		case COMPUTE_CM:
			SetShaderStates(gpuCmds[i].compute, gpuCmds[i].order);
			SetShaderParams(gpuCmds[i].compute);
			Dispatch(gpuCmds[i].compute);
			break;
		}
	}

	// draw tweak bars
	TwDraw();

	swapChain->Present(VSYNC_ENABLED, 0);
}

void DX11_Renderer::Draw(DrawCmd &cmd)
{
	deviceContext->IASetPrimitiveTopology((D3D11_PRIMITIVE_TOPOLOGY)cmd.primitiveType);
	if (cmd.indexBuffer != NULL)
	{
		if (cmd.numInstances < 2)
			deviceContext->DrawIndexed(cmd.numElements, cmd.firstIndex, 0);
		else
			deviceContext->DrawIndexedInstanced(cmd.numElements, cmd.numInstances, cmd.firstIndex, 0, 0);
	}
	else
	{
		if (cmd.numInstances < 2)
			deviceContext->Draw(cmd.numElements, cmd.firstIndex);
		else
			deviceContext->DrawInstanced(cmd.numElements, cmd.numInstances, cmd.firstIndex, 0);
	}
	UnbindShaderResources();
}

void DX11_Renderer::Dispatch(ComputeCmd &cmd)
{
	deviceContext->Dispatch(cmd.numThreadGroupsX, cmd.numThreadGroupsY, cmd.numThreadGroupsZ);
	UnbindShaderResources();

	ID3D11UnorderedAccessView *unorderedAccessViews[MAX_NUM_COLOR_BUFFERS] =
	{ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
	deviceContext->CSSetUnorderedAccessViews(0, MAX_NUM_COLOR_BUFFERS, unorderedAccessViews, NULL);
}

void DX11_Renderer::UnbindShaderResources()
{
	const unsigned int numViews = NUM_TEXTURE_BP + NUM_STRUCTURED_BUFFER_BP;
	ID3D11ShaderResourceView *shaderResourceViews[numViews] =
	{ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
	deviceContext->VSSetShaderResources(0, numViews, shaderResourceViews);
	deviceContext->GSSetShaderResources(0, numViews, shaderResourceViews);
	deviceContext->PSSetShaderResources(0, numViews, shaderResourceViews);
	deviceContext->CSSetShaderResources(0, numViews, shaderResourceViews);
}

void DX11_Renderer::SaveScreenshot() const
{
	// try to find a not existing path for screen-shot	
	char filePath[DEMO_MAX_FILEPATH];
	for (unsigned int i = 0; i < 1000; i++)
	{
		sprintf(filePath, "../Data/screenshots/screen%d.bmp", i);
		if (!Demo::fileManager->FilePathExists(filePath))
			break;
		if (i == 999)
			return;
	}

	// save content of back-buffer to bitmap file
	ID3D11Texture2D *backBufferTexture = NULL;
	if (swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBufferTexture) != S_OK)
		return;
	wchar_t wideFilePath[DEMO_MAX_FILEPATH];
	MultiByteToWideChar(CP_ACP, 0, filePath, DEMO_MAX_FILEPATH, wideFilePath, DEMO_MAX_FILEPATH);
	D3DX11SaveTextureToFileW(deviceContext, backBufferTexture, D3DX11_IFF_BMP, wideFilePath);
	SAFE_RELEASE(backBufferTexture);
}
