#ifndef RESOURCE_MANAGER_H
#define RESOURCE_MANAGER_H

#include <List.h>
#include <DX11_Shader.h>
#include <DX11_Texture.h>
#include <Material.h>
#include <Font.h>
#include <DemoMesh.h>

// ResourceManager
//
// Manages resources (shaders, textures, materials, fonts, demo-meshes).
class ResourceManager
{
public:
	ResourceManager()
	{
	}

	~ResourceManager()
	{
		Release();
	}

	void Release();

	// loads ".sdr" shader-file (references the actual shader source files)
	DX11_Shader* LoadShader(const char *fileName, unsigned int permutationMask = 0);

	DX11_Texture* LoadTexture(const char *fileName);

	// loads ".mtl" material-file
	Material* LoadMaterial(const char *fileName);

	// loads ".font" font-file 
	Font* LoadFont(const char *fileName);

	Font* GetFont(unsigned int index) const
	{
		assert(index < fonts.GetSize());
		return fonts[index];
	}

	// loads ".mesh" mesh-file
	DemoMesh* LoadDemoMesh(const char *fileName);

	DemoMesh* GetDemoMesh(unsigned int index) const
	{
		assert(index < demoMeshes.GetSize());
		return demoMeshes[index];
	}

	unsigned int GetNumFonts() const
	{
		return fonts.GetSize();
	}

	unsigned int GetNumDemoMeshes() const
	{
		return demoMeshes.GetSize();
	}

private:
	List<DX11_Shader*> shaders;
	List<DX11_Texture*> textures;
	List<Material*> materials;
	List<Font*> fonts;
	List<DemoMesh*> demoMeshes;

};

#endif
