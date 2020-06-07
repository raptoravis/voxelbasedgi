#include <stdafx.h>
#include <Demo.h>
#include <DX11_IndexBuffer.h>

void DX11_IndexBuffer::Release()
{
	SAFE_RELEASE(indexBuffer);
	SAFE_DELETE_ARRAY(indices);
}

bool DX11_IndexBuffer::Create(unsigned int maxIndexCount, bool dynamic)
{
	this->maxIndexCount = maxIndexCount;
	this->dynamic = dynamic;
	indices = new unsigned int[maxIndexCount];
	if (!indices)
		return false;

	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.ByteWidth = sizeof(unsigned int)*maxIndexCount;
	if (dynamic)
	{
		bd.Usage = D3D11_USAGE_DYNAMIC;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	}
	else
	{
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.CPUAccessFlags = 0;
	}
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;

	if (Demo::renderer->GetDevice()->CreateBuffer(&bd, NULL, &indexBuffer) != S_OK)
		return false;

	return true;
}

unsigned int DX11_IndexBuffer::AddIndices(unsigned int numIndices, const unsigned int *newIndices)
{
	int firstIndex = currentIndexCount;
	currentIndexCount += numIndices;
	assert(currentIndexCount <= maxIndexCount);
	memcpy(&indices[firstIndex], newIndices, sizeof(unsigned int)*numIndices);
	return firstIndex;
}

bool DX11_IndexBuffer::Update()
{
	if (currentIndexCount > 0)
	{
		if (dynamic)
		{
			D3D11_MAPPED_SUBRESOURCE mappedResource;
			if (Demo::renderer->GetDeviceContext()->Map(indexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource) != S_OK)
				return false;
			memcpy(mappedResource.pData, indices, sizeof(unsigned int)*currentIndexCount);
			Demo::renderer->GetDeviceContext()->Unmap(indexBuffer, 0);
		}
		else
		{
			Demo::renderer->GetDeviceContext()->UpdateSubresource(indexBuffer, 0, NULL, indices, 0, 0);
		}
	}
	return true;
}

void DX11_IndexBuffer::Bind() const
{
	Demo::renderer->GetDeviceContext()->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);
}
