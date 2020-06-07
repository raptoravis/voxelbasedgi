#ifndef VERTEX_TYPES_H
#define VERTEX_TYPES_H

enum vertexElements
{
	POSITION_ELEMENT = 0,
	TEXCOORDS_ELEMENT,
	NORMAL_ELEMENT,
	TANGENT_ELEMENT,
	COLOR_ELEMENT
};

enum elementFormats
{
	R32_FLOAT_EF = 0,
	R32G32_FLOAT_EF,
	R32G32B32_FLOAT_EF,
	R32G32B32A32_FLOAT_EF
};

struct VertexElementDesc
{
	vertexElements vertexElement;
	elementFormats format;
	unsigned int offset;
};

// this vertex-type is used by demo-meshes
struct GeometryVertex
{
	Vector3 position;
	Vector2 texCoords;
	Vector3 normal;
	Vector4 tangent;
};

// this vertex-type is used by fonts
struct FontVertex
{
	Vector3 position;
	Vector2 texCoords;
	Color color;
};

#endif