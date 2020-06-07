#include <stdafx.h>
#include <Demo.h>
#include <GlobalIllum.h>
#include <DemoMesh.h>

void DemoMesh::Release()
{
	SAFE_DELETE_PLIST(subMeshes);
}

bool DemoMesh::Load(const char *filename)
{
	// cache pointer to GlobalIllum post-processor
	globalIllumPP = (GlobalIllum*)Demo::renderer->GetPostProcessor("GlobalIllum");
	if (!globalIllumPP)
		return false;

	// load ".mesh" file
	strcpy(name, filename);
	char filePath[DEMO_MAX_FILEPATH];
	if (!Demo::fileManager->GetFilePath(filename, filePath))
		return false;
	FILE *file;
	fopen_s(&file, filePath, "rb");
	if (!file)
		return false;

	// check idString
	char idString[5];
	memset(idString, 0, 5);
	fread(idString, sizeof(char), 4, file);
	if (strcmp(idString, "DMSH") != 0)
	{
		fclose(file);
		return false;
	}

	// check version
	unsigned int version;
	fread(&version, sizeof(unsigned int), 1, file);
	if (version != CURRENT_DEMO_MESH_VERSION)
	{
		fclose(file);
		return false;
	}

	// get number of vertices
	unsigned int numVertices;
	fread(&numVertices, sizeof(unsigned int), 1, file);
	if (numVertices < 3)
	{
		fclose(file);
		return false;
	}

	// load vertices
	GeometryVertex *vertices = new GeometryVertex[numVertices];
	if (!vertices)
	{
		fclose(file);
		return false;
	}
	fread(vertices, sizeof(GeometryVertex), numVertices, file);

	//  create vertex layout for base pass
	VertexElementDesc vertexElementDescs[4] = { POSITION_ELEMENT, R32G32B32_FLOAT_EF, 0,
																					  TEXCOORDS_ELEMENT, R32G32_FLOAT_EF, 12,
																					  NORMAL_ELEMENT, R32G32B32_FLOAT_EF, 20,
																					  TANGENT_ELEMENT, R32G32B32A32_FLOAT_EF, 32 };
	baseVertexLayout = Demo::renderer->CreateVertexLayout(vertexElementDescs, 4);
	if (!baseVertexLayout)
		return false;

	// create vertex layout for voxel grid generation
	gridVertexLayout = Demo::renderer->CreateVertexLayout(vertexElementDescs, 3);
	if (!gridVertexLayout)
		return false;

	// create vertex layout for shadow map generation
	shadowVertexLayout = Demo::renderer->CreateVertexLayout(vertexElementDescs, 1);
	if (!shadowVertexLayout)
		return false;

	// create vertex buffer
	vertexBuffer = Demo::renderer->CreateVertexBuffer(sizeof(GeometryVertex), numVertices, false);
	if (!vertexBuffer)
	{
		SAFE_DELETE_ARRAY(vertices);
		fclose(file);
		return false;
	}
	vertexBuffer->AddVertices(numVertices, (float*)vertices);
	vertexBuffer->Update();
	SAFE_DELETE_ARRAY(vertices);

	// get number of indices
	unsigned int numIndices;
	fread(&numIndices, sizeof(unsigned int), 1, file);
	if (numIndices < 3)
	{
		fclose(file);
		return false;
	}

	// load indices 
	unsigned int *indices = new unsigned int[numIndices];
	if (!indices)
	{
		fclose(file);
		return false;
	}
	fread(indices, sizeof(unsigned int), numIndices, file);

	// create index-buffer
	indexBuffer = Demo::renderer->CreateIndexBuffer(numIndices, false);
	if (!indexBuffer)
	{
		SAFE_DELETE_ARRAY(indices);
		fclose(file);
		return false;
	}
	indexBuffer->AddIndices(numIndices, indices);
	indexBuffer->Update();
	SAFE_DELETE_ARRAY(indices);

	// get number of sub-meshes
	unsigned int numSubMeshes;
	fread(&numSubMeshes, sizeof(unsigned int), 1, file);
	if (numSubMeshes < 1)
	{
		fclose(file);
		return false;
	}

	// load/ create sub-meshes
	for (unsigned int i = 0; i < numSubMeshes; i++)
	{
		DemoSubmesh *subMesh = new DemoSubmesh;
		if (!subMesh)
		{
			fclose(file);
			return false;
		}
		char materialName[256];
		fread(materialName, sizeof(char), 256, file);
		subMesh->material = Demo::resourceManager->LoadMaterial(materialName);
		if (!subMesh->material)
		{
			fclose(file);
			SAFE_DELETE(subMesh);
			return false;
		}
		fread(&subMesh->firstIndex, sizeof(unsigned int), 1, file);
		fread(&subMesh->numIndices, sizeof(unsigned int), 1, file);
		subMeshes.AddElement(&subMesh);
	}

	fclose(file);

	uniformBuffer = Demo::renderer->CreateUniformBuffer(sizeof(Vector3));
	if (!uniformBuffer)
		return false;

	// render into albedoGloss and normal render-target of GBuffers
	RtConfigDesc rtcDesc;
	rtcDesc.numColorBuffers = 2;
	rtcDesc.firstColorBufferIndex = 1;
	multiRTC = Demo::renderer->CreateRenderTargetConfig(rtcDesc);
	if (!multiRTC)
		return false;

	Update();

	return true;
}

