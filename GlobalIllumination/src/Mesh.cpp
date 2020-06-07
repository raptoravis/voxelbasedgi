#include <stdafx.h>
#include <Demo.h>
#include <Mesh.h>

bool Mesh::Create(const MeshDesc &desc)
{
	primitiveType = desc.primitiveType;
	if (desc.numVertices < 1)
		return false;

	// create vertex layout
	vertexLayout = Demo::renderer->CreateVertexLayout(desc.vertexElementDescs, desc.numVertexElementDescs);
	if (!vertexLayout)
		return false;

	// create vertex buffer
	vertexBuffer = Demo::renderer->CreateVertexBuffer(vertexLayout->CalcVertexSize(), desc.numVertices, desc.dynamic);
	if (!vertexBuffer)
		return false;

	// create index buffer
	if (desc.numIndices > 0)
	{
		indexBuffer = Demo::renderer->CreateIndexBuffer(desc.numIndices, desc.dynamic);
		if (!indexBuffer)
			return false;
	}

	return true;
}
