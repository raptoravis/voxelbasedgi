#include <stdafx.h>
#include <Demo.h>
#include <GlobalIllum.h>

bool GlobalIllum::Create()
{
	// commonly used objects
	{
		sceneRT = Demo::renderer->GetRenderTarget(GBUFFERS_RT_ID);
		if (!sceneRT)
			return false;

		RenderTargetDesc rtDesc;
		rtDesc.depth = 0;
		computeRT = Demo::renderer->CreateRenderTarget(rtDesc);
		if (!computeRT)
			return false;

		BlendDesc blendDesc;
		blendDesc.blend = true;
		blendBS = Demo::renderer->CreateBlendState(blendDesc);
		if (!blendBS)
			return false;

		gridUniformBuffer = Demo::renderer->CreateUniformBuffer(sizeof(BufferData));
		if (!gridUniformBuffer)
			return false;

		gridHalfExtents[FINE_GRID] = 1000.0f;
		gridHalfExtents[COARSE_GRID] = 1600.0f;

		bufferData.gridCellSizes.x = gridHalfExtents[FINE_GRID] / 16.0f;
		bufferData.gridCellSizes.y = 1.0f / bufferData.gridCellSizes.x;
		bufferData.gridCellSizes.z = gridHalfExtents[COARSE_GRID] / 16.0f;
		bufferData.gridCellSizes.w = 1.0f / bufferData.gridCellSizes.z;

		// for generating the voxel-grids, the scene geometry is rendered with orthographic projection
		for (unsigned int i = 0; i < 2; i++)
		{
			gridProjMatrices[i].SetOrtho(-gridHalfExtents[i], gridHalfExtents[i], -gridHalfExtents[i],
				gridHalfExtents[i], 0.0f, 2.0f*gridHalfExtents[i]);
		}
	}

	// objects used for generating the voxel-grids
	{
		RenderTargetDesc rtDesc;
		rtDesc.width = 64;
		rtDesc.height = 64;
		rtDesc.colorBufferDescs[0].format = TEX_FORMAT_I8;
		gridRT = Demo::renderer->CreateRenderTarget(rtDesc);
		if (!gridRT)
			return false;

		gridSBs[FINE_GRID] = Demo::renderer->CreateStructuredBuffer(32 * 32 * 32, 5 * sizeof(float));
		if (!gridSBs[FINE_GRID])
			return false;
		gridSBs[COARSE_GRID] = Demo::renderer->CreateStructuredBuffer(32 * 32 * 32, 5 * sizeof(float));
		if (!gridSBs[COARSE_GRID])
			return false;

		RtConfigDesc desc;
		desc.numUnorderedAccessViews = 1;
		desc.unorderedAccessViews[0] = gridSBs[FINE_GRID]->GetUnorderdAccessView();
		gridRTCs[FINE_GRID] = Demo::renderer->CreateRenderTargetConfig(desc);
		if (!gridRTCs[FINE_GRID])
			return false;

		desc.unorderedAccessViews[0] = gridSBs[COARSE_GRID]->GetUnorderdAccessView();
		gridRTCs[COARSE_GRID] = Demo::renderer->CreateRenderTargetConfig(desc);
		if (!gridRTCs[COARSE_GRID])
			return false;

		gridFillShaders[FINE_GRID] = Demo::resourceManager->LoadShader("shaders/gridFill.sdr", 1); // (Permutation 1 = FINE_GRID)
		if (!gridFillShaders[0])
			return false;

		gridFillShaders[COARSE_GRID] = Demo::resourceManager->LoadShader("shaders/gridFill.sdr");
		if (!gridFillShaders[1])
			return false;

		RasterizerDesc rasterDesc;
		gridRS = Demo::renderer->CreateRasterizerState(rasterDesc);
		if (!gridRS)
			return false;

		// disable depth-write and depth-test, in order to fully "voxelize" scene geometry 
		DepthStencilDesc depthStencilDesc;
		depthStencilDesc.depthTest = false;
		depthStencilDesc.depthMask = false;
		gridDSS = Demo::renderer->CreateDepthStencilState(depthStencilDesc);
		if (!gridDSS)
			return false;

		// disable color-write since instead of outputting the rasterized voxel information into the bound render-target,
		// it will be written into a 3D structured buffer/ texture 
		BlendDesc blendDesc;
		blendDesc.colorMask = 0;
		gridBS = Demo::renderer->CreateBlendState(blendDesc);
		if (!gridBS)
			return false;
	}

	// objects used for illuminating the voxel-grids
	{
		// Create for FINE_GRID/ COARSE_GRID a 32x32x32 2D texture array with 3 attached render-targets (one for each second order spherical
		// harmonics coefficients for each color channel). Since this render-targets will be further used for the light propagation step,
		// and the results of the last frame are required, for each grid resolution three MRTs are required.  
		for (unsigned int i = 0; i < 3; i++)
		{
			RenderTargetDesc rtDesc;
			rtDesc.width = 32;
			rtDesc.height = 32;
			rtDesc.depth = 32;
			for (unsigned int j = 0; j < 3; j++)
			{
				rtDesc.colorBufferDescs[j].format = TEX_FORMAT_RGBA16F;
				rtDesc.colorBufferDescs[j].rtFlags = UAV_RTF;
			}
			lightRTs[FINE_GRID][i] = Demo::renderer->CreateRenderTarget(rtDesc);
			if (!lightRTs[FINE_GRID][i])
				return false;
			lightRTs[COARSE_GRID][i] = Demo::renderer->CreateRenderTarget(rtDesc);
			if (!lightRTs[COARSE_GRID][i])
				return false;
		}
	}

	// objects used for the light propagation
	{
		// configure corresponding render-target, to perform light propagation in the compute shader
		RtConfigDesc desc;
		desc.numColorBuffers = 3;
		desc.flags = COMPUTE_RTCF;
		lightPropagateRTC = Demo::renderer->CreateRenderTargetConfig(desc);
		if (!lightPropagateRTC)
			return false;

		lightPropagateShaders[0] = Demo::resourceManager->LoadShader("shaders/lightPropagate.sdr");
		if (!lightPropagateShaders[0])
			return false;
		lightPropagateShaders[1] = Demo::resourceManager->LoadShader("shaders/lightPropagate.sdr", 1); // (Permutation 1 = USE_OCCLUSION)
		if (!lightPropagateShaders[1])
			return false;
	}

	// objects used for interpolating light propagation results from current and last frame
	{
		gridSmoothShaders[FINE_GRID] = Demo::resourceManager->LoadShader("shaders/gridSmooth.sdr", 1); // (Permutation 1 = FINE_GRID)
		if (!gridSmoothShaders[FINE_GRID])
			return false;
		gridSmoothShaders[COARSE_GRID] = Demo::resourceManager->LoadShader("shaders/gridSmooth.sdr");
		if (!gridSmoothShaders[COARSE_GRID])
			return false;
	}

	// objects used for generating the indirect illumination
	{
		// only render into the accumulation render-target of the GBuffers
		RtConfigDesc desc;
		desc.numColorBuffers = 1;
		desc.flags = DS_READ_ONLY_RTCF;
		outputRTC = Demo::renderer->CreateRenderTargetConfig(desc);
		if (!outputRTC)
			return false;

		globalIllumShaders[0] = Demo::resourceManager->LoadShader("shaders/globalIllum.sdr");
		if (!globalIllumShaders[0])
			return false;
		globalIllumShaders[1] = Demo::resourceManager->LoadShader("shaders/globalIllum.sdr", 1); // (Permutation 1 = TEXTURE)
		if (!globalIllumShaders[1])
			return false;

		// only illuminate actual scene geometry, not sky
		DepthStencilDesc depthStencilDesc;
		depthStencilDesc.stencilTest = true;
		depthStencilDesc.stencilRef = 1;
		depthStencilDesc.stencilPassOp = KEEP_STENCIL_OP;
		stencilTestDSS = Demo::renderer->CreateDepthStencilState(depthStencilDesc);
		if (!stencilTestDSS)
			return false;
	}

	// objects used for visualizing the voxel-grids
	{
		gridVisShader = Demo::resourceManager->LoadShader("shaders/gridVis.sdr");
		if (!gridVisShader)
			return false;
	}

	// objects used for clearing the voxel-grids
	{
		// use compute shader for clearing the voxel-grids
		RtConfigDesc desc;
		desc.numUnorderedAccessViews = 2;
		desc.unorderedAccessViews[0] = gridSBs[FINE_GRID]->GetUnorderdAccessView();
		desc.unorderedAccessViews[1] = gridSBs[COARSE_GRID]->GetUnorderdAccessView();
		desc.flags = COMPUTE_RTCF;
		clearRTC = Demo::renderer->CreateRenderTargetConfig(desc);
		if (!clearRTC)
			return false;

		clearShader = Demo::resourceManager->LoadShader("shaders/gridClear.sdr");
		if (!clearShader)
			return false;
	}

	Update();

	return true;
}

