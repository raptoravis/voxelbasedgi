#ifndef DEMO_MESH_H
#define DEMO_MESH_H

#include <List.h>

#define CURRENT_DEMO_MESH_VERSION 1

class DX11_RenderTargetConfig;
class DX11_VertexLayout;
class DX11_VertexBuffer;
class DX11_IndexBuffer;
class DX11_UniformBuffer;
class DX11_Shader;
class Material;
class GlobalIllum;

struct DemoSubmesh
{
	DemoSubmesh():
    material(NULL),
    firstIndex(0),
    numIndices(0)
	{
	}

	Material *material;
	unsigned int firstIndex;
	unsigned int numIndices;
};

// DemoMesh
//
// Simple custom mesh format (".mesh") for storing non-animated meshes. Since for simplicity this demo does 
// not use any kind of visibility determination algorithms, all sub-meshes with the same material are already
// pre-batched. Furthermore the normalized normals and tangents as well as the tangent-space handedness are 
// already calculated.
class DemoMesh
{
public:
	DemoMesh():
    baseVertexLayout(NULL),
    gridVertexLayout(NULL),
    shadowVertexLayout(NULL),
    vertexBuffer(NULL),
    indexBuffer(NULL),
    uniformBuffer(NULL),
    multiRTC(NULL),
    globalIllumPP(NULL),
    active(true),
    performUpdate(true)
	{
	  name[0] = 0;
	}

	~DemoMesh()
	{
		Release();
	}

	void Release();

	bool Load(const char *filename);

	void AddSurfaces();

	void SetActive(bool active)
	{
		this->active = active;
	}

	bool IsActive() const
	{
		return active;
	}

	const char* GetName() const
	{
		return name;
	}

	void SetPosition(const Vector3 &position);

	Vector3 GetPosition() const 
	{
		return position;
	}

private:	 
  void Update();

	// adds surfaces for filling the GBuffers
	void AddBaseSurfaces();

	// adds surfaces for generating the voxel-grids
	void AddGridSurfaces();

	// adds surfaces for generating shadow map for specified light
	void AddShadowMapSurfaces(unsigned int lightIndex);

	// data for uniform-buffer
	Vector3 position;

  DX11_VertexLayout *baseVertexLayout;
  DX11_VertexLayout *gridVertexLayout;
  DX11_VertexLayout *shadowVertexLayout;
  DX11_VertexBuffer *vertexBuffer;
	DX11_IndexBuffer *indexBuffer;
	DX11_UniformBuffer *uniformBuffer;
	DX11_RenderTargetConfig *multiRTC; 
	GlobalIllum *globalIllumPP;

  List<DemoSubmesh*> subMeshes; // list of all sub-meshes
  char name[DEMO_MAX_FILENAME];
  bool active;
  bool performUpdate;

};

#endif