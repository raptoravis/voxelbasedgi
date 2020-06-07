#ifndef DX11_RENDERER_H
#define DX11_RENDERER_H

#include <vertex_types.h>
#include <List.h>
#include <GpuCmd.h>
#include <DX11_Sampler.h>
#include <DX11_RasterizerState.h>
#include <DX11_DepthStencilState.h>
#include <DX11_BlendState.h>
#include <DX11_RenderTargetConfig.h>
#include <DX11_RenderTarget.h>
#include <DX11_VertexLayout.h>
#include <DX11_VertexBuffer.h>
#include <DX11_IndexBuffer.h>
#include <DX11_UniformBuffer.h>
#include <DX11_StructuredBuffer.h>
#include <DX11_Texture.h>
#include <DX11_Shader.h>
#include <Camera.h>
#include <PointLight.h>
#include <DirectionalLight.h>
#include <Mesh.h>
#include <IPostProcessor.h>

#define CLEAR_COLOR Color(0.0f, 0.0f, 0.0f, 0.0f) // render-target clear color
#define CLEAR_DEPTH 1.0f // render-target clear depth
#define CLEAR_STENCIL 0 // render-target clear stencil 

// predefined IDs for frequently used samplers
enum samplerID
{
	LINEAR_SAMPLER_ID = 0,
	TRILINEAR_SAMPLER_ID,
	SHADOW_MAP_SAMPLER_ID
};

// predefined IDs for frequently used render-targets
enum renderTargetID
{
	BACK_BUFFER_RT_ID = 0, // back buffer
	GBUFFERS_RT_ID, // geometry buffers
	SHADOW_MAP_RT_ID // shadow map
};

// predefined IDs for frequently used cameras 
enum cameraID
{
	MAIN_CAMERA_ID
};

// predefined IDs for frequently used meshes 
enum meshID
{
	UNIT_SPHERE_MESH_ID = 0
};

// DX11_Renderer
//
// Manages DirectX 11 rendering.
class DX11_Renderer
{
public:
	DX11_Renderer() :
		device(NULL),
		deviceContext(NULL),
		swapChain(NULL),
		noneCullRS(NULL),
		noDepthTestDSS(NULL),
		defaultBS(NULL),
		lastGpuCmd(DRAW_CM)
	{
	}

	~DX11_Renderer()
	{
		Destroy();
	}

	void Destroy();

	bool Create();

	DX11_Sampler* CreateSampler(const SamplerDesc &desc);

	DX11_Sampler* GetSampler(unsigned int ID) const
	{
		assert(ID < samplers.GetSize());
		return samplers[ID];
	}

	DX11_RasterizerState* CreateRasterizerState(const RasterizerDesc &desc);

	DX11_DepthStencilState* CreateDepthStencilState(const DepthStencilDesc &desc);

	DX11_BlendState* CreateBlendState(const BlendDesc &desc);

	DX11_RenderTargetConfig* CreateRenderTargetConfig(const RtConfigDesc &desc);

	DX11_RenderTarget* CreateRenderTarget(const RenderTargetDesc &desc);

	DX11_RenderTarget* CreateBackBufferRt();

	DX11_RenderTarget* GetRenderTarget(unsigned int ID) const
	{
		assert(ID < renderTargets.GetSize());
		return renderTargets[ID];
	}

	DX11_VertexLayout* CreateVertexLayout(const VertexElementDesc *vertexElementDescs, unsigned int numVertexElementDescs);

	DX11_VertexBuffer* CreateVertexBuffer(unsigned int vertexSize, unsigned int maxVertexCount, bool dynamic);

	DX11_IndexBuffer* CreateIndexBuffer(unsigned int maxIndexCount, bool dynamic);

	DX11_UniformBuffer* CreateUniformBuffer(unsigned int bufferSize);

	DX11_StructuredBuffer* CreateStructuredBuffer(unsigned int elementCount, unsigned int elementSize);

	Camera* CreateCamera(float fovy, float aspectRatio, float nearClipDistance, float farClipDistance);