void GlobalIllum::Update()
{
	// update data for FINE_GRID/ COARSE_GRID
	for (unsigned int i = 0; i < 2; i++)
		UpdateGrid(i);

	// update uniform-buffer
	gridUniformBuffer->Update(&bufferData);

	// Only allow smoothing after demo has already run few frames since otherwise initial buildup of indirect
	// illumination would be visible.
	static unsigned int counter = 0;
	if (counter < 10)
		counter++;
	else
		allowSmoothing = true;
}

void GlobalIllum::UpdateGrid(unsigned int gridType)
{
	const Camera *camera = Demo::renderer->GetCamera(MAIN_CAMERA_ID);

	// save snapped grid positions from last frame 
	bufferData.lastSnappedGridPositions[gridType] = bufferData.snappedGridPositions[gridType];
	bufferData.lastSnappedGridPositions[gridType].w = (float)Demo::timeManager->GetFrameInterval();

	// calculate center of grid
	Vector3 tmpGridPosition;
	if (gridType == FINE_GRID)
		tmpGridPosition = camera->GetPosition() + (camera->GetDirection()*0.5f*gridHalfExtents[FINE_GRID]);
	else
		tmpGridPosition = camera->GetPosition() + (camera->GetDirection()*(gridHalfExtents[COARSE_GRID] - 0.5f*gridHalfExtents[FINE_GRID]));
	bufferData.gridPositions[gridType].Set(tmpGridPosition);

	// calculate center of grid that is snapped to the grid-cell extents
	Vector3 tmpSnappedGridPosition;
	tmpSnappedGridPosition = tmpGridPosition;
	tmpSnappedGridPosition *= bufferData.gridCellSizes[gridType * 2 + 1];
	tmpSnappedGridPosition.Floor();
	tmpSnappedGridPosition *= bufferData.gridCellSizes[gridType * 2];
	bufferData.snappedGridPositions[gridType].Set(tmpSnappedGridPosition);

	// back to front viewProjMatrix
	{
		Vector3 translate = -tmpSnappedGridPosition - Vector3(0.0f, 0.0f, gridHalfExtents[gridType]);
		Matrix4 gridTransMatrix;
		gridTransMatrix.SetTranslation(translate);
		bufferData.gridViewProjMatrices[gridType * 3] = gridProjMatrices[gridType] * gridTransMatrix;
	}

	// right to left viewProjMatrix
	{
		Vector3 translate = -tmpSnappedGridPosition - Vector3(gridHalfExtents[gridType], 0.0f, 0.0f);
		Matrix4 gridTransMatrix;
		gridTransMatrix.SetTranslation(translate);
		Matrix4 gridXRotMatrix;
		gridXRotMatrix.SetRotationY(-90.0f);
		Matrix4 gridViewMatrix;
		gridViewMatrix = gridXRotMatrix * gridTransMatrix;
		bufferData.gridViewProjMatrices[gridType * 3 + 1] = gridProjMatrices[gridType] * gridViewMatrix;
	}

	// top to down viewProjMatrix
	{
		Vector3 translate = -tmpSnappedGridPosition - Vector3(0.0f, gridHalfExtents[gridType], 0.0f);
		Matrix4 gridTransMatrix;
		gridTransMatrix.SetTranslation(translate);
		Matrix4 gridYRotMatrix;
		gridYRotMatrix.SetRotationX(90.0f);
		Matrix4 gridViewMatrix;
		gridViewMatrix = gridYRotMatrix * gridTransMatrix;
		bufferData.gridViewProjMatrices[gridType * 3 + 2] = gridProjMatrices[gridType] * gridViewMatrix;
	}
}

