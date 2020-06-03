#ifndef GLOBAL_ILLUM_H
#define GLOBAL_ILLUM_H

#include <GpuCmd.h>
#include <IPostProcessor.h>

class DX11_RenderTarget;
class DX11_RenderTargetConfig;
class DX11_UniformBuffer;
class DX11_StructuredBuffer;
class DX11_Shader;
class DX11_RasterizerState;
class DX11_DepthStencilState;
class DX11_BlendState;	

enum gridTypes
{
	FINE_GRID=0, // fine resolution grid
	COARSE_GRID // coarse resolution grid
};

enum globalIllumModes
{
  DIRECT_ILLUM_ONLY_GIM=0, 
  INDIRECT_ILLUM_ONLY_GIM,
  DEFAULT_GIM,                   
  VISUALIZE_GRIDS_GIM    
};

// GlobalIllum
//
// This post-processor performs "Rasterized Voxel-based Dynamic Global Illumination".
// Actually the technique can be divided into 6 steps:
// 1. In an initial step a voxel-representation of the scene geometry is created by 
//    utilizing the hardware rasterizer.
// 2. For each light the voxel-grid is illuminated and from the illuminated voxels virtual 
//    point lights (represented as spherical harmonics) are created.
// 3. The virtual point lights are propagated iteratively until the required light distribution
//    is achieved. Thereby the previously generated voxel-grid is utilized to perform geometric
//    occlusion for the propagated light.
// 4. The light propagation results from current frame are linearly interpolated with those of the
//    previous frame, in order to prevent flickering caused by rapidly moving objects.
// 5. With the help of the normal-/ depth-buffer (of the GBuffers) each visible pixel is illuminated,
//    thereby generating the actual indirect illumination.
// 6. Finally the voxel grid is cleared.
// The voxel- as well as the virtual point light grid are consisting of 32x32x32 cells. In order to
// cover the entire scene, therefore two cascades are used, a fine and a coarse resolution grid.
class GlobalIllum: public IPostProcessor
{
public: 
  struct BufferData
  {
    Matrix4 gridViewProjMatrices[6];     // viewProjMatrices for generating the voxel-grids
    Vector4 gridCellSizes;               // (inverse) sizes of grid-cells (FINE_GRID/ COARSE_GRID)
    Vector4 gridPositions[2];            // center of FINE_GRID/ COARSE_GRID
    Vector4 snappedGridPositions[2];     // center of FINE_GRID/ COARSE_GRID, snapped to the corresponding grid-cell extents 
    Vector4 lastSnappedGridPositions[2]; // snapped grid positions of FINE_GRID/ COARSE_GRID from last frame (w-component = frame-interval)
    Vector4 globalIllumParams;           // x = flux amplifier, y = occlusion amplifier, z = diffuse GI-contribution power
  };

	GlobalIllum()
	{
		strcpy(name, "GlobalIllum");
	
    sceneRT = NULL;
    computeRT = NULL;
	  blendBS = NULL;
		gridUniformBuffer = NULL;
	
    gridRT = NULL;	
    gridSBs[FINE_GRID] = NULL;
    gridSBs[COARSE_GRID] = NULL;
    gridRTCs[FINE_GRID] = NULL;
    gridRTCs[COARSE_GRID] = NULL;
	  gridFillShaders[FINE_GRID] = NULL;
    gridFillShaders[COARSE_GRID] = NULL;
		gridRS = NULL;
		gridDSS = NULL;
		gridBS = NULL;	

		for(unsigned int i=0; i<3; i++)
		{
			lightRTs[FINE_GRID][i] = NULL; 
			lightRTs[COARSE_GRID][i] = NULL; 
		}
		inputLightRTIndices[FINE_GRID] = 0;
		outputLightRTIndices[FINE_GRID] = 1;
		lastInputLightRTIndices[FINE_GRID] = 2;
		inputLightRTIndices[COARSE_GRID] = 0;
		outputLightRTIndices[COARSE_GRID] = 1;
		lastInputLightRTIndices[COARSE_GRID] = 2;

		lightPropagateRTC = NULL;
		for(unsigned int i=0; i<2; i++)
			lightPropagateShaders[i] = NULL;

		gridSmoothShaders[FINE_GRID] = NULL;
		gridSmoothShaders[COARSE_GRID] = NULL;

		outputRTC = NULL;
	  for(unsigned int i=0; i<4; i++)
      globalIllumShaders[i] = NULL;
		stencilTestDSS = NULL;

		gridVisShader = NULL;

		clearRTC = NULL;
		clearShader = NULL;

		allowSmoothing = false;

		Reset();
	}

	virtual bool Create() override;

	virtual DX11_RenderTarget* GetOutputRT() const override
	{
		return sceneRT;
	}

	virtual void Execute() override;

	// configures surface for generating the voxel-grid
	void SetupGridSurface(DrawCmd &drawCmd, gridTypes gridType);

	// configures surface for illuminating the voxel-grid
	void SetupLightGridSurface(DrawCmd &drawCmd, gridTypes gridType);

	void Reset()
	{
		mode = DEFAULT_GIM;
		useOcclusion = true;
		useSmoothing = true;
		numPropagationSteps = 10;
    bufferData.globalIllumParams.Set(2.55f, 1.25f, 0.45f, 0.0f);
	}

	void SetGlobalIllumMode(globalIllumModes mode)
	{
		this->mode = mode;
	}	

  globalIllumModes GetGlobalIllumMode() const 
  {
    return mode;
  }

