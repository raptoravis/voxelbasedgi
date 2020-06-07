#ifndef GPU_CMD_H
#define GPU_CMD_H

#include <render_states.h>

class DX11_VertexLayout;
class DX11_VertexBuffer;
class DX11_IndexBuffer;
class DX11_UniformBuffer;
class DX11_StructuredBuffer;
class DX11_RenderTargetConfig;
class DX11_RenderTarget;
class DX11_RasterizerState;
class DX11_DepthStencilState;
class DX11_BlendState;
class DX11_Texture;
class DX11_Shader;
class DX11_Sampler;
class Camera;
class ILight;

enum gpuCmdOrders
{
	BASE_CO = 0, // fill GBuffers
	GRID_FILL_CO, // fill voxel-grid
	SHADOW_CO, // generate shadow maps
	ILLUM_CO, // direct illumination
	GRID_ILLUM_CO, // illuminate voxel-grid
	GLOBAL_ILLUM_CO, // generate global illumination
	SKY_CO, // render sky
	POST_PROCESS_CO, // perform post-processing
	GUI_CO, // render GUI
};

enum gpuCmdModes
{
	DRAW_CM = 0, // draw-call
	COMPUTE_CM, // dispatch of compute-shader
};

struct ShaderCmd
{
	DX11_RenderTarget *renderTarget;
	DX11_RenderTargetConfig *renderTargetConfig;
	Camera *camera;
	ILight *light;
	DX11_Texture *textures[NUM_TEXTURE_BP];
	DX11_Sampler *samplers[NUM_TEXTURE_BP];
	DX11_UniformBuffer *customUBs[NUM_CUSTOM_UNIFORM_BUFFER_BP];
	DX11_StructuredBuffer *customSBs[NUM_STRUCTURED_BUFFER_BP];
	DX11_Shader *shader;
};

struct DrawCmd : public ShaderCmd
{
	DX11_VertexLayout *vertexLayout;
	DX11_VertexBuffer *vertexBuffer;
	DX11_IndexBuffer *indexBuffer;
	DX11_RasterizerState *rasterizerState;
	DX11_DepthStencilState *depthStencilState;
	DX11_BlendState *blendState;
	primitiveTypes primitiveType;
	unsigned int firstIndex;
	unsigned int numElements;
	unsigned int numInstances;
};

struct ComputeCmd : public ShaderCmd
{
	unsigned int numThreadGroupsX;
	unsigned int numThreadGroupsY;
	unsigned int numThreadGroupsZ;
};

// GpuCmd
//
class GpuCmd
{
public:
	friend class DX11_Renderer;

	explicit GpuCmd(gpuCmdModes mode)
	{
		Reset(mode);
	}

	void Reset(gpuCmdModes mode)
	{
		memset(this, 0, sizeof(GpuCmd));
		this->mode = mode;
		if (mode == DRAW_CM)
		{
			draw.primitiveType = TRIANGLES_PRIMITIVE;
			draw.numInstances = 1;
		}
	}

	gpuCmdModes GetMode() const
	{
		return mode;
	}

	unsigned int GetID() const
	{
		return ID;
	}

	gpuCmdOrders order;
	union
	{
		DrawCmd draw;
		ComputeCmd compute;
	};

private:
	gpuCmdModes mode;
	unsigned int ID;

};

#endif