void GlobalIllum::SwapIndices(unsigned int &indexA, unsigned int &indexB) const
{
	unsigned int tmpIndex = indexA;
	indexA = indexB;
	indexB = tmpIndex;
}

void GlobalIllum::PerformGridLightingPass()
{
	// let each active light illuminate FINE_GRID/ COARSE_GRID
	for (unsigned int i = 0; i < Demo::renderer->GetNumLights(); i++)
		Demo::renderer->GetLight(i)->AddGridSurfaces();

	SwapIndices(inputLightRTIndices[FINE_GRID], outputLightRTIndices[FINE_GRID]);
	SwapIndices(inputLightRTIndices[COARSE_GRID], outputLightRTIndices[COARSE_GRID]);
}

void GlobalIllum::PerformLightPropagatePass(unsigned int index, gridTypes gridType)
{
	// Propagate virtual point lights to 6 neighbor grid-cells in the compute shader. In the 
	// first iteration no occlusion is used, in order to initially let the light distribute. 
	// From the second iteration on we use the geometry occlusion, in order to avoid light leaking. 
	GpuCmd gpuCmd(COMPUTE_CM);
	gpuCmd.order = GLOBAL_ILLUM_CO;
	gpuCmd.compute.renderTarget = lightRTs[gridType][outputLightRTIndices[gridType]];
	gpuCmd.compute.renderTargetConfig = lightPropagateRTC;
	if ((useOcclusion) && (index > 0))
	{
		gpuCmd.compute.shader = lightPropagateShaders[1];
		gpuCmd.compute.customSBs[0] = gridSBs[gridType];
	}
	else
	{
		gpuCmd.compute.shader = lightPropagateShaders[0];
	}
	gpuCmd.compute.textures[CUSTOM0_TEX_ID] = lightRTs[gridType][inputLightRTIndices[gridType]]->GetTexture();
	gpuCmd.compute.textures[CUSTOM1_TEX_ID] = lightRTs[gridType][inputLightRTIndices[gridType]]->GetTexture(1);
	gpuCmd.compute.textures[CUSTOM2_TEX_ID] = lightRTs[gridType][inputLightRTIndices[gridType]]->GetTexture(2);
	gpuCmd.compute.numThreadGroupsX = 4;
	gpuCmd.compute.numThreadGroupsY = 4;
	gpuCmd.compute.numThreadGroupsZ = 4;
	Demo::renderer->AddGpuCmd(gpuCmd);

	SwapIndices(inputLightRTIndices[gridType], outputLightRTIndices[gridType]);
}

