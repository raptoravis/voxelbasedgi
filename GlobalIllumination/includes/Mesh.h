#ifndef MESH_H
#define MESH_H

class DX11_VertexLayout;
class DX11_VertexBuffer;
class DX11_IndexBuffer;

// descriptor for setting up a generic mesh
struct MeshDesc
{
  MeshDesc():
    primitiveType(TRIANGLES_PRIMITIVE),
    vertexElementDescs(NULL),
    numVertexElementDescs(0),
    numVertices(0),
    numIndices(0),
    dynamic(false)
  {
  }

  primitiveTypes primitiveType;
  VertexElementDesc *vertexElementDescs;
  unsigned int numVertexElementDescs;
  unsigned int numVertices;
  unsigned int numIndices;
  bool dynamic;
};

// Mesh
//
// Simple generic mesh class.
class Mesh
{
public:
	Mesh():
    vertexLayout(NULL),
    vertexBuffer(NULL),
    indexBuffer(NULL),
    primitiveType(TRIANGLES_PRIMITIVE)
	{
	}

  bool Create(const MeshDesc &desc);

  DX11_VertexLayout *vertexLayout;
	DX11_VertexBuffer *vertexBuffer;
	DX11_IndexBuffer *indexBuffer;
	primitiveTypes primitiveType;

};

#endif