	void SetFluxAmplifier(float fluxAmplifier)
	{
		bufferData.globalIllumParams.x = fluxAmplifier;
		CLAMP(bufferData.globalIllumParams.x, 0.1f, 10.0f);
	}

	float GetFluxAmplifier() const
	{
		return bufferData.globalIllumParams.x;
	}

	void SetOcclusionAmplifier(float occlusionAmplifier)
	{
		bufferData.globalIllumParams.y = occlusionAmplifier;
		CLAMP(bufferData.globalIllumParams.y, 0.1f, 10.0f);
	}

	float GetOcclusionAmplifier() const
	{
		return bufferData.globalIllumParams.y;
	}

	void SetGlobalIllumPower(float globalIllumPower)
	{
		bufferData.globalIllumParams.z = globalIllumPower;
		CLAMP(bufferData.globalIllumParams.z, 0.1f, 1.0f);
	}

	float GetGlobalIllumPower() const
	{
		return bufferData.globalIllumParams.z;
	}

	void SetNumPropagationSteps(unsigned int numPropagationSteps)
	{
    this->numPropagationSteps = numPropagationSteps;
		CLAMP(this->numPropagationSteps, 1, 40);
	}

	unsigned int GetNumPropagationSteps() const
	{
		return numPropagationSteps;
	}
	
	void EnableOcclusion(bool enable)
	{
		useOcclusion = enable;
	}

	bool IsOcclusionEnabled() const
	{
		return useOcclusion;
	}

	void EnableSmoothing(bool enable)
	{
		useSmoothing = enable;
	}

	bool IsSmoothingEnabled() const
	{
		return useSmoothing;
	}

private:
	void Update();

  void UpdateGrid(unsigned int gridType);

	void SwapIndices(unsigned int &indexA, unsigned int &indexB) const;

	// performs illumination of the voxel-grids
	void PerformGridLightingPass();
  
  // performs propagation of virtual point lights
	void PerformLightPropagatePass(unsigned int index, gridTypes gridType);

	// performs linear interpolation between light propagation results of current and last frame
	void PerformGridSmoothingPass(gridTypes gridType);

	// performs indirect illumination 
	void PerformGlobalIllumPass();	

	// visualizes voxel-grids
	void PerformGridVisPass();

	// clears voxel-grids  
  void PerformClearPass();
	
	// commonly used objects
  DX11_RenderTarget *sceneRT;                 // GBuffers
  DX11_RenderTarget *computeRT;               // empty render-target used for compute shaders 
  DX11_BlendState *blendBS;                   // use additive blending
  DX11_UniformBuffer *gridUniformBuffer;      // uniform buffer with information about the grids  

  // objects used for generating the voxel-grids
  DX11_RenderTarget *gridRT;                  // simple 64x64 R8 render-target 
  DX11_StructuredBuffer *gridSBs[2];          // structured buffers for FINE_GRID/ COARSE_GRID
  DX11_RenderTargetConfig *gridRTCs[2];       // render-target configs for FINE_GRID/ COARSE_GRID
  DX11_Shader *gridFillShaders[2];            // shaders for FINE_GRID/ COARSE_GRID
  DX11_RasterizerState *gridRS;               // default rasterizer state (no culling, solid mode)
  DX11_DepthStencilState *gridDSS;            // no depth-write/ -test depth-stencil state
  DX11_BlendState *gridBS;	                  // default blend state (blending disabled)

	// objects used for illuminating the voxel-grids
	DX11_RenderTarget *lightRTs[2][3];	        // three 32x32x32 RGBA16F render-targets for each FINE_GRID/ COARSE_GRID
	unsigned int inputLightRTIndices[2];        // keep track of currently set input render-target for FINE_GRID/ COARSE_GRID
	unsigned int outputLightRTIndices[2];       // keep track of currently set output render-target for FINE_GRID/ COARSE_GRID   
	unsigned int lastInputLightRTIndices[2];    // keep track of input render-target for FINE_GRID/ COARSE_GRID that was last set in last frame

	// objects used for the light propagation
	DX11_RenderTargetConfig *lightPropagateRTC; // render-target config for light propagation
	DX11_Shader *lightPropagateShaders[2];      // shaders (with and without occlusion)

	// objects used for smoothing light propagation results
	DX11_Shader *gridSmoothShaders[2];          // shaders for interpolating between light propagation results of current and last frame for FINE_GRID/ COARSE_GRID

	// objects used for generating the indirect illumination
  DX11_RenderTargetConfig *outputRTC;         // only render into the accumulation render-target of the GBuffers
	DX11_Shader *globalIllumShaders[2];         // shaders for generating indirect illumination
	DX11_DepthStencilState *stencilTestDSS;     // only illuminate actual geometry, not the sky

	// objects used for visualizing the voxel-grids
	DX11_Shader *gridVisShader;                 //  shader for voxel-grid visualization

	// objects used for clearing the voxel-grids  
	DX11_RenderTargetConfig *clearRTC;          // render-target config for clearing FINE_GRID + COARSE_GRID
	DX11_Shader *clearShader;                   // shader to clear FINE_GRID + COARSE_GRID

	// data for grid uniform-buffer
	BufferData bufferData;
	
	// helper variables  
	float gridHalfExtents[2];                   // half extents of cubic FINE_GRID/ COARSE_GRID     
	Matrix4 gridProjMatrices[2];                // orthographic projection matrices for FINE_GRID/ COARSE_GRID   
	globalIllumModes mode;
	bool useOcclusion;
	bool allowSmoothing;
	bool useSmoothing;
	unsigned int numPropagationSteps;
	
};

#endif