#ifndef DX11_STRUCTURED_BUFFER_H
#define DX11_STRUCTURED_BUFFER_H

#include <render_states.h>

// DX11_StructuredBuffer
//
// Manages a DirectX 11 structured buffer.
class DX11_StructuredBuffer
{
public:
	DX11_StructuredBuffer() :
		elementCount(0),
		elementSize(0),
		structuredBuffer(NULL),
		unorderedAccessView(NULL),
		shaderResourceView(NULL)
	{
	}

	~DX11_StructuredBuffer()
	{
		Release();
	}

	void Release();

	bool Create(unsigned int elementCount, unsigned int elementSize);

	void Bind(structuredBufferBP bindingPoint, shaderTypes shaderType = VERTEX_SHADER) const;

	unsigned int GetElementCount() const
	{
		return elementCount;
	}

	unsigned int GetElementSize() const
	{
		return elementCount;
	}

	ID3D11UnorderedAccessView* GetUnorderdAccessView() const
	{
		return unorderedAccessView;
	}

private:
	unsigned int elementCount; // number of structured elements in buffer
	unsigned int elementSize; // size of 1 structured element in bytes
	ID3D11Buffer *structuredBuffer;
	ID3D11UnorderedAccessView *unorderedAccessView;
	ID3D11ShaderResourceView *shaderResourceView;

};

#endif