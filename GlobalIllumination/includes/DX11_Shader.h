#ifndef DX11_SHADER_H
#define DX11_SHADER_H

#include <List.h>
#include <render_states.h>

#define CURRENT_SHADER_VERSION 3 // current version of binary pre-compiled shaders

class DX11_UniformBuffer;
class DX11_StructuredBuffer;
class DX11_Texture;
class DX11_Sampler;

// DX11_Shader
//
// Loaded from a simple text-file (".sdr"), that references the actual shader source files.
// To avoid long loading times, per default pre-compiled shaders are used.
class DX11_Shader
{
public:
	friend class ShaderInclude;

	DX11_Shader() :
		permutationMask(0),
		vertexShader(NULL),
		geometryShader(NULL),
		fragmentShader(NULL),
		computeShader(NULL)
	{
		name[0] = 0;
		memset(byteCodes, 0, sizeof(ID3DBlob*)*NUM_SHADER_TYPES);
		memset(uniformBufferMasks, 0, sizeof(unsigned int)*NUM_UNIFORM_BUFFER_BP);
		memset(textureMasks, 0, sizeof(unsigned int)*(NUM_TEXTURE_BP + NUM_STRUCTURED_BUFFER_BP));
	}

	~DX11_Shader()
	{
		Release();
	}

	void Release();

	bool Load(const char *fileName, unsigned int permutationMask = 0);

	void Bind() const;

	void SetUniformBuffer(uniformBufferBP bindingPoint, const DX11_UniformBuffer *uniformBuffer) const;

	void SetStructuredBuffer(structuredBufferBP bindingPoint, const DX11_StructuredBuffer *structuredBuffer) const;

	void SetTexture(textureBP bindingPoint, const DX11_Texture *texture, const DX11_Sampler *sampler) const;

	unsigned int GetPermutationMask() const
	{
		return permutationMask;
	}

	const char* GetName() const
	{
		return name;
	}

private:
	void LoadDefines(std::ifstream &file);

	static bool ReadShaderFile(const char *fileName, unsigned char **data, unsigned int *dataSize);

	bool CreateShaderUnit(shaderTypes shaderType, void *byteCode, unsigned int shaderSize);

	bool InitShaderUnit(shaderTypes shaderType, const char *fileName);

	bool LoadShaderUnit(shaderTypes shaderType, std::ifstream &file);

	bool CreateShaderMacros();

	void ParseShaderString(shaderTypes shaderType, const char *shaderString);

	bool LoadShaderBin(const char *filePath);

	bool SaveShaderBin(const char *filePath);

	List<char[DEMO_MAX_STRING]> defineStrings;
	unsigned int permutationMask;
	char name[DEMO_MAX_FILENAME];

	ID3D11VertexShader *vertexShader;
	ID3D11GeometryShader *geometryShader;
	ID3D11PixelShader *fragmentShader;
	ID3D11ComputeShader *computeShader;
	List<D3D10_SHADER_MACRO> shaderMacros;
	ID3DBlob *byteCodes[NUM_SHADER_TYPES];

	unsigned int uniformBufferMasks[NUM_UNIFORM_BUFFER_BP];
	unsigned int textureMasks[NUM_TEXTURE_BP + NUM_STRUCTURED_BUFFER_BP];

	static const char *shaderModels[NUM_SHADER_TYPES];
	static const char *uniformBufferRegisterNames[NUM_UNIFORM_BUFFER_BP];
	static const char *textureRegisterNames[NUM_TEXTURE_BP + NUM_STRUCTURED_BUFFER_BP];

};

#endif