void GlobalIllum::PerformGridSmoothingPass(gridTypes gridType)
{
	// Due to the discrete voxel-grid representation of the scene, rapidly moving objects, which contribute to the
	// indirect illumination, can cause flickering. To prevent such flickering the light propagation results of 
	// the current frame can be interpolated with those of the last frame. This can be done by first determining 
	// the world space position of each cell in the current virtual point light grid; the corresponding cell with 
	// the same world space position is looked up in the virtual point light grid of the last frame. In this way 
	// the SH-coefficients of both cells can be linearly interpolated, which results in a smooth transition for even
	// rapidly moving objects. For cells which are not anymore within the bounds of the grid from the last frame 
	// simply the SH-coefficients of the current grid are used.
	GpuCmd gpuCmd(COMPUTE_CM);
	gpuCmd.order = GLOBAL_ILLUM_CO;
	gpuCmd.compute.renderTarget = lightRTs[gridType][outputLightRTIndices[gridType]];
	gpuCmd.compute.renderTargetConfig = lightPropagateRTC;
	gpuCmd.compute.shader = gridSmoothShaders[gridType];
	gpuCmd.compute.textures[CUSTOM0_TEX_ID] = lightRTs[gridType][inputLightRTIndices[gridType]]->GetTexture();
	gpuCmd.compute.textures[CUSTOM1_TEX_ID] = lightRTs[gridType][inputLightRTIndices[gridType]]->GetTexture(1);
	gpuCmd.compute.textures[CUSTOM2_TEX_ID] = lightRTs[gridType][inputLightRTIndices[gridType]]->GetTexture(2);
	gpuCmd.compute.textures[CUSTOM3_TEX_ID] = lightRTs[gridType][lastInputLightRTIndices[gridType]]->GetTexture();
	gpuCmd.compute.textures[CUSTOM4_TEX_ID] = lightRTs[gridType][lastInputLightRTIndices[gridType]]->GetTexture(1);
	gpuCmd.compute.textures[CUSTOM5_TEX_ID] = lightRTs[gridType][lastInputLightRTIndices[gridType]]->GetTexture(2);
	gpuCmd.compute.customUBs[0] = gridUniformBuffer;
	gpuCmd.compute.numThreadGroupsX = 4;
	gpuCmd.compute.numThreadGroupsY = 4;
	gpuCmd.compute.numThreadGroupsZ = 4;
	Demo::renderer->AddGpuCmd(gpuCmd);

	SwapIndices(inputLightRTIndices[gridType], outputLightRTIndices[gridType]);
}

