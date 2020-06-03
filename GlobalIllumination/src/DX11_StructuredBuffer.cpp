#include <stdafx.h>
#include <Demo.h>
#include <DX11_StructuredBuffer.h>

void DX11_StructuredBuffer::Release()
{
	SAFE_RELEASE(structuredBuffer);
	SAFE_RELEASE(unorderedAccessView);
  SAFE_RELEASE(shaderResourceView);
}

bool DX11_StructuredBuffer::Create(unsigned int elementCount, unsigned int elementSize)
{
	this->elementCount = elementCount;
	this->elementSize = elementSize;

	D3D11_BUFFER_DESC bufferDesc;
	unsigned int stride = elementSize;
	bufferDesc.ByteWidth = stride*elementCount;
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	bufferDesc.StructureByteStride = stride;
	if(Demo::renderer->GetDevice()->CreateBuffer(&bufferDesc, NULL, &structuredBuffer) != S_OK)
		return false;

	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	uavDesc.Buffer.FirstElement = 0;
	uavDesc.Buffer.Flags = 0;
	uavDesc.Buffer.NumElements = elementCount; 
	if(Demo::renderer->GetDevice()->CreateUnorderedAccessView(structuredBuffer, &uavDesc, &unorderedAccessView) != S_OK)
		return false;
	
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.ElementOffset = 0;
	srvDesc.Buffer.ElementWidth = elementCount;
	if(Demo::renderer->GetDevice()->CreateShaderResourceView(structuredBuffer, &srvDesc, &shaderResourceView) != S_OK) 
		return false;

	return true;
}

void DX11_StructuredBuffer::Bind(structuredBufferBP bindingPoint, shaderTypes shaderType) const
{
	switch(shaderType)
	{
	case VERTEX_SHADER:
		Demo::renderer->GetDeviceContext()->VSSetShaderResources(bindingPoint, 1, &shaderResourceView);
		break;

	case GEOMETRY_SHADER:
		Demo::renderer->GetDeviceContext()->GSSetShaderResources(bindingPoint, 1, &shaderResourceView);
		break;

	case FRAGMENT_SHADER: 
		Demo::renderer->GetDeviceContext()->PSSetShaderResources(bindingPoint, 1, &shaderResourceView);
		break;

	case COMPUTE_SHADER:
		Demo::renderer->GetDeviceContext()->CSSetShaderResources(bindingPoint, 1, &shaderResourceView);
		break;
	}
}