	Camera* GetCamera(unsigned int ID) const
	{
		assert(ID < cameras.GetSize());
		return cameras[ID];
	}

	PointLight* CreatePointLight(const Vector3 &position, float radius, const Color &color, float multiplier);

	DirectionalLight* CreateDirectionalLight(const Vector3 &direction, const Color &color, float multiplier);

	ILight* GetLight(unsigned int index) const
	{
		assert(index < lights.GetSize());
		return lights[index];
	}

	Mesh* CreateMesh(const MeshDesc &desc);

	Mesh* GetMesh(unsigned int ID) const
	{
		assert(ID < meshes.GetSize());
		return meshes[ID];
	}

	template<class T> T* CreatePostProcessor()
	{
		T *postProcessor = new T;
		if (!postProcessor)
			return NULL;
		if (!postProcessor->Create())
		{
			SAFE_DELETE(postProcessor);
			return NULL;
		}
		postProcessors.AddElement((IPostProcessor**)(&postProcessor));
		return postProcessor;
	}

	IPostProcessor* GetPostProcessor(const char *name) const;

	unsigned int GetNumLights() const
	{
		return lights.GetSize();
	}

	void UpdateLights();

	void SetupPostProcessSurface(DrawCmd &drawCmd);

	// add new GPU command per frame 
	void AddGpuCmd(const GpuCmd &gpuCmd);

	void ClearFrame();

	// execute all GPU commands, which have been passed per frame to renderer
	void ExecuteGpuCmds();

	// save a BMP screen-shot
	void SaveScreenshot() const;

	ID3D11Device* GetDevice() const
	{
		return device;
	}

	ID3D11DeviceContext* GetDeviceContext() const
	{
		return deviceContext;
	}

	IDXGISwapChain* GetSwapChain() const
	{
		return swapChain;
	}

private:
	// create frequently used objects
	bool CreateDefaultObjects();

	// create unit sphere geometry
	bool CreateUnitSphere();

	void ExecutePostProcessors();

	// set draw states for passed draw command
	void SetDrawStates(const DrawCmd &cmd);

	// set shader states for passed shader command
	void SetShaderStates(const ShaderCmd &cmd, gpuCmdOrders order);

	// set shader params for passed shader command
	void SetShaderParams(const ShaderCmd &cmd);

	void Draw(DrawCmd &cmd);

	void Dispatch(ComputeCmd &cmd);

	void UnbindShaderResources();

	// list of all samplers
	List<DX11_Sampler*> samplers;

	// list of all rasterizer states
	List<DX11_RasterizerState*> rasterizerStates;

	// list of all depth-stencil states
	List<DX11_DepthStencilState*> depthStencilStates;

	// list of all blend states
	List<DX11_BlendState*> blendStates;

	// list of all render-target configs
	List<DX11_RenderTargetConfig*> renderTargetConfigs;

	// list of all render-targets
	List<DX11_RenderTarget*> renderTargets;

	// list of all vertex layouts
	List<DX11_VertexLayout*> vertexLayouts;

	// list of all vertex buffers
	List<DX11_VertexBuffer*> vertexBuffers;

	// list of all index buffers
	List<DX11_IndexBuffer*> indexBuffers;

	// list of all uniform buffers
	List<DX11_UniformBuffer*> uniformBuffers;

	// list of all structured buffers
	List<DX11_StructuredBuffer*> structuredBuffers;

	// list of all cameras
	List<Camera*> cameras;

	// list of all dynamic lights
	List<ILight*> lights;

	// list of all dynamically created meshes
	List<Mesh*> meshes;

	// list of all post-processors
	List<IPostProcessor*> postProcessors;

	// list of all per frame passed GPU commands 
	List<GpuCmd> gpuCmds;

	// DirectX 11 objects
	ID3D11Device *device;
	ID3D11DeviceContext *deviceContext;
	IDXGISwapChain *swapChain;

	// render-states, frequently used by post-processors
	DX11_RasterizerState *noneCullRS;
	DX11_DepthStencilState *noDepthTestDSS;
	DX11_BlendState *defaultBS;

	// helper variable
	GpuCmd lastGpuCmd;

};

#endif 