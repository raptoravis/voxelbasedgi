#include <stdafx.h>
#include <Demo.h>
#include <DX11_VertexLayout.h>

static const char *elementFormatNames[] = { "float", "float2", "float3", "float4" };
static const DXGI_FORMAT elementFormats[] = { DXGI_FORMAT_R32_FLOAT, DXGI_FORMAT_R32G32_FLOAT,
											  DXGI_FORMAT_R32G32B32_FLOAT, DXGI_FORMAT_R32G32B32A32_FLOAT };
static const unsigned int elementFormatSizes[] = { 4, 8, 12, 16 };

static const char *elementNames[] = { "position", "texCoords", "normal", "tangent", "color" };
static const char *semanticNames[] = { "POSITION", "TEXCOORD", "NORMAL", "TANGENT", "COLOR" };

void DX11_VertexLayout::Release()
{
	SAFE_RELEASE(inputLayout);
	SAFE_DELETE_ARRAY(vertexElementDescs);
}

bool DX11_VertexLayout::Create(const VertexElementDesc *vertexElementDescs, unsigned int numVertexElementDescs)
{
	if ((!vertexElementDescs) || (numVertexElementDescs < 1))
		return false;

	this->numVertexElementDescs = numVertexElementDescs;
	this->vertexElementDescs = new VertexElementDesc[numVertexElementDescs];
	if (!this->vertexElementDescs)
		return false;
	memcpy(this->vertexElementDescs, vertexElementDescs, sizeof(VertexElementDesc)*numVertexElementDescs);

	// create tmp shader for input layout
	std::string str;
	str = "struct VS_Input\n{\n  ";
	for (unsigned int i = 0; i < numVertexElementDescs; i++)
	{
		VertexElementDesc desc = vertexElementDescs[i];
		str += elementFormatNames[desc.format];
		str += " ";
		str += elementNames[desc.vertexElement];
		str += ": ";
		str += semanticNames[desc.vertexElement];
		str += ";\n  ";
	}
	str += "};\n";
	str += "struct VS_Output\n{\n  float4 pos: SV_POSITION;\n};\n";
	str += "VS_Output main(VS_Input input)\n{\n  VS_Output output = (VS_Output)0;\n  return output;\n};";

	// compile tmp shader
	ID3DBlob* vsBlob = NULL;
	ID3DBlob* errorBlob = NULL;
	if (D3DX11CompileFromMemory(str.c_str(), str.length(), "tmp", NULL, NULL, "main", "vs_5_0",
		D3DCOMPILE_ENABLE_STRICTNESS, 0, NULL, &vsBlob, &errorBlob, NULL) != S_OK)
	{
		if (errorBlob)
		{
			MessageBox(NULL, (char*)errorBlob->GetBufferPointer(), "Vertex Shader Error", MB_OK | MB_ICONEXCLAMATION);
			SAFE_RELEASE(errorBlob);
		}
		return false;
	}
	SAFE_RELEASE(errorBlob);

	// create input layout
	D3D11_INPUT_ELEMENT_DESC *layout = new D3D11_INPUT_ELEMENT_DESC[numVertexElementDescs];
	for (unsigned int i = 0; i < numVertexElementDescs; i++)
	{
		VertexElementDesc desc = vertexElementDescs[i];
		layout[i].SemanticName = semanticNames[desc.vertexElement];
		layout[i].SemanticIndex = 0;
		layout[i].Format = elementFormats[desc.format];
		layout[i].InputSlot = 0;
		layout[i].AlignedByteOffset = desc.offset;
		layout[i].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		layout[i].InstanceDataStepRate = 0;
	}
	if (Demo::renderer->GetDevice()->CreateInputLayout(layout, numVertexElementDescs, vsBlob->GetBufferPointer(),
		vsBlob->GetBufferSize(), &inputLayout) != S_OK)
	{
		SAFE_RELEASE(vsBlob);
		SAFE_DELETE_ARRAY(layout);
		return false;
	}
	SAFE_RELEASE(vsBlob);
	SAFE_DELETE_ARRAY(layout);

	return true;
}

void DX11_VertexLayout::Bind() const
{
	Demo::renderer->GetDeviceContext()->IASetInputLayout(inputLayout);
}

unsigned int DX11_VertexLayout::CalcVertexSize() const
{
	unsigned int vertexSize = 0;
	for (unsigned int i = 0; i < numVertexElementDescs; i++)
		vertexSize += elementFormatSizes[vertexElementDescs[i].format];
	return vertexSize;
}

bool DX11_VertexLayout::IsEqual(const VertexElementDesc *vertexElementDescs, unsigned int numVertexElementDescs) const
{
	if ((!vertexElementDescs) || (this->numVertexElementDescs != numVertexElementDescs))
		return false;
	for (unsigned int i = 0; i < numVertexElementDescs; i++)
	{
		if (this->vertexElementDescs[i].vertexElement != vertexElementDescs[i].vertexElement)
			return false;
		if (this->vertexElementDescs[i].format != vertexElementDescs[i].format)
			return false;
		if (this->vertexElementDescs[i].offset != vertexElementDescs[i].offset)
			return false;
	}
	return true;
}