void GlobalIllum::PerformGlobalIllumPass()
{
	// Use normal-/ depth buffer (of GBuffers) to perform actual indirect illumination of each visible pixel.
	// By using the stencil buffer, we prevent that the sky is illuminated, too.
	GpuCmd gpuCmd(DRAW_CM);
	gpuCmd.order = GLOBAL_ILLUM_CO;
	gpuCmd.draw.renderTarget = sceneRT;
	gpuCmd.draw.renderTargetConfig = outputRTC;
	gpuCmd.draw.camera = Demo::renderer->GetCamera(MAIN_CAMERA_ID);
	gpuCmd.draw.textures[NORMAL_TEX_ID] = sceneRT->GetTexture(2); // normal
	gpuCmd.draw.textures[SPECULAR_TEX_ID] = sceneRT->GetDepthStencilTexture(); // depth
	gpuCmd.draw.textures[CUSTOM0_TEX_ID] = lightRTs[FINE_GRID][inputLightRTIndices[FINE_GRID]]->GetTexture();
	gpuCmd.draw.samplers[CUSTOM0_TEX_ID] = Demo::renderer->GetSampler(LINEAR_SAMPLER_ID);
	gpuCmd.draw.textures[CUSTOM1_TEX_ID] = lightRTs[FINE_GRID][inputLightRTIndices[FINE_GRID]]->GetTexture(1);
	gpuCmd.draw.textures[CUSTOM2_TEX_ID] = lightRTs[FINE_GRID][inputLightRTIndices[FINE_GRID]]->GetTexture(2);
	gpuCmd.draw.textures[CUSTOM3_TEX_ID] = lightRTs[COARSE_GRID][inputLightRTIndices[COARSE_GRID]]->GetTexture();
	gpuCmd.draw.textures[CUSTOM4_TEX_ID] = lightRTs[COARSE_GRID][inputLightRTIndices[COARSE_GRID]]->GetTexture(1);
	gpuCmd.draw.textures[CUSTOM5_TEX_ID] = lightRTs[COARSE_GRID][inputLightRTIndices[COARSE_GRID]]->GetTexture(2);
	gpuCmd.draw.customUBs[0] = gridUniformBuffer;
	Demo::renderer->SetupPostProcessSurface(gpuCmd.draw);
	gpuCmd.draw.depthStencilState = stencilTestDSS;
	if (mode == DEFAULT_GIM)
	{
		gpuCmd.draw.textures[COLOR_TEX_ID] = sceneRT->GetTexture(1); // albedoGloss
		gpuCmd.draw.blendState = blendBS;
		gpuCmd.draw.shader = globalIllumShaders[1];
	}
	else
	{
		gpuCmd.draw.shader = globalIllumShaders[0];
	}
	Demo::renderer->AddGpuCmd(gpuCmd);
}