void DemoMesh::Update()
{
	if (!performUpdate)
		return;
	uniformBuffer->Update(position);
	performUpdate = false;
}

void DemoMesh::SetPosition(const Vector3 &position)
{
	if (this->position == position)
		return;
	this->position = position;
	performUpdate = true;
}

void DemoMesh::AddBaseSurfaces()
{
	for (unsigned int i = 0; i < subMeshes.GetSize(); i++)
	{
		GpuCmd gpuCmd(DRAW_CM);
		gpuCmd.order = BASE_CO;
		gpuCmd.draw.renderTarget = Demo::renderer->GetRenderTarget(GBUFFERS_RT_ID);
		gpuCmd.draw.renderTargetConfig = multiRTC;
		gpuCmd.draw.primitiveType = TRIANGLES_PRIMITIVE;
		gpuCmd.draw.camera = Demo::renderer->GetCamera(MAIN_CAMERA_ID);
		gpuCmd.draw.vertexLayout = baseVertexLayout;
		gpuCmd.draw.vertexBuffer = vertexBuffer;
		gpuCmd.draw.indexBuffer = indexBuffer;
		gpuCmd.draw.firstIndex = subMeshes[i]->firstIndex;
		gpuCmd.draw.numElements = subMeshes[i]->numIndices;
		gpuCmd.draw.textures[COLOR_TEX_ID] = subMeshes[i]->material->colorTexture;
		gpuCmd.draw.samplers[COLOR_TEX_ID] = Demo::renderer->GetSampler(TRILINEAR_SAMPLER_ID);
		gpuCmd.draw.textures[NORMAL_TEX_ID] = subMeshes[i]->material->normalTexture;
		gpuCmd.draw.textures[SPECULAR_TEX_ID] = subMeshes[i]->material->specularTexture;
		gpuCmd.draw.rasterizerState = subMeshes[i]->material->rasterizerState;
		gpuCmd.draw.depthStencilState = subMeshes[i]->material->depthStencilState;
		gpuCmd.draw.blendState = subMeshes[i]->material->blendState;
		gpuCmd.draw.customUBs[0] = uniformBuffer;
		gpuCmd.draw.shader = subMeshes[i]->material->shader;
		Demo::renderer->AddGpuCmd(gpuCmd);
	}
}

void DemoMesh::AddGridSurfaces()
{
	if (globalIllumPP->GetGlobalIllumMode() == DIRECT_ILLUM_ONLY_GIM)
		return;

	// add surfaces for generating voxel-grids
	for (unsigned int i = 0; i < 2; i++)
	{
		for (unsigned int j = 0; j < subMeshes.GetSize(); j++)
		{
			GpuCmd gpuCmd(DRAW_CM);
			gpuCmd.order = GRID_FILL_CO;
			gpuCmd.draw.vertexLayout = gridVertexLayout;
			gpuCmd.draw.vertexBuffer = vertexBuffer;
			gpuCmd.draw.indexBuffer = indexBuffer;
			gpuCmd.draw.primitiveType = TRIANGLES_PRIMITIVE;
			gpuCmd.draw.firstIndex = subMeshes[j]->firstIndex;
			gpuCmd.draw.numElements = subMeshes[j]->numIndices;
			gpuCmd.draw.textures[COLOR_TEX_ID] = subMeshes[j]->material->colorTexture;
			gpuCmd.draw.samplers[COLOR_TEX_ID] = Demo::renderer->GetSampler(TRILINEAR_SAMPLER_ID);
			gpuCmd.draw.customUBs[0] = uniformBuffer;
			globalIllumPP->SetupGridSurface(gpuCmd.draw, (gridTypes)i);
			Demo::renderer->AddGpuCmd(gpuCmd);
		}
	}
}

void DemoMesh::AddShadowMapSurfaces(unsigned int lightIndex)
{
	for (unsigned int i = 0; i < subMeshes.GetSize(); i++)
	{
		GpuCmd gpuCmd(DRAW_CM);
		gpuCmd.order = SHADOW_CO;
		gpuCmd.draw.vertexLayout = shadowVertexLayout;
		gpuCmd.draw.vertexBuffer = vertexBuffer;
		gpuCmd.draw.indexBuffer = indexBuffer;
		gpuCmd.draw.primitiveType = TRIANGLES_PRIMITIVE;
		gpuCmd.draw.firstIndex = subMeshes[i]->firstIndex;
		gpuCmd.draw.numElements = subMeshes[i]->numIndices;
		gpuCmd.draw.customUBs[0] = uniformBuffer;
		Demo::renderer->GetLight(lightIndex)->SetupShadowMapSurface(gpuCmd.draw);
		Demo::renderer->AddGpuCmd(gpuCmd);
	}
}

void DemoMesh::AddSurfaces()
{
	if (!active)
		return;
	Update();
	AddBaseSurfaces();
	AddGridSurfaces();
	for (unsigned int i = 0; i < Demo::renderer->GetNumLights(); i++)
	{
		if ((!Demo::renderer->GetLight(i)->IsActive()) || (!Demo::renderer->GetLight(i)->HasShadow()))
			continue;
		AddShadowMapSurfaces(i);
	}
}
