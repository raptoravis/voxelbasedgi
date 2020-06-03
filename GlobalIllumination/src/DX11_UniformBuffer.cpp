#include <stdafx.h>
#include <Demo.h>
#include <DX11_UniformBuffer.h>

void DX11_UniformBuffer::Release()
{
  SAFE_RELEASE(uniformBuffer);
}

bool DX11_UniformBuffer::Create(unsigned int bufferSize)
{
  if(bufferSize < 4)
    return false;

  size = bufferSize;

	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	unsigned int alignedSize = size;
	unsigned int align = alignedSize % 16;
	if(align > 0)
		alignedSize += 16-align;
	bd.ByteWidth = alignedSize;
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	if(Demo::renderer->GetDevice()->CreateBuffer(&bd, NULL, &uniformBuffer) != S_OK)
		return false;

	return true;
}

bool DX11_UniformBuffer::Update(const void *uniformBufferData) 
{
	if(!uniformBufferData)
		return false;
	
	D3D11_MAPPED_SUBRESOURCE MappedResource;
	Demo::renderer->GetDeviceContext()->Map(uniformBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
	float *resourceData = (float*)MappedResource.pData;
  memcpy(resourceData, uniformBufferData, size);
  Demo::renderer->GetDeviceContext()->Unmap(uniformBuffer, 0);

	return true;
}

void DX11_UniformBuffer::Bind(uniformBufferBP bindingPoint, shaderTypes shaderType) const
{
	switch(shaderType)
	{
	case VERTEX_SHADER:
		Demo::renderer->GetDeviceContext()->VSSetConstantBuffers(bindingPoint, 1, &uniformBuffer);
		break;

	case GEOMETRY_SHADER:
		Demo::renderer->GetDeviceContext()->GSSetConstantBuffers(bindingPoint, 1, &uniformBuffer);
		break;

	case FRAGMENT_SHADER: 
		Demo::renderer->GetDeviceContext()->PSSetConstantBuffers(bindingPoint, 1, &uniformBuffer);
		break;

	case COMPUTE_SHADER: 
		Demo::renderer->GetDeviceContext()->CSSetConstantBuffers(bindingPoint, 1, &uniformBuffer);
		break;
	}
}