void GlobalIllum::PerformGridVisPass()
{
	// Use depth buffer (of GBuffers) to reconstruct position of the pixels and extract/ visualize 
	  // the corresponding voxels.
	GpuCmd gpuCmd(DRAW_CM);
	gpuCmd.order = GLOBAL_ILLUM_CO;
	gpuCmd.draw.renderTarget = sceneRT;
	gpuCmd.draw.renderTargetConfig = outputRTC;
	gpuCmd.draw.camera = Demo::renderer->GetCamera(MAIN_CAMERA_ID);
	gpuCmd.draw.textures[COLOR_TEX_ID] = sceneRT->GetDepthStencilTexture(); // depth
	gpuCmd.draw.customUBs[0] = gridUniformBuffer;
	gpuCmd.draw.customSBs[0] = gridSBs[FINE_GRID];
	gpuCmd.draw.customSBs[1] = gridSBs[COARSE_GRID];
	gpuCmd.draw.shader = gridVisShader;
	Demo::renderer->SetupPostProcessSurface(gpuCmd.draw);
	gpuCmd.draw.depthStencilState = stencilTestDSS;
	Demo::renderer->AddGpuCmd(gpuCmd);
}

void GlobalIllum::PerformClearPass()
{
	// Clear voxel-grids in compute shader.
	GpuCmd gpuCmd(COMPUTE_CM);
	gpuCmd.order = GLOBAL_ILLUM_CO;
	gpuCmd.compute.renderTarget = computeRT;
	gpuCmd.compute.renderTargetConfig = clearRTC;
	gpuCmd.compute.shader = clearShader;
	gpuCmd.compute.numThreadGroupsX = 4;
	gpuCmd.compute.numThreadGroupsY = 4;
	gpuCmd.compute.numThreadGroupsZ = 4;
	Demo::renderer->AddGpuCmd(gpuCmd);
}

void GlobalIllum::Execute()
{
	if (!active)
		return;

	if (mode != DIRECT_ILLUM_ONLY_GIM)
	{
		Update();
	}

	if ((mode == DEFAULT_GIM) || (mode == INDIRECT_ILLUM_ONLY_GIM))
	{
		PerformGridLightingPass();
		for (unsigned int i = 0; i < numPropagationSteps; i++)
			PerformLightPropagatePass(i, FINE_GRID);
		for (unsigned int i = 0; i < numPropagationSteps; i++)
			PerformLightPropagatePass(i, COARSE_GRID);
		if (allowSmoothing && useSmoothing)
		{
			PerformGridSmoothingPass(FINE_GRID);
			PerformGridSmoothingPass(COARSE_GRID);
		}
		PerformGlobalIllumPass();
	}
	else if (mode == VISUALIZE_GRIDS_GIM)
	{
		PerformGridVisPass();
	}

	if (mode != DIRECT_ILLUM_ONLY_GIM)
	{
		PerformClearPass();
	}

	SwapIndices(inputLightRTIndices[FINE_GRID], lastInputLightRTIndices[FINE_GRID]);
	SwapIndices(inputLightRTIndices[COARSE_GRID], lastInputLightRTIndices[COARSE_GRID]);
}

void GlobalIllum::SetupGridSurface(DrawCmd &drawCmd, gridTypes gridType)
{
	// Setup passed scene geometry surface to generate the voxel-grid. Actually for each grid
	// the scene geometry is rendered in 3 orthographic views without depth-testing into a 64x64/ 
  // 512x512 render-target. Instead of writing out the results in the fragment shader into the 
  // render-target, they are written into a 3D structured buffer/ texture.
	drawCmd.renderTarget = gridRT;
	drawCmd.renderTargetConfig = gridRTCs[gridType];
	drawCmd.rasterizerState = gridRS;
	drawCmd.depthStencilState = gridDSS;
	drawCmd.blendState = gridBS;
	drawCmd.customUBs[1] = gridUniformBuffer;
	drawCmd.shader = gridFillShaders[gridType];
}

void GlobalIllum::SetupLightGridSurface(DrawCmd &drawCmd, gridTypes gridType)
{
	// Setup passed surface to illuminate the FINE_GRID/ COARSE_GRID. This is done by rendering for each
	// light a full-screen quad (32x32 pixels) with 32 instances into the corresponding 32x32x32 2D texture 
  // array. By using additive hardware blending the results are combined.
	drawCmd.renderTarget = lightRTs[gridType][outputLightRTIndices[gridType]];
	drawCmd.customUBs[0] = gridUniformBuffer;
	drawCmd.customSBs[0] = gridSBs[gridType];
	drawCmd.numInstances = 32;
	Demo::renderer->SetupPostProcessSurface(drawCmd);
	drawCmd.blendState = blendBS;
}



