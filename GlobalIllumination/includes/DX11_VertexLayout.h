#ifndef DX11_VERTEX_LAYOUT_H
#define DX11_VERTEX_LAYOUT_H

#include <vertex_types.h>

// DX11_VertexLayout
//
// Manages a DirectX 11 input layout.
class DX11_VertexLayout
{
public:
	DX11_VertexLayout():
    inputLayout(NULL),
    vertexElementDescs(NULL),
    numVertexElementDescs(0)
	{
	}

	~DX11_VertexLayout()
	{
		Release();
	}

	void Release();

	bool Create(const VertexElementDesc *vertexElementDescs, unsigned int numVertexElementDescs);

	void Bind() const;

  unsigned int CalcVertexSize() const;

  bool IsEqual(const VertexElementDesc *vertexElementDescs, unsigned int numVertexElementDescs) const;

private:
	ID3D11InputLayout *inputLayout;
  VertexElementDesc *vertexElementDescs;
  unsigned int numVertexElementDescs;

};

#endif