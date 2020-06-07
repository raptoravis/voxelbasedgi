#ifndef DX11_UNIFORM_BUFFER_H
#define DX11_UNIFORM_BUFFER_H

#include <render_states.h>

// DX11_UniformBuffer
//
// Manages a DirectX 11 constant buffer.
class DX11_UniformBuffer
{
public:
	DX11_UniformBuffer() :
		size(0),
		uniformBuffer(NULL)
	{
	}

	~DX11_UniformBuffer()
	{
		Release();
	}

	void Release();

	bool Create(unsigned int bufferSize);

	// Please note: uniforms must be aligned according to the HLSL rules, in order to be able
	// to upload data in 1 block.
	bool Update(const void *uniformBufferData);

	void Bind(uniformBufferBP bindingPoint, shaderTypes shaderType = VERTEX_SHADER) const;

private:
	unsigned int size; // size of uniform data
	ID3D11Buffer *uniformBuffer;

};

#endif